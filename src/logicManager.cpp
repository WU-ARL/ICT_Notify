#include "logicManager.hpp"
#include "logger.hpp"

INIT_LOGGER(logicManager);

namespace notificationLib {

NotificationProtocol::NotificationProtocol(ndn::Face& face,
                                           //const Name& notificationPrefix,
                                           //const NotificationCallback& onUpdate,
                                           const Name& defaultSigningId,
                                           std::shared_ptr<Validator> validator,
                                           const time::milliseconds& eventInterestLifetime,
                                           const time::milliseconds& eventReplyFreshness)
  : m_face(face)
  //, m_eventPrefix(notificationPrefix)
  //, m_onUpdate(onUpdate)
  , m_signingId(defaultSigningId)
  , m_scheduler(m_face.getIoService())
  , m_interestTable(m_face.getIoService())
  , m_randomGenerator(static_cast<unsigned int>(std::time(0)))
  , m_reexpressionJitter(m_randomGenerator, boost::uniform_int<>(100,500))
  , m_eventInterestLifetime(eventInterestLifetime)
  , m_eventReplyFreshness(eventReplyFreshness)
  , m_validator(validator)
{

}

NotificationProtocol::~NotificationProtocol()
{
  for(const auto& itr : m_registeteredEventsList) {
    if (static_cast<bool>(itr.second))
      m_face.unsetInterestFilter(itr.second);
  }
  m_scheduler.cancelAllEvents();
  m_interestTable.clear();
  m_face.shutdown();
}

void
NotificationProtocol::sendEventInterest(const Name& eventPrefix,
                                        const NotificationCallback& onUpdate)
{
  _LOG_DEBUG("NotificationProtocol::sendEventInterest");

  Name interestName;
  interestName.append(eventPrefix);

  //m_outstandingInterestName = interestName;

  // scheduling the next interest so there is always a
  // long-lived interest packet in the PIT
  ndn::EventId eventId =
    m_scheduler.scheduleEvent(m_eventInterestLifetime / 2 +
                              ndn::time::milliseconds(m_reexpressionJitter()),
                              bind(&NotificationProtocol::sendEventInterest,
                                    this, eventPrefix, onUpdate));


  //m_scheduler.cancelEvent(m_reexpressingInterestId);
  //m_reexpressingInterestId = eventId;

  Interest interest(interestName);
  interest.setMustBeFresh(true);
  interest.setInterestLifetime(m_eventInterestLifetime);

  //m_outstandingInterestId = m_face.expressInterest(interest,
  m_face.expressInterest(interest,
                         bind(&NotificationProtocol::onEventData, this, _1, _2, onUpdate),
                         bind(&NotificationProtocol::onEventTimeout, this, _1), // Nack
                         bind(&NotificationProtocol::onEventTimeout, this, _1));

  _LOG_INFO("NotificationProtocol::sendEventInterest: Sent interest: " << interest.getName());
}

void
NotificationProtocol::onEventData(const Interest& interest,
                                  const Data& data,
                                  const NotificationCallback& onUpdate)
{
  _LOG_INFO("NotificationProtocol::onEventData for interest: " << interest);
  NotificationData reply;

  // validate data

  // decode data
  reply.wireDecode(data.getContent().blockFromValue());

  if(reply.empty())
  {
    _LOG_INFO("received data is empty!!");
    return;
  }

  for (unsigned i=0; i< reply.m_notificationList.size(); i++)
  {
    _LOG_DEBUG("NotificationProtocol::onEventData: Received event: "
               << reply.m_notificationList.at(i));
  }
  // send to application callback
  onUpdate(reply.m_notificationList);

}
void
NotificationProtocol::onEventDataValidationFailed(const Data& data)
{
  _LOG_DEBUG("NotificationProtocol::onEventDataValidationFailed");
}

void
NotificationProtocol::onEventDataValidated(const Data& data, bool firstData)
{
  _LOG_DEBUG("NotificationProtocol::onEventDataValidated");
  /*Name name = data.getName();
  ConstBufferPtr digest = make_shared<ndn::Buffer>(name.get(-1).value(), name.get(-1).value_size());

  processSyncData(name, digest, data.getContent().blockFromValue(), firstData);*/
}

void
NotificationProtocol::processEventData(const Name& name,
                       const Block& evenyReplyBlock)
{
  _LOG_DEBUG("NotificationProtocol::processEventData");

  try {
    // Remove satisfied interest from PIT
    m_interestTable.erase(name);

    // construct data consisting of all event names
  }
  catch (const Error&) {
    _LOG_DEBUG("Decoding failed");
    return;
  }
}

void
NotificationProtocol::onEventNack(const Interest& interest)
{
  _LOG_DEBUG("NotificationProtocol::onEventNack");
}
void
NotificationProtocol::onEventTimeout(const Interest& interest)
{
  _LOG_DEBUG("NotificationProtocol::onEventTimeout");
}

void
NotificationProtocol::registerEventPrefix(const Name& eventPrefix)
{
  // add event and interest filter to registered events list
  m_registeteredEventsList[eventPrefix] = m_face.setInterestFilter(eventPrefix,
                           bind(&NotificationProtocol::onEventInterest,
                                this, _1, _2),
                           [] (const Name& prefix, const std::string& msg) {});
}


void
NotificationProtocol::onEventInterest(const Name& prefix, const Interest& interest)
{
  _LOG_DEBUG("NotificationProtocol::onEventInterest");

  // TBD: if data is ready then push it now.
  // if not, save interest in interest table for future processing
  _LOG_DEBUG("NotificationProtocol::onEventInterest insert interest: "
             << interest << "and prefix: " << prefix);

  m_interestTable.insert(interest, prefix);
}

void
NotificationProtocol::satisfyPendingEventInterests(const Name& notificationPrefix,
                                                   const NotificationData& notificationList,
                                                   const ndn::time::milliseconds& freshness)
{
  _LOG_DEBUG("NotificationProtocol::satisfyPendingEventInterests: "
             << "notificationPrefix is: " << notificationPrefix);
  try {
    bool doesExist = m_interestTable.has(notificationPrefix);
    if (doesExist)
    {
      _LOG_DEBUG("NotificationProtocol::found notification prefix");
      sendEventData(notificationPrefix, notificationList, freshness);
      m_interestTable.erase(notificationPrefix);
    }
      //if (request->isUnknown)
    //    sendSyncData(updatedPrefix, request->interest.getName(), m_state);
  //    else

  }
  catch (const InterestTable::Error&) {
    // ok. not really an error
  }
}
void
NotificationProtocol::sendEventData(const Name& eventPrefix,
                                    const NotificationData& notificationList,
                                    const ndn::time::milliseconds& freshness)
{
  _LOG_DEBUG("NotificationProtocol::sendEventData");

  if (notificationList.empty())
  {
    _LOG_INFO("NotificationProtocol::sendEventData: Notification List is empty, stopping.");
    return;
  }
  shared_ptr<Data> data = make_shared<Data>();
  /*std::vector<uint8_t> buffer;
  size_t bufferLength = 0;
  const uint8_t * bufferPtr = reinterpret_cast<const uint8_t*>(notificationList.data());

  for(ndn::Name name : notificationList) {
    _LOG_DEBUG("adding: " << name << " of size: " << name.size());
    bufferLength+=name.size();
  }

  buffer.insert(buffer.end(),bufferPtr, bufferPtr + bufferLength);
  //std::copy(notificationList.begin(), notificationList.end(), buffer);
  //data->setContent(buffer.data(), static_cast<size_t>(nCharsRead));

  _LOG_DEBUG("NotificationProtocol::sendEventData: About to set content"
             << buffer.data() << " of size: " << bufferLength);*/
  data->setContent(notificationList.wireEncode());
  data->setFinalBlockId(eventPrefix.get(-1));
  data->setFreshnessPeriod(freshness);
  data->setName(eventPrefix);

  if (m_signingId.empty())
    m_keyChain.sign(*data);
  else
    m_keyChain.sign(*data, security::signingByIdentity(m_signingId));

  m_face.put(*data);

  _LOG_DEBUG("NotificationProtocol::sendEventData end");
}

} //namespace notificationLib
