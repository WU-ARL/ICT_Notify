/* -*- Mode:C++; c-file-style:"bsd"; indent-tabs-mode:nil; -*- */
/**
 * Copyright 2020 Washington University in St. Louis
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. 
 */

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

typedef std::unordered_map<uint64_t,std::vector<Name>> notificationList_t;

namespace StateType
{
  enum
  {
    IBF = 1,
    LIST = 2,
    TUPLE = 3
  };
}
class State : noncopyable
{
public:
  State(size_t maxNotificationMemory, int listType);

  uint64_t createKey(const std::vector<Name>& eventList);

  ConstBufferPtr getState() const;

  bool getDiff(ConstBufferPtr rmtStateStr,
               std::set<std::pair<uint64_t,std::vector<uint8_t> > >& inLocal,
               std::set<std::pair<uint64_t,std::vector<uint8_t> > >& inRemote) const;

  static bool
  isExpired(const uint64_t now,
            uint64_t timestamp,
            ndn::time::milliseconds max_freshness);

  void
  erase(const uint64_t timestamp);

  void
  cleanup(ndn::time::milliseconds max_freshness);


  bool reconcile(ConstBufferPtr newState,
                 NotificationData& data,
                 ndn::time::milliseconds max_freshness);

  std::vector<Name>& getEventsAtTimestamp(uint64_t timestamp);

  // for debugging
  std::string dumpItems() const;

  std::string dumpHistory() const;
  std::string dumpHistory(std::unordered_map<uint64_t,std::vector<Name>> history) const;

private:
  std::vector<uint8_t> _pseudoRandomValue(uint64_t n);

  void _addTimestamp(uint64_t timestamp, const std::vector<Name>& eventList, int partyIndex = 0);
  void _saveHistory(uint64_t timestamp, const std::vector<Name>&eventList);

  void _removeFromHistory(uint64_t timestamp);

  size_t m_maxNotificationMemory;
  // history containers
  IBFT m_ibft;
  //bool m_isList;
  int m_stateType;
  int m_localIndex;
  //std::unordered_map<uint64_t,shared_ptr<Data>> m_DataList;
  notificationList_t m_NotificationHistory;
  std::unordered_map<uint64_t,notificationList_t> m_NotificationTuple;
  //std::vector<std::unordered_map<<uint64_t,std::vector<Name>>> m_NotificationTuple;
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
