#include "api.hpp"
#include "logger.hpp"

INIT_LOGGER(api);

namespace notificationLib {

const ndn::Name api::DEFAULT_NAME;
const ndn::Name api::DEFAULT_PREFIX;
const std::shared_ptr<Validator> api::DEFAULT_VALIDATOR;

api::api(const Name& notificationPrefix,
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
}

api::~api()
{
  for(const auto& itr : m_registeredPrefixList) {
    if (static_cast<bool>(itr.second))
      m_face.unsetInterestFilter(itr.second);
  }
  m_ims.erase("/");
}

void
api::addNotificationAlert(const Name& prefix, const Name& signingId)
{

}

void
api::removeNotificationAlert(const Name& prefix)
{
  if (prefix == DEFAULT_NAME)
    return;

  auto itr = m_registeredPrefixList.find(prefix);
  if (itr != m_registeredPrefixList.end()) {
    if (static_cast<bool>(itr->second))
      m_face.unsetInterestFilter(itr->second);
    m_registeredPrefixList.erase(itr);
  }

  m_ims.erase(prefix);
}

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

  m_ims.insert(*data);

}

void
api::onInterest(const Name& prefix, const Interest& interest)
{
  shared_ptr<const Data>data = m_ims.find(interest);
  if (static_cast<bool>(data)) {
    m_face.put(*data);
  }
}

void
api::onData(const Interest& interest, const Data& data,
               const DataValidatedCallback& onValidated,
               const DataValidationErrorCallback& onFailed)
{
  _LOG_DEBUG("api::onData");

  if (static_cast<bool>(m_validator))
    m_validator->validate(data, onValidated, onFailed);
  else
    onValidated(data);
}

void
api::onDataTimeout(const Interest& interest, int nRetries,
                      const DataValidatedCallback& onValidated,
                      const DataValidationErrorCallback& onFailed)
{
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
}

void
api::onDataValidationFailed(const Data& data,
                               const ValidationError& error)
{
}

} // namespace notificationLib
