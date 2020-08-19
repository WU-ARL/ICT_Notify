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

#include "event.hpp"
#include "logger.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/info_parser.hpp>


INIT_LOGGER(event);

namespace notificationLib {

Event::Event()
{
}
// Event::Event(const Name& name)
//   : m_name(name)
// {
// }

void
Event::addFilter(unique_ptr<Filter> filter)
{
  m_filters.push_back(std::move(filter));
}

bool
Event::match(const Name& eventName) const
{

  _LOG_DEBUG("Trying to match " << eventName);

  if (m_filters.empty()) {
    return true;
  }

  bool retval = false;
  for (const auto& filter : m_filters) {
    retval |= filter->match(tlv::Data, eventName);
    if (retval) {
      break;
    }
  }
  return retval;
}

unique_ptr<Event>
Event::create(const ConfigSection& configSection, const std::string& configFilename)
{
  auto propertyIt = configSection.begin();

  auto event = make_unique<Event>();

  // Get event.filter (all filters)
  for (; propertyIt != configSection.end(); propertyIt++) {
    if (!boost::iequals(propertyIt->first, "filter")) {
      BOOST_THROW_EXCEPTION(Error("Expecting <event.filter> in Event"));
    }
    event->addFilter(Filter::create(propertyIt->second, configFilename));
  }

  // Check other stuff
  if (propertyIt != configSection.end()) {
    BOOST_THROW_EXCEPTION(Error("Expecting the end of event"));
  }

  return event;
}


} //namespace notificationLib
