#ifndef NOTIFICATIONLIB_API_CPP
#define NOTIFICATIONLIB_API_CPP

#include <unordered_map>
#include <ndn-cxx/ims/in-memory-storage-persistent.hpp>
#include "common.hpp"
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

  using NotificationCallback = function<void(const std::vector<Name>&)>;

  api(const Name& notificationPrefix,
         ndn::Face& face,
         const NotificationCallback& notificationCB,
         const Name& signingId = DEFAULT_NAME,
         std::shared_ptr<Validator> validator = DEFAULT_VALIDATOR);

  ~api();

  using DataValidatedCallback = function<void(const Data&)>;

  using DataValidationErrorCallback = function<void(const Data&, const ValidationError& error)> ;

  void
  addNotificationAlert(const Name& prefix, const Name& signingId = DEFAULT_NAME);

  void
  removeNotificationAlert(const Name& prefix);

  void
  publishData(const Block& content, const ndn::time::milliseconds& freshness,
              const Name& prefix = DEFAULT_PREFIX);

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
  static const ndn::Name DEFAULT_NAME;
  static const ndn::Name DEFAULT_PREFIX;
  static const std::shared_ptr<Validator> DEFAULT_VALIDATOR;

private:
  using RegisteredPrefixList = std::unordered_map<ndn::Name, const ndn::RegisteredPrefixId*>;

  Name m_notificationPrefix;
  NotificationCallback m_onNotification;
  ndn::Face& m_face;

  Name m_signingId;
  ndn::KeyChain m_keyChain;
  std::shared_ptr<Validator> m_validator;

  RegisteredPrefixList m_registeredPrefixList;
  ndn::InMemoryStoragePersistent m_ims;
};

} // namespace notificationLib

#endif // NOTIFICATIONLIB_API_CPP
