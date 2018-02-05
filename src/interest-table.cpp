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

  request->expirationEvent = m_scheduler.scheduleEvent(entryLifetime, [=] { erase(name); });

  m_table.insert(request);
}

void
InterestTable::erase(const Name& name)
{
  auto it = m_table.get<hashed>().find(name);
  while (it != m_table.get<hashed>().end()) {
    m_scheduler.cancelEvent((*it)->expirationEvent);
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
    m_scheduler.cancelEvent(item->expirationEvent);
  }

  m_table.clear();
}

} // namespace notificationLib
