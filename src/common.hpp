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

#ifndef NOTIFICATIONLIB_COMMON_HPP
#define NOTIFICATIONLIB_COMMON_HPP

#include "config.hpp"

#include <cstddef>
#include <list>
#include <queue>
#include <set>
#include <vector>
#include <tuple>

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/security/v2/validator.hpp>
#include <ndn-cxx/security/v2/validator-config/filter.hpp>
#include <ndn-cxx/security/validator-config.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/util/time.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/chrono.hpp>

namespace notificationLib {

using std::size_t;

using boost::noncopyable;
using boost::scoped_ptr;
typedef boost::property_tree::ptree ConfigSection;

using std::bind;
using std::const_pointer_cast;
using std::cref;
using std::dynamic_pointer_cast;
using std::enable_shared_from_this;
using std::function;
using std::make_shared;
using std::make_tuple;
using std::ref;
using std::shared_ptr;
using std::static_pointer_cast;
using std::weak_ptr;
using std::unique_ptr;

using ndn::Block;
using ndn::ConstBufferPtr;
using ndn::Data;
using ndn::Exclude;
using ndn::Interest;
using ndn::Name;
using ndn::security::v2::ValidationError;
using ndn::security::v2::Validator;
using ndn::security::v2::validator_config::Filter;
using ndn::EncodingImpl;
using ndn::EncodingEstimator;
using ndn::EncodingBuffer;
using ndn::make_unique;


//using NotificationAppCallback = function<void(const std::vector<Name>&)>;
using NotificationAppCallback = function<void(uint64_t receivedTime, const std::unordered_map<uint64_t,std::vector<Name>>&)>;
using NotificationAPICallback = function<void(const Name&, const std::unordered_map<uint64_t,std::vector<Name>>&)>;

namespace tlv {
using namespace ndn::tlv;
} // namespace tlv

namespace name = ndn::name;
namespace time = ndn::time;
namespace security = ndn::security;
namespace encoding = ndn::encoding;


class Error : public std::runtime_error
{
public:
  explicit
  Error(const std::string& what)
    : std::runtime_error(what)
  {
  }
};

} // namespace NotificationLib

#endif // NOTIFICATIONLIB_COMMON_HPP
