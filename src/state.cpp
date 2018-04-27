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

  _saveHistory(now_ns_long_type, eventList);
  return now_ns_long_type;
}

// name::Component
// State::getState() const
// {
//   std::string str;
//   str.assign(reinterpret_cast<char*>(m_ibft.getIBFBuffer()), m_ibft.getIBFSize());
//
//   //name::Component compressedNameComponent(Gzip::compress(str));
//   auto contentBuffer = bzip2::compress(reinterpret_cast<const char*>(m_ibft.getIBFBuffer()),
//                                           m_ibft.getIBFSize());
//
//   name::Component compressedNameComponent(contentBuffer);
//   return compressedNameComponent;
// }

ConstBufferPtr
State::getState() const
{

  auto contentBuffer = bzip2::compress(reinterpret_cast<const char*>(m_ibft.getIBFBuffer()),
                                          m_ibft.getIBFSize());

  return contentBuffer;
}

// bool State::getDiff(name::Component& rmtStateStr,
//                    std::set<std::pair<uint64_t,std::vector<uint8_t> > >& inLocal,
//                    std::set<std::pair<uint64_t,std::vector<uint8_t> > >& inRemote) const
// {
//   // name::Component localStateName = getState();
//   // return (getDiff(localStateName, rmtStateStr, inLocal, inRemote));
//
//   // Decomposing name component, DO NOT  use toUri()!
//   // std::string rmtStr(reinterpret_cast<const char*>(rmtStateStr.value()),
//   //                    rmtStateStr.value_size());
//   //
//   // IBFT remoteIBF(Gzip::decompress(rmtStr), 4);
//
//   auto remoteBuf = bzip2::decompress(reinterpret_cast<const char*>(rmtStateStr.value()),
//                                                 rmtStateStr.value_size());
//
//   std::cout << " get remote IBF" << std::endl;
//   IBFT remoteIBF(remoteBuf->get<char>(), remoteBuf->size(), 4);
//
//
//   IBFT diff = m_ibft-remoteIBF;
//   return (diff.listEntries(inLocal, inRemote));
//
// }
// bool State::getDiff(name::Component& local, name::Component& remote,
//                    std::set<std::pair<uint64_t,std::vector<uint8_t> > >& inLocal,
//                    std::set<std::pair<uint64_t,std::vector<uint8_t> > >& inRemote) const
// {
//   std::cout << " get strings" << std::endl;
//   std::cout << " local: " << local << std::endl;
//   std::cout << " remote: " << remote << std::endl;
//
//   // // Decomposing name component, DO NOT  use toUri()!
//   // std::string rmtStr(reinterpret_cast<const char*>(remote.value()),
//   //                    remote.value_size());
//   //
//   // std::string localStr(reinterpret_cast<const char*>(local.value()),
//   //                      local.value_size());
//
//   // std::cout << " rmt string: " << rmtStr << std::endl;
//   // std::cout << " local string: " << localStr << std::endl;
//   // std::cout << " local string before string: " << local << std::endl;
//   // std::cout << " get IBFs" << std::endl;
//   //
//   //   IBFT remoteIBF(Gzip::decompress(rmtStr), 4);
//   //   IBFT localIBF(Gzip::decompress(localStr), 4);
//   //
//   // std::cout << " about to minus" << std::endl;
//   //   IBFT diff = localIBF-remoteIBF;
//   //   std::cout << " after minus" << std::endl;
//   //   return (diff.listEntries(inLocal, inRemote));
//   //
//
//   auto localBuf = bzip2::decompress(reinterpret_cast<const char*>(local.value()),
//                                     local.value_size());
//
//   auto remoteBuf = bzip2::decompress(reinterpret_cast<const char*>(remote.value()),
//                                                 remote.value_size());
//
//   std::cout << " get remote IBF" << std::endl;
//   IBFT remoteIBF(remoteBuf->get<char>(), remoteBuf->size(), 4);
//
//   std::cout << " get local IBF" << std::endl;
//   IBFT localIBF(localBuf->get<char>(), localBuf->size(), 4);
//
//   IBFT diff = localIBF-remoteIBF;
//
//   std::cout << " after minus" << std::endl;
//   return (diff.listEntries(inLocal, inRemote));
// }

bool State::getDiff(ConstBufferPtr rmtStateStr,
                   std::set<std::pair<uint64_t,std::vector<uint8_t> > >& inLocal,
                   std::set<std::pair<uint64_t,std::vector<uint8_t> > >& inRemote) const
{
  // name::Component localStateName = getState();
  // return (getDiff(localStateName, rmtStateStr, inLocal, inRemote));

  // Decomposing name component, DO NOT  use toUri()!
  // std::string rmtStr(reinterpret_cast<const char*>(rmtStateStr.value()),
  //                    rmtStateStr.value_size());
  //
  // IBFT remoteIBF(Gzip::decompress(rmtStr), 4);

  auto remoteBuf = bzip2::decompress(rmtStateStr->get<char>(),
                                     rmtStateStr->size());

  std::cout << " get remote IBF" << std::endl;
  IBFT remoteIBF(remoteBuf->get<char>(), remoteBuf->size(), 4);


  IBFT diff = m_ibft-remoteIBF;
  return (diff.listEntries(inLocal, inRemote));

}

// bool State::getDiff(ConstBufferPtr local, ConstBufferPtr remote,
//                    std::set<std::pair<uint64_t,std::vector<uint8_t> > >& inLocal,
//                    std::set<std::pair<uint64_t,std::vector<uint8_t> > >& inRemote)
// {
//   std::cout << " get strings" << std::endl;
//   std::cout << " local: " << local << std::endl;
//   std::cout << " remote: " << remote << std::endl;
//
//   auto localBuf = bzip2::decompress(local->get<char>(),
//                                     local->size());
//
//   auto remoteBuf = bzip2::decompress(remote->get<char>(),
//                                       remote->size());
//
//   std::cout << " get remote IBF" << std::endl;
//   IBFT remoteIBF(remoteBuf->get<char>(), remoteBuf->size(), 4);
//
//   std::cout << " get local IBF" << std::endl;
//   IBFT localIBF(localBuf->get<char>(), localBuf->size(), 4);
//
//   IBFT diff = localIBF-remoteIBF;
//
//   std::cout << " after minus" << std::endl;
//   return (diff.listEntries(inLocal, inRemote));
// }
// bool
// State::reconcile(name::Component& newState, NotificationData& data)
// {
//   name::Component localStateName = getState();
//   return(reconcile(localStateName, newState, data));
//}
bool
State::reconcile(ConstBufferPtr newState, NotificationData& data)
{
  std::set<std::pair<uint64_t,std::vector<uint8_t> > > inNew, inOld;
  //ConstBufferPtr oldState  = getState();
  std::cout << " about to getDiff" << std::endl;
  if (getDiff(newState, inOld, inNew))
  {
    std::cout << " after to getDiff" << std::endl;
    // for now, only add new timestamps to local IBF and History
    for(auto const& newit: inNew)
    {
      std::cout << " Done reconciling - return true" << std::endl;
      addTimestamp(newit.first, data.m_eventsObj.getEventList(newit.first));
      //listToPush[lit.first] = m_state.getEventsAtTimestamp(lit.first);
    }
    // TBD - handle removals
    std::cout << " Done reconcile - return true" << std::endl;
    return true;
  }
  else
    return false;
}

// bool
// State::reconcile(name::Component& newState, name::Component& oldState, NotificationData& data)
// {
//   std::set<std::pair<uint64_t,std::vector<uint8_t> > > inNew, inOld;
//
//   std::cout << " about to getDiff" << std::endl;
//   if (getDiff(newState, oldState, inNew, inOld))
//   {
//     std::cout << " after to getDiff" << std::endl;
//     // for now, only add new timestamps to local IBF and History
//     for(auto const& newit: inNew)
//     {
//       std::cout << " Done reconcile - return true" << std::endl;
//       addTimestamp(newit.first, data.m_eventsObj.getEventList(newit.first));
//       //listToPush[lit.first] = m_state.getEventsAtTimestamp(lit.first);
//     }
//     // TBD - handle removals
//     std::cout << " Done reconcile - return true" << std::endl;
//     return true;
//   }
//   else
//     return false;
// }

std::vector<Name>&
State::getEventsAtTimestamp(uint64_t timestamp)
{
  return m_NotificationHistory[timestamp];
}

void
State::_saveHistory(uint64_t timestamp, const std::vector<Name>&eventList)
{
  _LOG_DEBUG("State::_saveHistory");
  for(auto const& eit: eventList)
    std::cout << "in inList: " << eit << std::endl;
  m_NotificationHistory[timestamp] = eventList;
  std::cout << "in hostory: " << dumpHistory() << std::endl;

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
// std::vector<char> unzip(const std::vector<char> compressed)
// {
//    std::vector<char> decompressed = std::vector<char>();
//
//    boost::iostreams::filtering_ostream os;
//
//    os.push(boost::iostreams::gzip_decompressor());
//    os.push(boost::iostreams::back_inserter(decompressed));
//
//    boost::iostreams::write(os, &compressed[0], compressed.size());
//
//    return decompressed;
// }

} //namespace notificationLib
