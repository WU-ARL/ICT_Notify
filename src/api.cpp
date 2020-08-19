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

#include "api.hpp"
#include "logger.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/info_parser.hpp>


INIT_LOGGER(api);

namespace notificationLib {

const ndn::Name api::EMPTY_NAME;
const ndn::Name api::DEFAULT_NAME;
//const NotificationData api::EMPTY_EVENT_LIST;
const std::shared_ptr<Validator> api::DEFAULT_VALIDATOR;
const time::milliseconds api::DEFAULT_EVENT_INTEREST_LIFETIME(3000);
const time::milliseconds api::DEFAULT_EVENT_FRESHNESS(4);

std::unordered_map<uint64_t,int> api::m_DataNameSizeCollector;
std::unordered_map<uint64_t,int> api::m_InterestNameSizeCollector;

api::api(ndn::Face& face,
         const Name& signingId,
         std::shared_ptr<Validator> validator)
  : m_face(face)
  // , m_notificationProtocol(face,
  //                          signingId,
  //                          validator,
  //                          api::DEFAULT_EVENT_INTEREST_LIFETIME,
  //                          api::DEFAULT_EVENT_FRESHNESS)
   , m_signingId(signingId)
  , m_validator(validator)
{

}

api::~api()
{
  /*for(const auto& itr : m_registeredEventsList) {
    if (static_cast<bool>(itr.second))
      m_face.unsetInterestFilter(itr.second);
  }*/
}

void
api::init(const std::string& filename,
          const NotificationAppCallback& notificationCB)
          //const NotificationAppCallback& notificationCB)
{
  // get list of names and rulse from configuration file
  std::ifstream inputFile;
  inputFile.open(filename.c_str());
  if (!inputFile.good() || !inputFile.is_open()) {
    std::string msg = "Failed to read configuration file: ";
    msg += filename;
    BOOST_THROW_EXCEPTION(Error(msg));
  }
  loadConfigurationFile(inputFile, filename);
  inputFile.close();

  // TBD:: read notificationCB from file so every notification
  // can have its own callback
  m_onNotificationAppCB = notificationCB;
  // for each event
  for (unsigned i=0; i< m_notificationList.size(); i++)
  {
    // schedule long-lived interests for each notificatoin prefix
    unique_ptr<Notification>& notification = m_notificationList[i];

    if(notification->getIsListener())
      (notification->m_notificationProtocol).sendNotificationInterest();
      // (notification->m_notificationProtocol).sendNotificationInterest(
      //                                       std::bind(&api::onNotificationUpdate,
      //                                                 this,
      //                                                 notification->getName(),
      //                                                 _1));

   if(notification->getIsProvider())
      (notification->m_notificationProtocol).registerNotificationFilter(notification->getName());


    // m_notificationProtocol.sendNotificationInterest((m_notificationList.at(i))->getName(),
    //                                         std::bind(&api::onNotificationUpdate, this, m_notificationList[i] , _1));
  }
}
void
api::loadConfigurationFile(std::istream& input, const std::string& filename)
{
  ConfigSection configSection;
  try {
    boost::property_tree::read_info(input, configSection);
  }
  catch (const boost::property_tree::info_parser_error& error) {
    std::stringstream msg;
    msg << "Failed to parse configuration file";
    msg << " " << filename;
    msg << " " << error.message() << " line " << error.line();
    BOOST_THROW_EXCEPTION(Error(msg.str()));
  }

  // make sure the file is not empty
  BOOST_ASSERT(!filename.empty());
  if (configSection.begin() == configSection.end()) {
    std::string msg = "Error processing configuration file";
    msg += ": ";
    msg += filename;
    msg += " no data";
    BOOST_THROW_EXCEPTION(Error(msg));
  }
  for (const auto& subSection : configSection) {
    const std::string& sectionName = subSection.first;
    const ConfigSection& section = subSection.second;

    if (boost::iequals(sectionName, "notification")) {
      auto notification = Notification::create(section,
                              filename,
                              m_face,
                              std::bind(&api::onNotificationUpdate, this, _1, _2));
      m_notificationList.push_back(std::move(notification));

    }
    else {
      std::string msg = "Error processing configuration file";
      msg += " ";
      msg += filename;
      msg += " unrecognized section: " + sectionName;
      BOOST_THROW_EXCEPTION(Error(msg));
    }
  }
}

// removed from API
// register prefix for notifications (producer side)
// void
// api::registerNotificationPrefix(const Name& notificationName)
// {
  // fix
  //m_notificationProtocol.registerNotificationFilter(notificationName);


  /* Moved to logicManager
  // add event and interest filter to registered events list
  m_registeredEventsList[eventPrefix] = m_face.setInterestFilter(eventPrefix,
                           bind(&api::onInterest, this, _1, _2),
                           [] (const Name& prefix, const std::string& msg) {});
   */
// }

// notify a notification (producer side)
void
api::notify(const Name& notificationName,
       const std::vector<Name>& eventList,
       const ndn::time::milliseconds& freshness)
{
  // get notification pointer by name
  int index = getNotificationIndex(notificationName);
  if(index >= m_notificationList.size()) {
      _LOG_ERROR("api::notify ERROR, no notification name: " << notificationName);
      return;
  }

  unique_ptr<Notification>& notification = m_notificationList[index];

  (notification->m_notificationProtocol).satisfyPendingNotificationInterests(eventList, freshness);

  // fix
  //m_notificationProtocol.satisfyPendingNotificationInterests(notificationName, eventList, freshness);
  //m_ims.insert(*data);
  //ndn::shared_ptr<ndn::Data> data = ndn::make_shared<ndn::Data>();
}

void api::onNotificationUpdate (const Name& notificationName,
                                const std::unordered_map<uint64_t,std::vector<Name>>& notificationList)
{
  _LOG_DEBUG("api::onNotificationUpdate");
  std::unordered_map<uint64_t,std::vector<Name>> matchedNotifications;

  auto now_ns = boost::chrono::time_point_cast<boost::chrono::nanoseconds>(ndn::time::system_clock::now());
  auto now_ns_long_type = (now_ns.time_since_epoch()).count();

  // Get notification by its name
  int index = getNotificationIndex(notificationName);
  if(index >= m_notificationList.size()) {
      _LOG_ERROR("api::onNotificationUpdate ERROR, no notification name: " << notificationName);
      return;
  }
  unique_ptr<Notification>& notification = m_notificationList[index];
  const std::vector<unique_ptr<Event>>& eventList = notification->getEventList();

  for(auto const& iList: notificationList)
  {
    const uint64_t timestamp = iList.first;

    _LOG_DEBUG("api::onNotificationUpdate checking events for timestamp: " << timestamp);
    const std::vector<Name>& eventsAtTimestamp = iList.second;
    std::vector<Name> matchedNames;

    for (size_t i = 0; i < eventsAtTimestamp.size(); i++)
    {
      Name eventName = eventsAtTimestamp[i];

      _LOG_DEBUG("api::onNotificationUpdate checking event name: " << eventName);

      auto it = std::find_if(eventList.begin(), eventList.end(),
                            [&eventName](const unique_ptr<Event>& event)
                            {return (event->match(eventName));});
      if (it != eventList.end())
      {
        _LOG_DEBUG("api::onNotificationUpdate matched event: " << eventName);

        // check if event expired
        // how long past
        auto tPassed = now_ns_long_type - timestamp;

        // if still relevant (convert ms to ns)
        auto freshnessInNano =  (notification->m_notificationProtocol).getFreshnessInNanoSeconds();
        if(tPassed <= freshnessInNano)
        {
          _LOG_DEBUG("api::onNotificationUpdate is still fresh: " << eventName);
          matchedNames.push_back(eventName);
        }
        else
          _LOG_DEBUG("api::onNotificationUpdate is NOT fresh: " << eventName);
     }
    }
    if(!matchedNames.empty())
    {
      matchedNotifications[timestamp] = matchedNames;
      m_onNotificationAppCB(now_ns_long_type, matchedNotifications);

    }
  }

// notify applications without timestamps
#if 0
  _LOG_DEBUG("api::onNotificationUpdate");

  std::vector<Name> matchedNames;
  std::vector<Name> incomingEventList;

  /// for now  - get all the names into ine vector regarsless of timestamp
  incomingEventList.reserve(notificationList.size());
  for(auto const& imap: notificationList)
    incomingEventList.insert(incomingEventList.end(),
                             imap.second.begin(),
                             imap.second.end());

  // Get notification by its name
  int index = getNotificationIndex(notificationName);
  if(index >= m_notificationList.size()) {
      _LOG_ERROR("api::onNotificationUpdate ERROR, no notification name: " << notificationName);
      return;
  }
  unique_ptr<Notification>& notification = m_notificationList[index];

  //const std::vector<unique_ptr<Event>>& eventList = (m_notificationList[notificationIndex])->getEventList();
  const std::vector<unique_ptr<Event>>& eventList = notification->getEventList();
  for (size_t i = 0; i < incomingEventList.size(); i++) {
    Name eventName = incomingEventList[i];
    auto it = std::find_if(eventList.begin(), eventList.end(),
                          [&eventName](const unique_ptr<Event>& event)
                          {return (event->match(eventName));});
    if (it != eventList.end()){
      _LOG_INFO("api::onNotificationUpdate event: " << incomingEventList[i]);
      matchedNames.push_back(incomingEventList[i]);
    }
  }
    if(!matchedNames.empty())
      m_onNotificationAppCB(matchedNames);
#endif
}

int api::getNotificationIndex(const Name& notificationName)
{
  auto it = std::find_if(m_notificationList.begin(), m_notificationList.end(),
                        [&notificationName](const unique_ptr<Notification>& notification)
                        {return (notification->getName() == notificationName);});

  return(std::distance(m_notificationList.begin(),it));
}

void
api::collectNameSize(int type, uint32_t number)
{
  auto now_ns = boost::chrono::time_point_cast<boost::chrono::nanoseconds>(ndn::time::system_clock::now());
  auto now_ns_long_type = (now_ns.time_since_epoch()).count();

  // get time
  if(type == 1)
    m_InterestNameSizeCollector[now_ns_long_type] = number;
  else if(type == 2)
    m_DataNameSizeCollector[now_ns_long_type] = number;

}
}
