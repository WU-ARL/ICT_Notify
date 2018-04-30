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
               bool isListener,
               bool isProvider,
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
