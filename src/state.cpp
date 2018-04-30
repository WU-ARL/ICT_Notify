#include "state.hpp"
#include "logger.hpp"
#include "murmurhash3.hpp"
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

INIT_LOGGER(state);

namespace notificationLib {

State::State(size_t maxNotificationMemory)
  : m_maxNotificationMemory(maxNotificationMemory)
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

ConstBufferPtr
State::getState() const
{
  Block ibfBlock = m_ibft.wireEncode();
  // auto contentBuffer = bzip2::compress(reinterpret_cast<const char*>(m_ibft.getIBFBuffer()),
  //                                         m_ibft.getIBFSize());

  auto contentBuffer = bzip2::compress(reinterpret_cast<const char*>(ibfBlock.wire()),
                                       ibfBlock.size());

  return contentBuffer;
}

bool State::getDiff(ConstBufferPtr rmtStateStr,
                   std::set<std::pair<uint64_t,std::vector<uint8_t> > >& inLocal,
                   std::set<std::pair<uint64_t,std::vector<uint8_t> > >& inRemote) const
{
  auto remoteBuf = bzip2::decompress(rmtStateStr->get<char>(),
                                     rmtStateStr->size());


  //IBFT remoteIBF(remoteBuf->get<char>(), remoteBuf->size(), 4);
  IBFT remoteIBF(remoteBuf, m_maxNotificationMemory, 4);

  // std::cout << "My IBF" << m_ibft.dumpItems() << std::endl;
  // std::cout << "Remote" << remoteIBF.dumpItems() << std::endl;

  IBFT diff = m_ibft-remoteIBF;
  return (diff.listEntries(inLocal, inRemote));
}

bool
State::reconcile(ConstBufferPtr newState, NotificationData& data)
{
  std::set<std::pair<uint64_t,std::vector<uint8_t> > > inNew, inOld;
  //ConstBufferPtr oldState  = getState();
  if (getDiff(newState, inOld, inNew))
  {
    // for now, only add new timestamps to local IBF and History
    for(auto const& newit: inNew)
    {
      addTimestamp(newit.first, data.m_eventsObj.getEventList(newit.first));
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
State::_saveHistory(uint64_t timestamp, const std::vector<Name>&eventList)
{
  _LOG_DEBUG("State::_saveHistory");
  for(auto const& eit: eventList)

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
