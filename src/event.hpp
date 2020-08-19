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

#ifndef NOTIFICATIONLIB_EVENT_CPP
#define NOTIFICATIONLIB_EVENT_CPP

#include <unordered_map>
#include <ndn-cxx/ims/in-memory-storage-persistent.hpp>
#include <ndn-cxx/security/v2/validator-config/filter.hpp>
#include "common.hpp"


namespace notificationLib {

class Event : noncopyable
{
public:
  Event();
  //Event(const Name& name);

  // const Name&
  // getName() const
  // {
  //   return m_name;
  // }

  void
  addFilter(unique_ptr<Filter> filter);

  bool
  match(const Name& eventName) const;

  static unique_ptr<Event>
  create(const ConfigSection& configSection, const std::string& configFilename);

  //Name m_name;
  std::vector<unique_ptr<Filter>> m_filters;
};
} // namespace notificationLib

#endif // NOTIFICATIONLIB_EVENT_CPP
