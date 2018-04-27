#ifndef NOTIFICATIONLIB_STATE_CPP
#define NOTIFICATIONLIB_STATE_CPP

#include "common.hpp"
#include "ibft.hpp"
#include "notificationData.hpp"
#include <sstream>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <boost/iostreams/detail/iostream.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/copy.hpp>

#include <ndn-cxx/encoding/buffer-stream.hpp>

namespace notificationLib {

class State : noncopyable
{
public:
  State(size_t maxNotificationMemory);

  void addTimestamp(uint64_t timestamp, const std::vector<Name>& eventList);
  uint64_t update(const std::vector<Name>& eventList);

  //name::Component getState() const;
  ConstBufferPtr getState() const;

  // bool getDiff(name::Component& rmtStateStr,
  //             std::set<std::pair<uint64_t,std::vector<uint8_t> > >& inLocal,
  //             std::set<std::pair<uint64_t,std::vector<uint8_t> > >& inRemote) const;

  // bool getDiff(name::Component& local, name::Component& remote,
  //              std::set<std::pair<uint64_t,std::vector<uint8_t> > >& inLocal,
  //              std::set<std::pair<uint64_t,std::vector<uint8_t> > >& inRemote) const;

  bool getDiff(ConstBufferPtr rmtStateStr,
               std::set<std::pair<uint64_t,std::vector<uint8_t> > >& inLocal,
               std::set<std::pair<uint64_t,std::vector<uint8_t> > >& inRemote) const;
  // static bool getDiff(ConstBufferPtr local, ConstBufferPtr remote,
  //              std::set<std::pair<uint64_t,std::vector<uint8_t> > >& inLocal,
  //              std::set<std::pair<uint64_t,std::vector<uint8_t> > >& inRemote);

  //bool reconcile(name::Component& newState, NotificationData& data);
  bool reconcile(ConstBufferPtr newState, NotificationData& data);
  //bool reconcile(name::Component& newState, name::Component& oldState, NotificationData& data);
  std::vector<Name>& getEventsAtTimestamp(uint64_t timestamp);
  // for debugging
  std::string dumpItems() const;

  std::string dumpHistory() const;
  std::string dumpHistory(std::map<uint64_t,std::vector<Name>> history) const;

private:
  std::vector<uint8_t> _pseudoRandomValue(uint64_t n);

  void _saveHistory(uint64_t timestamp, const std::vector<Name>&eventList);

  size_t m_maxNotificationMemory;
  // history containers
  IBFT m_ibft;
  std::map<uint64_t,shared_ptr<Data>> m_DataList;
  std::map<uint64_t,std::vector<Name>> m_NotificationHistory;
};

// class Gzip {
// public:
// 	static std::string compress(const std::string& data)
// 	{
// 		namespace bio = boost::iostreams;
//
// 		std::stringstream compressed;
// 		std::stringstream origin(data);
//
// 		bio::filtering_streambuf<bio::input> out;
// 		out.push(bio::gzip_compressor(bio::gzip_params(bio::gzip::best_compression)));
// 		out.push(origin);
// 		bio::copy(out, compressed);
//
// 		return compressed.str();
// 	}
//
// 	static std::string decompress(const std::string& data)
// 	{
// 		namespace bio = boost::iostreams;
//
// 		std::stringstream compressed(data);
// 		std::stringstream decompressed;
//
// 		bio::filtering_streambuf<bio::input> out;
// 		out.push(bio::gzip_decompressor());
// 		out.push(compressed);
// 		bio::copy(out, decompressed);
//
// 		return decompressed.str();
// 	}
// };
class bzip2 {

public:
  static std::shared_ptr<ndn::Buffer>
  compress(const char* buffer, size_t bufferSize)
  {
    namespace bio = boost::iostreams;

    ndn::OBufferStream os;
    bio::filtering_streambuf<bio::output> out;
    out.push(bio::bzip2_compressor());
    out.push(os);
    bio::stream<bio::array_source> in(reinterpret_cast<const char*>(buffer), bufferSize);
    bio::copy(in, out);
    return os.buf();
  }

  static std::shared_ptr<ndn::Buffer>
  decompress(const char* buffer, size_t bufferSize)
  {
    namespace bio = boost::iostreams;
    ndn::OBufferStream os;
    bio::filtering_streambuf<bio::output> out;
    out.push(bio::bzip2_decompressor());
    out.push(os);
    bio::stream<bio::array_source> in(reinterpret_cast<const char*>(buffer), bufferSize);
    bio::copy(in, out);
    return os.buf();
  }
};

} // namespace notificationLib

#endif // NOTIFICATIONLIB_STATE_CPP