#include "api.hpp"
#include "logger.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/info_parser.hpp>


INIT_LOGGER(api);

namespace notificationLib {

const ndn::Name api::EMPTY_NAME;
const ndn::Name api::DEFAULT_NAME;
const NotificationData api::EMPTY_EVENT_LIST;
const std::shared_ptr<Validator> api::DEFAULT_VALIDATOR;
const time::milliseconds api::DEFAULT_EVENT_INTEREST_LIFETIME(10000);
const time::milliseconds api::DEFAULT_EVENT_FRESHNESS(1);

/*api::api(const Name& notificationPrefix,
       ndn::Face& face,
       const NotificationCallback& notificationCB,
       const Name& signingId,
       std::shared_ptr<Validator> validator)
  : m_notificationPrefix(notificationPrefix)
  , m_face(face)
  , m_onNotification(notificationCB)
  , m_signingId(signingId)
  , m_validator(validator)
{
  if (m_notificationPrefix != DEFAULT_PREFIX)
    m_registeredPrefixList[m_notificationPrefix] =
      m_face.setInterestFilter(m_notificationPrefix,
                               bind(&api::onInterest, this, _1, _2),
                               [] (const Name& prefix, const std::string& msg) {});
}*/

api::api(ndn::Face& face,
         const Name& signingId,
         std::shared_ptr<Validator> validator)
  : m_face(face)
  , m_notificationProtocol(face,
                           signingId,
                           validator,
                           api::DEFAULT_EVENT_INTEREST_LIFETIME,
                           api::DEFAULT_EVENT_FRESHNESS)
  , m_signingId(signingId)
  , m_validator(validator)
{
}

api::~api()
{
  /*for(const auto& itr : m_registeteredEventsList) {
    if (static_cast<bool>(itr.second))
      m_face.unsetInterestFilter(itr.second);
  }*/
}

//Deprecated
void
api::subscribe(const Name& eventPrefix, //Filter
               const NotificationCallback& notificationCB)
{
  if (eventPrefix.empty())
    return;

  // add notification prefix to subscription list (list of pairs with filters??)
  m_subscriptionList.push_back(eventPrefix);

  // schedule long-lived interests
  m_notificationProtocol.sendEventInterest(eventPrefix, notificationCB);
                                           //bind(&notificationCB, this, _1));
}

void
api::subscribe(const std::string& filename,
          const NotificationCallback& notificationCB)
{
  // get list of names and rulse from configuration file
  std::ifstream inputFile;
  inputFile.open(filename.c_str());
  if (!inputFile.good() || !inputFile.is_open()) {
    std::string msg = "Failed to read configuration file: ";
    msg += filename;
    BOOST_THROW_EXCEPTION(Error(msg));
  }
  //loadEventsConfigurationFile(inputFile, filename);
  loadEventsConfigurationFile(inputFile, filename);
  inputFile.close();

  m_onNotificationAppCB = notificationCB;
  // for each event
  for (unsigned i=0; i< m_eventList.size(); i++)
  {
    // schedule long-lived interests for each event prefix
    m_notificationProtocol.sendEventInterest((m_eventList.at(i))->getName(),
                                            std::bind(&api::onNotificationUpdate, this, _1));
    //m_notificationProtocol.sendEventInterest((m_eventList.at(i))->getName(), notificationCB);
  }
    // add notification prefix to subscription list (list of pairs with filters??)
    //m_subscriptionList.push_back(eventPrefix);
}
void
api::loadEventsConfigurationFile(std::istream& input, const std::string& filename)
{
  ConfigSection configSection;
  try {
    boost::property_tree::read_info(input, configSection);
  }
  catch (const boost::property_tree::info_parser_error& error) {
    std::stringstream msg;
    msg << "Failed to parse configuration file";
    msg << " " << filename;
    msg << " " << error.message() << " line " << error.line();
    BOOST_THROW_EXCEPTION(Error(msg.str()));
  }

  // make sure the file is not empty
  BOOST_ASSERT(!filename.empty());
  if (configSection.begin() == configSection.end()) {
    std::string msg = "Error processing configuration file";
    msg += ": ";
    msg += filename;
    msg += " no data";
    BOOST_THROW_EXCEPTION(Error(msg));
  }

  for (const auto& subSection : configSection) {
    const std::string& sectionName = subSection.first;
    const ConfigSection& section = subSection.second;

    if (boost::iequals(sectionName, "event")) {
      auto event = Event::create(section, filename);
      m_eventList.push_back(std::move(event));
    }
    else {
      std::string msg = "Error processing configuration file";
      msg += " ";
      msg += filename;
      msg += " unrecognized section: " + sectionName;
      BOOST_THROW_EXCEPTION(Error(msg));
    }
  }

}


void
api::registerEventPrefix(const Name& eventPrefix)
{
  m_notificationProtocol.registerEventPrefix(eventPrefix);
  /* Moved to logicManager
  // add event and interest filter to registered events list
  m_registeteredEventsList[eventPrefix] = m_face.setInterestFilter(eventPrefix,
                           bind(&api::onInterest, this, _1, _2),
                           [] (const Name& prefix, const std::string& msg) {});
   */
}


void
api::notify(const Name& prefix,
       const NotificationData& notificationList,
       const ndn::time::milliseconds& freshness)
{
  m_notificationProtocol.satisfyPendingEventInterests(prefix, notificationList, freshness);
  //m_ims.insert(*data);
  //ndn::shared_ptr<ndn::Data> data = ndn::make_shared<ndn::Data>();
}

/*
void
api::publishData(const Block& content, const ndn::time::milliseconds& freshness,
                 const Name& prefix)
{
  shared_ptr<Data> data = make_shared<Data>();
  data->setContent(content);
  data->setFreshnessPeriod(freshness);
  data->setName(prefix);

  if (m_signingId.empty())
    m_keyChain.sign(*data);
  else
    m_keyChain.sign(*data, security::signingByIdentity(m_signingId));

  m_ims.insert(*data);*

}
*/
void api::onNotificationUpdate (const std::vector<Name>& nameList)
{
  _LOG_DEBUG("api::onNotificationUpdate");

  std::vector<Name> matchedNames;
  for (size_t i = 0; i < nameList.size(); i++) {
    Name name = nameList[i];
    auto it = std::find_if(m_eventList.begin(), m_eventList.end(),
                          [&name](const unique_ptr<Event>& event)
                          {return (event->match(name));});
    if (it != m_eventList.end()){
      matchedNames.push_back(nameList[i]);
    }

    // for (size_t j = 0; j < m_eventList.size(); j++) {
    //   if(m_eventList[j]->match(nameList[i])){
    //      _LOG_DEBUG("api::onNotificationUpdate MATCH!!: " << nameList[i]);
    //      matchedNames.push_back(nameList[i]);
    //    }
    // }
  }
    if(!matchedNames.empty())
      m_onNotificationAppCB(matchedNames);
}

void
api::onInterest(const Name& prefix, const Interest& interest)
{
  /*
  shared_ptr<const Data>data = m_ims.find(interest);
  if (static_cast<bool>(data)) {
    m_face.put(*data);
  }*/
}

void
api::onData(const Interest& interest, const Data& data,
               const DataValidatedCallback& onValidated,
               const DataValidationErrorCallback& onFailed)
{
  /*
  _LOG_DEBUG("api::onData");

  if (static_cast<bool>(m_validator))
    m_validator->validate(data, onValidated, onFailed);
  else
    onValidated(data);
    */
}

void
api::onDataTimeout(const Interest& interest, int nRetries,
                      const DataValidatedCallback& onValidated,
                      const DataValidationErrorCallback& onFailed)
{
  /*
  _LOG_DEBUG("api::onDataTimeout");
  if (nRetries <= 0)
    return;

  Interest newNonceInterest(interest);
  newNonceInterest.refreshNonce();

  m_face.expressInterest(newNonceInterest,
                         bind(&api::onData, this, _1, _2, onValidated, onFailed),
                         bind(&api::onDataTimeout, this, _1, nRetries - 1,
                              onValidated, onFailed), // Nack
                         bind(&api::onDataTimeout, this, _1, nRetries - 1,
                              onValidated, onFailed));
                              */
}

void
api::onDataValidationFailed(const Data& data,
                               const ValidationError& error)
{
}

} // namespace notificationLib
