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
