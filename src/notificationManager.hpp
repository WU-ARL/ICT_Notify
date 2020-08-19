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

#ifndef LOGIC_MANAGER_HPP
#define LOGIC_MANAGER_HPP

#include "common.hpp"
#include "interest-table.hpp"
#include "notificationData.hpp"
#include "state.hpp"
//#include <boost/random.hpp>
#include <random>

#include <boost/archive/iterators/dataflow_exception.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/assert.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/throw_exception.hpp>

#include <memory>
#include <random>
#include <unordered_map>


namespace notificationLib
{
  //using NotificationCallback = function<void(const std::vector<Name>&)>;

  class NotificationProtocol : noncopyable
  {
  public:
    class Error : public std::runtime_error
    {
    public:
      explicit
      Error(const std::string& what)
        : std::runtime_error(what)
      {
      }
    };

    NotificationProtocol(ndn::Face& face,
                         const Name& notificationName,
                         size_t maxNotificationMemory,
                         const time::milliseconds& notificationMemoryFreshness,
                         const time::milliseconds& eventInterestLifetime,
                         int listType,
                         //const Name& notificationPrefix,
                         const NotificationAPICallback& onUpdate,
                         const Name& defaultSigningId,
                         std::shared_ptr<Validator> validator,
                         const time::milliseconds& eventReplyFreshness);
    ~NotificationProtocol();

    void
    sendNotificationInterest();

    void
    registerNotificationFilter(const Name& notificationNameFilter);

    void
    satisfyPendingNotificationInterests(const std::vector<Name>& eventList,
                                        const ndn::time::milliseconds& freshness);
  public:
    ndn::Scheduler&
    getScheduler()
    {
      return m_scheduler;
    }

    uint64_t getFreshnessInNanoSeconds()
    {
      return m_notificationMemoryFreshness.count()*1000000;
    }

  private:
    void
    onNotificationInterest(const Name& name, const Interest& interest);

    void
    onNotificationData(const Interest& interest, const Data& data);

    void
    onNotificationInterestNack(const Interest& interest);

    void
    onNotificationInterestTimeout(const Interest& interest);

    // void
    // onEventDataValidationFailed(const Data& data);

    // void
    // onEventDataValidated(const Data& data, bool firstData);

    // void
    // processEventData(const Name& name,
    //                  const Block& EventReplyBlock);

    // void
    // pushNotificationData(const Name& dataName,
    //                      const std::vector<Name>& eventList,
    //                      const ndn::time::milliseconds& freshness);
    int
    sendDiff(const Name& interestName,
             const ndn::time::milliseconds freshness = ndn::time::milliseconds(-1));

    void
    pushNotificationData(const Name& dataName,
                          std::unordered_map<uint64_t,std::vector<Name>>& notificationList,
                          const ndn::time::milliseconds& freshness);
    // void
    // pushNotificationData(const Name& dataName,
    //                      std::set<std::pair<uint64_t,std::vector<uint8_t> > >& list,
    //                      const ndn::time::milliseconds& freshness);

    // void
    // CreateNotificationData(const Name& dataName,
    //                        const uint64_t timestampKey,
    //                        const std::vector<Name>& notificationList,
    //                        const ndn::time::milliseconds& freshness);

     void
     resetOutstandingInterest();

  public:
    /*
    static const ndn::Name DEFAULT_NAME;
    static const ndn::Name EMPTY_NAME;
    static const std::shared_ptr<Validator> DEFAULT_VALIDATOR;
*/
  private:

    // Communication
    ndn::Face& m_face;
    Name m_notificationName;
    std::unordered_map<ndn::Name, ndn::RegisteredPrefixHandle> m_registeredNotificationList;
    //std::unordered_map<ndn::Name, const ndn::RegisteredPrefixId*> m_registeredNotificationList;
    InterestTable m_interestTable;
    ndn::scheduler::Scheduler m_scheduler;
    ndn::scheduler::EventId m_scheduledInterestId;

    // State
    State m_state;
    Name m_outstandingInterestName;
    ndn::PendingInterestHandle m_outstandingInterestId;
    //const ndn::PendingInterestId* m_outstandingInterestId;
    NotificationAPICallback m_onUpdate;
    std::vector<ConstBufferPtr> m_stateHistory;

    // Timer
    time::milliseconds m_notificationInterestLifetime;
    time::milliseconds m_notificationMemoryFreshness;
    time::milliseconds m_notificationReplyFreshness;
    std::mt19937 m_randomGenerator;
    std::uniform_int_distribution<> m_reexpressionJitter;
    //boost::mt19937 m_randomGenerator;
    //boost::variate_generator<boost::mt19937&, boost::uniform_int<> > m_reexpressionJitter;

    // Security
    Name m_signingId;
    ndn::KeyChain m_keyChain;
    std::shared_ptr<Validator> m_validator;
  };


} // notificationLib

#endif //LOGIC_MANAGER_HPP
