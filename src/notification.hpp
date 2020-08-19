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

#ifndef NOTIFICATIONLIB_NOTIFICATION_CPP
#define NOTIFICATIONLIB_NOTIFICATION_CPP

#include "common.hpp"
#include "ibft.hpp"
#include "event.hpp"
#include "notificationManager.hpp"

// Notification class
// each notification consists of a prefix, an IBF
// and a data packet after this was received

namespace notificationLib {

class Notification : noncopyable
{
public:
  Notification(const Name& name,
               size_t maxNotificationMemory,
               const time::milliseconds memoryFreshness,
               const time::milliseconds lifetime,
               bool isListener,
               bool isProvider,
               int stateType,
               ndn::Face& face,
               NotificationAPICallback notificationCB);

  static unique_ptr<Notification>
  create(const ConfigSection& configSection,
         const std::string& configFilename,
         ndn::Face& face,
         const NotificationAPICallback& notificationCB);

  void
  addEvent(unique_ptr<Event> event);

  const Name&
  getName() const
  {
    return m_notificationName;
  }
  bool
  getIsListener() const
  {
    return m_isListener;
  }
  bool
  getIsProvider() const
  {
    return m_isProvider;
  }

  const std::vector<unique_ptr<Event>>&
  getEventList() const
  {
    return m_eventList;
  }

  NotificationProtocol m_notificationProtocol;

private:
  Name m_notificationName;
  bool m_isListener;
  bool m_isProvider;
  ndn::Face& m_face;
  std::vector<unique_ptr<Event>> m_eventList; // holding list though one might be enout

};
} // namespace notificationLib

#endif // NOTIFICATIONLIB_EVENT_CPP
