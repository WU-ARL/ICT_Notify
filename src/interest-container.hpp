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

#ifndef NOTIFICATIONLIB_INTEREST_CONTAINER_HPP
#define NOTIFICATIONLIB_INTEREST_CONTAINER_HPP

#include "common.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <ndn-cxx/util/sha256.hpp>

namespace notificationLib {


// Multi-Index-Container Tags
struct hashed
{
};

struct ordered
{
};

struct sequenced
{
};

struct timed
{
};

namespace mi = boost::multi_index;

struct NameHash
{
  std::size_t
  operator()(const Name& prefix) const
  {
    ConstBufferPtr buffer =
      ndn::util::Sha256::computeDigest(prefix.wireEncode().wire(), prefix.wireEncode().size());

    BOOST_ASSERT(buffer->size() > sizeof(std::size_t));

    return *reinterpret_cast<const std::size_t*>(buffer->data());
  }
};

struct NameEqual
{
  bool
  operator()(const Name& prefix1, const Name& prefix2) const
  {
    return prefix1 == prefix2;
  }
};

class UnsatisfiedInterest : noncopyable
{
public:
  UnsatisfiedInterest(const Interest& interest,
                      const Name& name);

  const Name&
  getName() const
  {
    return name;
  }
public:
  const Interest interest;
  const Name name;
  ndn::scheduler::EventId   expirationEvent;
};

using UnsatisfiedInterestPtr = shared_ptr<UnsatisfiedInterest>;
using ConstUnsatisfiedInterestPtr = shared_ptr<const UnsatisfiedInterest>;


struct InterestContainer : public mi::multi_index_container<
  UnsatisfiedInterestPtr,
  mi::indexed_by<
    mi::hashed_unique<
      mi::tag<hashed>,
      mi::const_mem_fun<UnsatisfiedInterest, const Name&, &UnsatisfiedInterest::getName>,
      NameHash,
      NameEqual
      >
    >
  >
{
};
} // namespace notificationLib

#endif // NOTIFICATIONLIB_INTEREST_CONTAINER_HPP
