#include "state.hpp"
#include "logger.hpp"
#include "murmurhash3.hpp"
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

INIT_LOGGER(state);

namespace notificationLib {

State::State(size_t maxNotificationMemory, bool isList)
  : m_maxNotificationMemory(maxNotificationMemory)
  , m_isList(isList)
  , m_ibft(maxNotificationMemory, 4) // 4 bytes hash value size in ibf
                                     // key size (timestamp) is 8 bytes
{
}

void
State::addTimestamp(uint64_t timestamp, const std::vector<Name>& eventList)
{
  _LOG_DEBUG("State::addTimestamp(): index timestamp " << timestamp);
  m_ibft.insert(timestamp, _pseudoRandomValue(timestamp));

  _saveHistory(timestamp, eventList);
}
uint64_t
State::update(const std::vector<Name>& eventList)
{
  // get current timestamp in nanoseconds
  auto now_ns = boost::chrono::time_point_cast<boost::chrono::nanoseconds>(ndn::time::system_clock::now());
  auto now_ns_long_type = (now_ns.time_since_epoch()).count();

  _LOG_DEBUG("State::update(): index timestamp " << now_ns_long_type);
  m_ibft.insert(now_ns_long_type, _pseudoRandomValue(now_ns_long_type));

  //std::cout << "Table after update" << m_ibft.DumpTable()<< std::endl;
  _saveHistory(now_ns_long_type, eventList);
  return now_ns_long_type;
}
void
State::erase(const uint64_t timestamp)
{
  _LOG_DEBUG("State::erase(): remove timestamp " << timestamp);
  m_ibft.erase(timestamp, _pseudoRandomValue(timestamp));

  //std::cout << "Table after update" << m_ibft.DumpTable()<< std::endl;
  _removeFromHistory(timestamp);

}
bool
State::isExpired(const uint64_t now, uint64_t timestamp, ndn::time::milliseconds max_fresh)
{
  _LOG_DEBUG("State::isExpired(): now " << now);
  _LOG_DEBUG("State::isExpired(): timestamp " << timestamp);
  auto tPasses = now - timestamp;
  _LOG_DEBUG("State::isExpired(): tPasses " << tPasses);
  if(tPasses > (max_fresh.count()*1000000))
  {
    _LOG_DEBUG("State::isExpired(): expired!!" << tPasses);
    return true;
  }
  return false;
}

ConstBufferPtr
State::getState() const
{
  if(m_isList)
  {
    size_t estimatedSize = 0;
    EncodingEstimator estimator;
    //size_t estimatedSize = wireEncode(estimator);
    for(auto iTime: m_NotificationHistory)
    {
      estimatedSize += prependNonNegativeIntegerBlock(estimator, tlv::ListEntry, iTime.first);
    }
    estimatedSize += estimator.prependVarNumber(estimatedSize);
    estimatedSize += estimator.prependVarNumber(tlv::ListTable);

    EncodingBuffer buffer(estimatedSize);
    estimatedSize = 0;
    for(auto iTime: m_NotificationHistory)
    {
      estimatedSize += prependNonNegativeIntegerBlock(buffer, tlv::ListEntry, iTime.first);
    }
    estimatedSize += buffer.prependVarNumber(estimatedSize);
    estimatedSize += buffer.prependVarNumber(tlv::ListTable);


    //wireEncode(buffer);

    Block listBlock = buffer.block();

    auto contentBuffer = bzip2::compress(reinterpret_cast<const char*>(listBlock.wire()),
                                                                      listBlock.size());
    return contentBuffer;
  }
  else
  {
    Block ibfBlock = m_ibft.wireEncode();

    auto contentBuffer = bzip2::compress(reinterpret_cast<const char*>(ibfBlock.wire()),
                                                                      ibfBlock.size());
    return contentBuffer;
  }

}

bool State::getDiff(ConstBufferPtr rmtStateStr,
                   std::set<std::pair<uint64_t,std::vector<uint8_t> > >& inLocal,
                   std::set<std::pair<uint64_t,std::vector<uint8_t> > >& inRemote) const
{
  auto remoteBuf = bzip2::decompress(rmtStateStr->get<char>(),
                                     rmtStateStr->size());

  if(m_isList)
  {
    Block bufferBlock = Block(remoteBuf);
    std::vector<uint8_t> emptyVec;
    std::vector<uint64_t> decodedVec;
    if(!bufferBlock.hasWire())
    {
      _LOG_ERROR("no wire");
      return false;
    }

    if (bufferBlock.type() != tlv::ListTable)
    {
      _LOG_ERROR("expecting tlv::ListTable");
      return false;
    }
    bufferBlock.parse();
    for (Block::element_const_iterator it = bufferBlock.elements_begin();
         it != bufferBlock.elements_end(); it++)
    {
      if (it->type() == tlv::ListEntry)
      {
        uint64_t remoteTime =  readNonNegativeInteger(*it);
        decodedVec.push_back(remoteTime);
        //std::cout << "  *****Decoded: "<< remoteTime << std::endl;
      }
    }
    // create inLocal, if local is not in decoded then add
    for(auto i: m_NotificationHistory)
    {
      if ( std::find(decodedVec.begin(), decodedVec.end(), i.first) == decodedVec.end() )
         inLocal.insert(std::make_pair(i.first, emptyVec));
    }
    // create inRemote, if decoded not in local history then add
    for(auto i: decodedVec)
    {
      auto entry = m_NotificationHistory.find(i);
      if( entry == m_NotificationHistory.end())
        inRemote.insert(std::make_pair(i, emptyVec));
    }
    return true;
  }
  else
  {
    IBFT remoteIBF(remoteBuf, m_maxNotificationMemory, 4);

    // std::cout << "My IBF" << m_ibft.dumpItems() << std::endl;
    // std::cout << "Remote" << remoteIBF.dumpItems() << std::endl;

    IBFT diff = m_ibft-remoteIBF;
    return (diff.listEntries(inLocal, inRemote));
  }
}

bool
State::reconcile(ConstBufferPtr newState, NotificationData& data, ndn::time::milliseconds max_freshness)
{
  auto now_ns = boost::chrono::time_point_cast<boost::chrono::nanoseconds>(ndn::time::system_clock::now());
  auto now_ns_long_type = (now_ns.time_since_epoch()).count();
  std::set<std::pair<uint64_t,std::vector<uint8_t> > > inNew, inOld;
  //ConstBufferPtr oldState  = getState();
  if (getDiff(newState, inOld, inNew))
  {
    // for now, only add new timestamps to local IBF and History
    for(auto const& newit: inNew)
    {
      _LOG_DEBUG("State::reconcile: found new item: " << newit.first);
      if(!State::isExpired(now_ns_long_type, newit.first, max_freshness))
      {
        _LOG_DEBUG("State::reconcile: item is fresh  " << newit.first);
        addTimestamp(newit.first, data.m_eventsObj.getEventList(newit.first));
      }
      else
        _LOG_DEBUG("State::reconcile: item expired  " << newit.first);

      //listToPush[lit.first] = m_state.getEventsAtTimestamp(lit.first);
    }
    // TBD - handle removals
    return true;
  }
  else
  {
    _LOG_ERROR("State::reconcile: Unable to get Diff");
    return false;
  }
}
void
State::cleanup(ndn::time::milliseconds max_freshness)
{
  auto now_ns = boost::chrono::time_point_cast<boost::chrono::nanoseconds>(ndn::time::system_clock::now());
  auto now_ns_long_type = (now_ns.time_since_epoch()).count();

  std::set<std::pair<uint64_t,std::vector<uint8_t> > > positive;
  std::set<std::pair<uint64_t,std::vector<uint8_t> > > negative;

  if(m_ibft.listEntries(positive, negative))
  {
    std::set<std::pair<uint64_t,std::vector<uint8_t> > > :: iterator it; //iterator to manipulate set
    for (it = positive.begin(); it!=positive.end(); it++)
    {
        std::pair<uint64_t,std::vector<uint8_t> > m = *it; // returns pair to m
        if(isExpired(now_ns_long_type, m.first, max_freshness))
        {

          // remove from state
          erase(m.first);
        }
    }
    for (it = negative.begin(); it!=negative.end(); it++)
    {
        std::pair<uint64_t,std::vector<uint8_t> > m = *it; // returns pair to m
        if(isExpired(now_ns_long_type, m.first, max_freshness))
        {
          // remove from state
          erase(m.first);
        }
    }
  }
}

std::vector<Name>&
State::getEventsAtTimestamp(uint64_t timestamp)
{
  std::vector<Name> emptyVec;
  auto entry = m_NotificationHistory.find(timestamp);
  if(entry != m_NotificationHistory.end())
    return entry->second;
  else
    return emptyVec;
}
void
State::_removeFromHistory(uint64_t timestamp)
{
  _LOG_DEBUG("State::_removeFromHistory");

  m_NotificationHistory.erase(timestamp);

}

void
State::_saveHistory(uint64_t timestamp, const std::vector<Name>&eventList)
{
  _LOG_DEBUG("State::_saveHistory");

  m_NotificationHistory[timestamp] = eventList;

}
std::string State::dumpHistory() const
{
  return dumpHistory(m_NotificationHistory);
}

std::string State::dumpHistory(std::map<uint64_t,std::vector<Name>> history) const
{
  std::ostringstream result;
  result << "items in History:\n";

  for(auto const& hit: history)
  {
    result << "\t timestamp: "<< hit.first << "\n";
    for(auto const& nit: hit.second)
      result << "\t \t event: "<< nit << "\n";
  }
  return result.str();
}

std::string State::dumpItems() const
{
  std::ostringstream result;
  result << "items in IBF:\n";
  std::set<std::pair<uint64_t,std::vector<uint8_t> > > positive;
  std::set<std::pair<uint64_t,std::vector<uint8_t> > > negative;

  result << "can be resolved:" << m_ibft.listEntries(positive, negative) << "\n";
  std::set<std::pair<uint64_t,std::vector<uint8_t> > > :: iterator it; //iterator to manipulate set
  for (it = positive.begin(); it!=positive.end(); it++){
      std::pair<uint64_t,std::vector<uint8_t> > m = *it; // returns pair to m
      result << "key: " << m.first << "\n"; //showing the key
  }
  return result.str();
}

std::vector<uint8_t>
State::_pseudoRandomValue(uint64_t n)
{
    std::vector<uint8_t> result;
    for (int i = 0; i < 8; i++) {
        result.push_back(static_cast<uint8_t>(MurmurHash3(n+i, result) & 0xff));
    }
    return result;
}

} //namespace notificationLib
