#ifndef NOTIFICATIONLIB_API_CPP
#define NOTIFICATIONLIB_API_CPP

#include <unordered_map>
#include <ndn-cxx/ims/in-memory-storage-persistent.hpp>
#include "common.hpp"
#include "event.hpp"
#include "notification.hpp"

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

  api(ndn::Face& face,
      const Name& signingId = DEFAULT_NAME,
      std::shared_ptr<Validator> validator = DEFAULT_VALIDATOR);

  ~api();

  // subscribe using an input file
  void
  init(const std::string& filename,
       const NotificationAppCallback& notificationCB);
       //const NotificationAppCallback& notificationCB);

  // void
  // registerNotificationPrefix(const Name& notificationPrefix);

  void
  notify(const Name& notificationName = EMPTY_NAME,
         const std::vector<Name>& eventList = EMPTY_EVENT_LIST,
         const ndn::time::milliseconds& freshness = DEFAULT_EVENT_FRESHNESS);

  // Define Callbacks
  // using DataValidatedCallback = function<void(const Data&)>;
  //
  // using DataValidationErrorCallback = function<void(const Data&, const ValidationError& error)> ;

  // for collecting
  static std::unordered_map<uint64_t,int> m_DataNameSizeCollector;
  static std::unordered_map<uint64_t,int> m_InterestNameSizeCollector;

  static void
  collectNameSize(int type, uint32_t number); // type: 1-interest, 2- Data

private:


  void
  onNotificationUpdate (const Name& notificationName,
                        const std::unordered_map<uint64_t,std::vector<Name>>& notificationList);

  void
  loadConfigurationFile(std::istream& input, const std::string& filename);

  int
  getNotificationIndex(const Name& notificationName);

  // for collecting
  static
  void reserveCollectors();

public:
  static const ndn::Name EMPTY_NAME;
  static const ndn::Name DEFAULT_NAME;
  static const std::vector<Name> EMPTY_EVENT_LIST;
  static const std::shared_ptr<Validator> DEFAULT_VALIDATOR;
  static const time::milliseconds DEFAULT_EVENT_INTEREST_LIFETIME;
  static const time::milliseconds DEFAULT_EVENT_FRESHNESS;

private:
  std::vector<unique_ptr<Notification>> m_notificationList;
  //NotificationProtocol m_notificationProtocol;
  NotificationAppCallback m_onNotificationAppCB;
  ndn::Face& m_face;
  Name m_signingId;
  ndn::KeyChain m_keyChain;
  std::shared_ptr<Validator> m_validator;

  //std::unordered_unordered_map<ndn::Name, const ndn::RegisteredPrefixId*> m_registeredEventsList;
  //ndn::InMemoryStoragePersistent m_inMemoryStorage;
};

} // namespace notificationLib

#endif // NOTIFICATIONLIB_API_CPP
