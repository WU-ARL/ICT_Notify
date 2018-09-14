#include "notification.hpp"
#include "logger.hpp"
#include "api.hpp"

INIT_LOGGER(notification);

namespace notificationLib {

Notification::Notification(const Name& name,
                          size_t maxNotificationMemory,
                          time::milliseconds memoryFreshness,
                          time::milliseconds lifetime,
                          bool isListener,
                          bool isProvider,
                          int stateType,
                          ndn::Face& face,
                          NotificationAPICallback notificationCB)
  : m_notificationName(name)
  , m_isListener(isListener)
  , m_isProvider(isProvider)
  , m_face(face)
  , m_notificationProtocol(face,
                           name,
                           maxNotificationMemory,
                           memoryFreshness,
                           lifetime,
                           stateType,
                           notificationCB,
                           api::DEFAULT_NAME,
                           api::DEFAULT_VALIDATOR,
                           api::DEFAULT_EVENT_FRESHNESS)
{
}
void
Notification::addEvent(unique_ptr<Event> event)
{
  m_eventList.push_back(std::move(event));
}

unique_ptr<Notification>
Notification::create(const ConfigSection& configSection,
                     const std::string& configFilename,
                     ndn::Face& face,
                     const NotificationAPICallback& notificationCB)
{
  auto propertyIt = configSection.begin();

  // Get notification.name
  if (propertyIt == configSection.end() || !boost::iequals(propertyIt->first, "name")) {
    BOOST_THROW_EXCEPTION(Error("Expecting <notification.name>"));
  }
  Name name = propertyIt->second.data();
  std::cout << name << std::endl;
  propertyIt++;

  // Get notification.maxMemorySize
  if (propertyIt == configSection.end() || !boost::iequals(propertyIt->first, "maxMemorySize")) {
    BOOST_THROW_EXCEPTION(Error("Expecting <notification.maxMemorySize>"));
  }

  size_t maxNotificationMemory = std::stoi(propertyIt->second.data());
  propertyIt++;

  // Get notification.memoryFreshness
  if (propertyIt == configSection.end() || !boost::iequals(propertyIt->first, "memoryFreshness")) {
    BOOST_THROW_EXCEPTION(Error("Expecting <notification.memoryFreshness>"));
  }
  // converting number to ms.
  size_t memoryFreshness = std::stoi(propertyIt->second.data()) *1000;
  propertyIt++;

  // Get notification.lifetime
  if (propertyIt == configSection.end() || !boost::iequals(propertyIt->first, "lifetime")) {
    BOOST_THROW_EXCEPTION(Error("Expecting <notification.lifetime>"));
  }
  // converting number to ms.
  size_t lifetime = std::stoi(propertyIt->second.data()) *1000;
  propertyIt++;


  // Get notification.isListener
  bool isListener = false;
  if (propertyIt == configSection.end() || !boost::iequals(propertyIt->first, "isListener")) {
    BOOST_THROW_EXCEPTION(Error("Expecting <notification.isListener>"));
  }

  if(propertyIt->second.data() == "true")
    isListener = true;
  propertyIt++;

  // Get notification.isProvider
  bool isProvider = false;
  if (propertyIt == configSection.end() || !boost::iequals(propertyIt->first, "isProvider")) {
    BOOST_THROW_EXCEPTION(Error("Expecting <notification.isProvider>"));
  }

  if(propertyIt->second.data() == "true")
    isProvider = true;
  propertyIt++;

  if (!isListener && !isProvider)
    BOOST_THROW_EXCEPTION(Error("isListener and isProvider are both false, expecting at least one to be true"));

  // Get notification.stateType
  int stateType = StateType::LIST;
  if (propertyIt == configSection.end() || !boost::iequals(propertyIt->first, "stateType")) {
    BOOST_THROW_EXCEPTION(Error("Expecting <notification.stateType>"));
  }

  if(propertyIt->second.data() == "IBF")
    stateType = StateType::IBF;
  else if(propertyIt->second.data() == "TUPLE")
    stateType = StateType::TUPLE;
  else if(propertyIt->second.data() == "LIST")
    stateType = StateType::LIST;
  else
    BOOST_THROW_EXCEPTION(Error("Expecting IBF, LIST or TUPLE for <notification.stateType>"));

  propertyIt++;

  auto notification = make_unique<Notification>(name,
                                                maxNotificationMemory,
                                                time::milliseconds(memoryFreshness),
                                                time::milliseconds(lifetime),
                                                isListener,
                                                isProvider,
                                                stateType,
                                                face,
                                                notificationCB);

  if (isListener)
  {
    // make sure event exists
    if (propertyIt == configSection.end() || !boost::iequals(propertyIt->first, "event")) {
      BOOST_THROW_EXCEPTION(Error("Expecting <notification.event>"));
    }
    // create event object
    notification->addEvent(Event::create(propertyIt->second, configFilename));
    propertyIt++;
  }

  // // Check other stuff
  // if (propertyIt != configSection.end()) {
  //   BOOST_THROW_EXCEPTION(Error("Expecting the end of notification, instead got " + propertyIt->first));
  // }

  return notification;
}
} // namespace notificationLib
