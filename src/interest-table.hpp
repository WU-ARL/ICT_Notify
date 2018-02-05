#ifndef NOTIFICATIONLIB_INTEREST_TABLE_HPP
#define NOTIFICATIONLIB_INTEREST_TABLE_HPP

#include "interest-container.hpp"

namespace notificationLib {

/**
 * @brief A table to keep unsatisfied Sync Interest
 */
class InterestTable : noncopyable
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

  using iterator = InterestContainer::iterator;
  using const_iterator = InterestContainer::const_iterator;

  explicit
  InterestTable(boost::asio::io_service& io);

  ~InterestTable();

  void
  insert(const Interest& interest, const Name& name);

  bool
  has(const Name& name);

  void
  erase(const Name& name);

  const_iterator
  begin() const
  {
    return m_table.begin();
  }

  iterator
  begin()
  {
    return m_table.begin();
  }

  const_iterator
  end() const
  {
    return m_table.end();
  }

  iterator
  end()
  {
    return m_table.end();
  }

  size_t
  size() const;

  void
  clear();

private:
  ndn::Scheduler m_scheduler;
  InterestContainer m_table;
};

} // namespace notificationLib

#endif // NOTIFICATIONLIB_INTEREST_TABLE_HPP
