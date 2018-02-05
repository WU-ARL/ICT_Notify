#ifndef NOTIFICATIONLIB_API_CPP
#define NOTIFICATIONLIB_API_CPP

#include <unordered_map>
#include <ndn-cxx/ims/in-memory-storage-persistent.hpp>
#include "common.hpp"
#include "logicManager.hpp"

namespace notificationLib {

/**
 * A simple interface for application.
 *
 * This interface allows an application to declare a prefix and a set of rulse
 * it wants to be notified on.
 */
class api : noncopyable
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

  api(ndn::Face& face,
      const Name& signingId = DEFAULT_NAME,
      std::shared_ptr<Validator> validator = DEFAULT_VALIDATOR);

  ~api();

  void
  subscribe(const Name& eventPrefix,
            const NotificationCallback& notificationCB);

  void
  registerEventPrefix(const Name& eventPrefix);

  void
  notify(const Name& prefix = EMPTY_NAME,
         const NotificationData& eventList = EMPTY_EVENT_LIST,
         const ndn::time::milliseconds& freshness = DEFAULT_EVENT_FRESHNESS);

  // Define Callbacks
  using DataValidatedCallback = function<void(const Data&)>;

  using DataValidationErrorCallback = function<void(const Data&, const ValidationError& error)> ;

private:
  void
  onInterest(const Name& prefix, const Interest& interest);

  void
  onData(const Interest& interest, const Data& data,
         const DataValidatedCallback& dataCallback,
         const DataValidationErrorCallback& failCallback);

  void
  onDataTimeout(const Interest& interest, int nRetries,
                const DataValidatedCallback& dataCallback,
                const DataValidationErrorCallback& failCallback);

  void
  onDataValidationFailed(const Data& data,
                         const ValidationError& error);

public:
  static const ndn::Name EMPTY_NAME;
  static const ndn::Name DEFAULT_NAME;
  static const NotificationData EMPTY_EVENT_LIST;
  static const std::shared_ptr<Validator> DEFAULT_VALIDATOR;
  static const time::milliseconds DEFAULT_EVENT_FRESHNESS;
  static const time::milliseconds DEFAULT_EVENT_INTEREST_LIFETIME;

private:

  std::unordered_map<ndn::Name, const ndn::RegisteredPrefixId*> m_registeteredEventsList;
  std::vector<ndn::Name> m_subscriptionList;
  NotificationProtocol m_notificationProtocol;
  //NotificationCallback m_onNotification;
  ndn::Face& m_face;
  ndn::InMemoryStoragePersistent m_inMemoryStorage;

  Name m_signingId;
  ndn::KeyChain m_keyChain;
  std::shared_ptr<Validator> m_validator;

};

} // namespace notificationLib

#endif // NOTIFICATIONLIB_API_CPP
