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

#include "interest-table.hpp"

namespace notificationLib {

InterestTable::InterestTable(boost::asio::io_service& io)
  : m_scheduler(io)
{
}

InterestTable::~InterestTable()
{
  clear();
}

void
InterestTable::insert(const Interest& interest, const Name& name)
{
  erase(name);

  auto request = make_shared<UnsatisfiedInterest>(interest, name);

  time::milliseconds entryLifetime = interest.getInterestLifetime();
  if (entryLifetime < time::milliseconds::zero())
    entryLifetime = ndn::DEFAULT_INTEREST_LIFETIME;

  request->expirationEvent = m_scheduler.schedule(entryLifetime, [=] { erase(name); });
  //request->expirationEvent = m_scheduler.scheduleEvent(entryLifetime, [=] { erase(name); });

  m_table.insert(request);
}

void
InterestTable::erase(const Name& name)
{
  auto it = m_table.get<hashed>().find(name);
  while (it != m_table.get<hashed>().end()) {
    (*it)->expirationEvent.cancel();
    //m_scheduler.cancelEvent((*it)->expirationEvent);
    m_table.erase(it);

    it = m_table.get<hashed>().find(name);
  }
}

bool
InterestTable::has(const Name& name)
{

  if (m_table.get<hashed>().find(name) != m_table.get<hashed>().end())
    return true;
  else
    return false;

}

size_t
InterestTable::size() const
{
  return m_table.size();
}

void
InterestTable::clear()
{
  for (const auto& item : m_table) {
    item->expirationEvent.cancel();
    //m_scheduler.cancelEvent(item->expirationEvent);
  }

  m_table.clear();
}

} // namespace notificationLib
