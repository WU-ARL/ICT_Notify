#include "notificationManager.hpp"
#include "logger.hpp"

INIT_LOGGER(logicManager);

namespace notificationLib {

NotificationProtocol::NotificationProtocol(ndn::Face& face,
                                           const Name& notificationName,
                                           size_t maxNotificationMemory,
                                           const NotificationAPICallback& onUpdate,
                                           const Name& defaultSigningId,
                                           std::shared_ptr<Validator> validator,
                                           const time::milliseconds& notificationInterestLifetime,
                                           const time::milliseconds& notificationReplyFreshness)
  : m_face(face)
  , m_notificationName(notificationName)
  , m_state(maxNotificationMemory)
  , m_onUpdate(onUpdate)
  , m_interestTable(m_face.getIoService())
  , m_outstandingInterestId(0)
  , m_scheduler(m_face.getIoService())
  //, m_randomGenerator(static_cast<unsigned int>(std::time(0)))
  , m_notificationInterestLifetime(notificationInterestLifetime)
  , m_notificationReplyFreshness(notificationReplyFreshness)
  , m_randomGenerator(std::random_device{}())
  //, m_reexpressionJitter(m_randomGenerator, boost::uniform_int<>(100,500))
  , m_reexpressionJitter(100,500)
  , m_signingId(defaultSigningId)
  , m_validator(validator)
{

}

NotificationProtocol::~NotificationProtocol()
{
  // clear local FIB entries towards producer
  for(const auto& itr : m_registeteredNotificationList) {
    if (static_cast<bool>(itr.second))
      m_face.unsetInterestFilter(itr.second);
  }
  m_face.shutdown();
  m_scheduler.cancelAllEvents();
  m_interestTable.clear();
}

void
NotificationProtocol::sendNotificationInterest()
{
  _LOG_DEBUG("NotificationProtocol::sendNotificationInterest");

  Name interestName(m_notificationName);
  ConstBufferPtr state  = m_state.getState();
  interestName.append(state->get<uint8_t>(), state->size());
  //interestName.appendVersion();
  m_outstandingInterestName = interestName;

  // scheduling the next interest so there is always a
  // long-lived interest packet in the PIT
  ndn::EventId eventId =
    m_scheduler.scheduleEvent(m_notificationInterestLifetime / 2 +
                              ndn::time::milliseconds(m_reexpressionJitter(m_randomGenerator)),
                              bind(&NotificationProtocol::sendNotificationInterest,
                                    this));

  m_scheduler.cancelEvent(m_scheduledInterestId);
  m_scheduledInterestId = eventId;

  Interest interest(interestName);
  interest.setMustBeFresh(true);
  interest.setInterestLifetime(m_notificationInterestLifetime);

  m_outstandingInterestId = m_face.expressInterest(interest,
                                   bind(&NotificationProtocol::onNotificationData, this, _1, _2),
                                   bind(&NotificationProtocol::onNotificationInterestNack, this, _1), // Nack
                                   bind(&NotificationProtocol::onNotificationInterestTimeout, this, _1));

  _LOG_INFO("NotificationProtocol::sendNotificationInterest: Sent interest: " << interest);
}

void
NotificationProtocol::onNotificationData(const Interest& interest,
                                         const Data& data)
{
  _LOG_INFO("NotificationProtocol::onNotificationData for interest: " << interest);
  NotificationData notificationData;

  // TBD: validate data

  // Remove satisfied interest from PIT
  m_interestTable.erase(interest.getName());
  std::cout << "full data name" << data.getName() << std::endl;
  ConstBufferPtr newStateComponentBuf = make_shared<ndn::Buffer>(data.getName().get(-1).value(),
                                                                 data.getName().get(-1).value_size());
  ConstBufferPtr oldStateComponentBuf = make_shared<ndn::Buffer>(data.getName().get(-2).value(),
                                                                 data.getName().get(-2).value_size());

  // for now, if our state is different then the old one - leave it
  ConstBufferPtr localStateName = m_state.getState();
  if(*oldStateComponentBuf != *localStateName)
  {
    // probably not an error, but print out so we know when it happens
    _LOG_ERROR("NotificationProtocol::onNotificationData old state is different then ours");
    return;
  }
  std::cout << " about to reconcile" << std::endl;
  // reconcile differences
  //m_state.reconcile(newStateComponent, oldStateComponent, notificationData);
  m_state.reconcile(newStateComponentBuf, notificationData);
  std::cout << " after  reconcile" << std::endl;
  //  send another interest only after update state with the new info
  if (m_outstandingInterestName == interest.getName()) {
    resetOutstandingInterest();
  }

  notificationData.wireDecode(data.getContent().blockFromValue());

  if(notificationData.m_type == NotificationData::dataType::EventsContainer)
  {
    // send to API callback
    _LOG_DEBUG("NotificationProtocol::onNotificationData notification type: EventsContainer");
    m_onUpdate(m_notificationName, notificationData.m_eventsObj.getEventList());
  }
  else if(notificationData.m_type == NotificationData::dataType::DataContainer)
  {
    // TBD - deal with data list
  }


}

void
NotificationProtocol::onNotificationInterestNack(const Interest& interest)
{
  _LOG_DEBUG("NotificationProtocol::onNotificationInterestNack");
}
void
NotificationProtocol::onNotificationInterestTimeout(const Interest& interest)
{
  _LOG_DEBUG("NotificationProtocol::onNotificationInterestTimeout");
}

void
NotificationProtocol::registerNotificationFilter(const Name& notificationNameFilter)
{
  _LOG_DEBUG("NotificationProtocol::registerNotificationFilter");

  // add notification name filter to registered notification list
  m_registeteredNotificationList[notificationNameFilter] = m_face.setInterestFilter(notificationNameFilter,
                           bind(&NotificationProtocol::onNotificationInterest,
                                this, _1, _2),
                           [] (const Name& prefix, const std::string& msg) {});
}


void
NotificationProtocol::onNotificationInterest(const Name& name, const Interest& interest)
{
  _LOG_INFO("NotificationProtocol::onNotificationInterest insert interest: "
             << interest);

  // TBD: if data is ready then push it now.
  // if not, save interest in interest table for future processing

  // Insert full name Interest with notification name
  m_interestTable.insert(interest, name);
}

void
NotificationProtocol::satisfyPendingNotificationInterests(const std::vector<Name>& eventList,
                                                          const ndn::time::milliseconds& freshness)
{
  Name notificationInterestName(m_notificationName);

  _LOG_DEBUG("NotificationProtocol::satisfyPendingEventInterests: "
             << "notificationPrefix is: " << notificationInterestName);

  if (eventList.empty())
  {
     _LOG_INFO("NotificationProtocol::satisfyPendingNotificationInterests: Notification List is empty, stopping.");
     return;
  }
  // first, create new key timestamp for the pushed events
  uint64_t newTimestampKey = m_state.update(eventList);

  _LOG_INFO("NotificationProtocol::satisfyPendingNotificationInterests:" << newTimestampKey);

  // get new status name component
  ConstBufferPtr newStatusName = m_state.getState();

  std::cout << "Test constructor" << std::endl;
  Name testName("/test/service/");
  testName.append(newStatusName->get<uint8_t>(),newStatusName->size());
  ConstBufferPtr fromNToB = make_shared<ndn::Buffer>(testName.get(-1).value(),
                                                                 testName.get(-1).value_size());
 auto fromBToDec = bzip2::decompress(fromNToB->get<char>(),
                                    fromNToB->size());

  IBFT test(fromBToDec->get<char>(), fromBToDec->size());

std::cout << "Test constructor end" << std::endl;
  try {
    // Go over all recorded requests and compute the set-difference
    // if can respond, reply with missing data
    auto it = m_interestTable.begin();
    while (it != m_interestTable.end()) {
      ConstUnsatisfiedInterestPtr request = *it;
      ++it;
      std::set<std::pair<uint64_t,std::vector<uint8_t> > > inLocal, inRemote;
      _LOG_INFO("NotificationProtocol::satisfyPendingNotificationInterests:  About to get diff");

      Name fullDataName(request->interest.getName());
      // get request state
      ConstBufferPtr rmtStatus = make_shared<ndn::Buffer>(fullDataName.get(-1).value(),
                                                          fullDataName.get(-1).value_size());

      fullDataName.append(newStatusName->get<uint8_t>(),newStatusName->size());
      std::cout << "full data name: " << fullDataName<< std::endl;
      // compute the set-difference
      if (m_state.getDiff(rmtStatus, inLocal, inRemote))
      {
        _LOG_INFO("NotificationProtocol::satisfyPendingNotificationInterests: list size is:" << inLocal.size());

        std::map<uint64_t,std::vector<Name>> listToPush;
        // send all new data (ignore removals for now. TBD)
        for(auto const& lit: inLocal)
        {
          listToPush[lit.first] = m_state.getEventsAtTimestamp(lit.first);
        }
        pushNotificationData(fullDataName, listToPush, freshness);

        // for now - ignore items we don't know
        // TBD: don't think we should remove here, only when recieving data
      }
    }
    m_interestTable.clear();
  }
  catch (const InterestTable::Error&) {
    // ok. not really an error
  }
}
void
NotificationProtocol::pushNotificationData(const Name& dataName,
                                          std::map<uint64_t,std::vector<Name>>& notificationList,
                                          const ndn::time::milliseconds& freshness)
{
  _LOG_DEBUG("NotificationProtocol::pushNotificationData named: " << dataName );
  _LOG_DEBUG("NotificationProtocol::pushNotificationData: about to send" << m_state.dumpHistory(notificationList) );

  NotificationData eventListData(notificationList);
  eventListData.setType(NotificationData::dataType::EventsContainer);

  // for(int i = 0; i < eventList.size(); ++i)
  // {
  //   _LOG_INFO("NotificationProtocol::pushNotificationData: Adding event" << eventList[i]);
  //   eventListData.addEvent(eventList[i]);
  // }
  shared_ptr<Data> data = make_shared<Data>();

  data->setContent(eventListData.wireEncode());
  //data->setFinalBlockId(dataName.get(-1));
  data->setFreshnessPeriod(freshness);
  data->setName(dataName);

  if (m_signingId.empty())
    m_keyChain.sign(*data);
  else
    m_keyChain.sign(*data, security::signingByIdentity(m_signingId));

  m_face.put(*data);

  _LOG_INFO("NotificationProtocol::pushNotificationData: sent data for name" << dataName);

  Name interestName = dataName.getPrefix(-1);
  if (m_outstandingInterestName == interestName) {
    resetOutstandingInterest();
  }
}

// void
// NotificationProtocol::pushNotificationData(const Name& dataName,
//                                            const std::vector<Name>& eventList,
//                                            const ndn::time::milliseconds& freshness)
// {
//   _LOG_INFO("NotificationProtocol::pushNotificationData named: " << dataName );
//
//   NotificationData eventListData;
//   eventListData.setType(NotificationData::dataType::EventsContainer);
//
//   _LOG_INFO("NotificationProtocol::event list size: " << eventList.size());
//   for(int i = 0; i < eventList.size(); ++i)
//   {
//     _LOG_INFO("NotificationProtocol::pushNotificationData: Adding event" << eventList[i]);
//     eventListData.addEvent(eventList[i]);
//   }
//   shared_ptr<Data> data = make_shared<Data>();
//
//   data->setContent(eventListData.wireEncode());
//   data->setFinalBlockId(dataName.get(-1));
//   data->setFreshnessPeriod(freshness);
//   data->setName(dataName);
//
//   if (m_signingId.empty())
//     m_keyChain.sign(*data);
//   else
//     m_keyChain.sign(*data, security::signingByIdentity(m_signingId));
//
//   m_face.put(*data);
//
//   _LOG_INFO("NotificationProtocol::pushNotificationData: sent data for name" << dataName);
//
//   Name interestName = dataName.getPrefix(-1);
//   if (m_outstandingInterestName == interestName) {
//     resetOutstandingInterest(interestName);
//   }
// }

// void
// NotificationProtocol::pushNotificationData(const Name& dataName,
//                                            std::set<std::pair<uint64_t,std::vector<uint8_t> > >& list,
//                                            const ndn::time::milliseconds& freshness)
// {
//   _LOG_INFO("NotificationProtocol::pushNotificationData2");
//   if(list.size() == 0)
//     return;
//
//   NotificationData eventListData;
//   eventListData.setType(NotificationData::dataType::EventsContainer);
//
//   std::set<std::pair<uint64_t,std::vector<uint8_t> > > :: iterator it; //iterator to manipulate set
//   for (it = list.begin(); it!=list.end(); it++)
//   {
//       uint64_t timestamp = it->first;
//       eventListData.addEvent(m_state.getEventsAtTimestamp(timestamp));
//   }
//
//   shared_ptr<Data> data = make_shared<Data>();
//
//   data->setContent(eventListData.wireEncode());
//   data->setFinalBlockId(dataName.get(-1));
//   data->setFreshnessPeriod(freshness);
//   data->setName(dataName);
//
//   if (m_signingId.empty())
//     m_keyChain.sign(*data);
//   else
//     m_keyChain.sign(*data, security::signingByIdentity(m_signingId));
//
//   m_face.put(*data);
//
//   _LOG_INFO("NotificationProtocol::pushNotificationData: sent data for name " << dataName);
//
//   Name interestName = dataName.getPrefix(-1);
//   if (m_outstandingInterestName == interestName) {
//     resetOutstandingInterest(interestName);
//   }
// }
void
NotificationProtocol::resetOutstandingInterest()
{
  // remove outstanding interest
  if (m_outstandingInterestId != 0) {
    m_face.removePendingInterest(m_outstandingInterestId);
    m_outstandingInterestId = 0;
  }

  //reschedule sending new notification interest
  time::milliseconds after(m_reexpressionJitter(m_randomGenerator));
  _LOG_DEBUG("Satisfy our own interest");
  _LOG_DEBUG("Reschedule notification interest after " << after);
  ndn::EventId eventId = m_scheduler.scheduleEvent(after,
                                              bind(&NotificationProtocol::sendNotificationInterest,
                                                   this));

  m_scheduler.cancelEvent(m_scheduledInterestId);
  m_scheduledInterestId = eventId;
}


void
NotificationProtocol::CreateNotificationData(const Name& dataName,
                                             const uint64_t timestampKey,
                                             const std::vector<Name>& eventList,
                                             const ndn::time::milliseconds& freshness)
{
  /*
  _LOG_DEBUG("NotificationProtocol::CreateNotificationData");

  NotificationData eventListData;
  eventListData.setType(NotificationData::dataType::EventsContainer);

  for(int i = 0; i < eventList.size(); ++i)
    eventListData.addEvent(eventList[i]);

  shared_ptr<Data> data = make_shared<Data>();

  data->setContent(eventListData.wireEncode());
  data->setFinalBlockId(dataName.get(-1));
  data->setName(dataName);

  if (m_signingId.empty())
    m_keyChain.sign(*data);
  else
    m_keyChain.sign(*data, security::signingByIdentity(m_signingId));

  //m_DataList.insert(std::make_pair(timestampKey, data));
  m_DataList[timestampKey] = data;

  m_NotificationHistory[timestampKey] = eventList;
  _LOG_DEBUG("NotificationProtocol::CreateNotificationData end");
  */
}

} //namespace notificationLib