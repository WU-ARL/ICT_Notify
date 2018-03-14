#include "event.hpp"
#include "logger.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/info_parser.hpp>


INIT_LOGGER(event);

namespace notificationLib {

Event::Event(const Name& name)
  : m_name(name)
{
}

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

  // Get event.prefix
  if (propertyIt == configSection.end() || !boost::iequals(propertyIt->first, "prefix")) {
    BOOST_THROW_EXCEPTION(Error("Expecting <event.prefix>"));
  }

  Name prefix = propertyIt->second.data();
  propertyIt++;

  auto event = make_unique<Event>(prefix);

  // Get event.filter
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
