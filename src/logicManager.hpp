#ifndef LOGIC_MANAGER_HPP
#define LOGIC_MANAGER_HPP

#include "common.hpp"
#include "interest-table.hpp"
#include "notificationData.hpp"
#include <boost/random.hpp>




namespace notificationLib
{
  using NotificationCallback = function<void(const std::vector<Name>&)>;

  class NotificationProtocol : noncopyable
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

    NotificationProtocol(ndn::Face& face,
                         //const Name& notificationPrefix,
                         //const NotificationCallback& onUpdate,
                         const Name& defaultSigningId,
                         std::shared_ptr<Validator> validator,
                         const time::milliseconds& eventInterestLifetime,
                         const time::milliseconds& eventReplyFreshness);
    ~NotificationProtocol();

    void
    sendEventInterest(const Name& eventPrefix, const NotificationCallback& onUpdate);

    void
    registerEventPrefix(const Name& eventPrefix);

    void
    satisfyPendingEventInterests(const Name& notificationPrefix,
                                 const NotificationData& notificationList,
                                 const ndn::time::milliseconds& freshness);
  public:
    ndn::Scheduler&
    getScheduler()
    {
      return m_scheduler;
    }

  private:
    void
    onEventInterest(const Name& prefix, const Interest& interest);

    void
    onEventData(const Interest& interest, const Data& data, const NotificationCallback& onUpdate);

    void
    onEventNack(const Interest& interest);

    void
    onEventTimeout(const Interest& interest);

    void
    onEventDataValidationFailed(const Data& data);

    void
    onEventDataValidated(const Data& data, bool firstData);

    void
    processEventData(const Name& name,
                     const Block& EventReplyBlock);

    void
    sendEventData(const Name& eventPrefix,
                  const NotificationData& notificationList,
                  const ndn::time::milliseconds& freshness);

  public:
    /*
    static const ndn::Name DEFAULT_NAME;
    static const ndn::Name EMPTY_NAME;
    static const std::shared_ptr<Validator> DEFAULT_VALIDATOR;
*/
  private:

    // Communication
    ndn::Face& m_face;
    Name m_eventPrefix;
    std::unordered_map<ndn::Name, const ndn::RegisteredPrefixId*> m_registeteredEventsList;
    InterestTable m_interestTable;

    //std::vector <const ndn::PendingInterestId*> m_outstandingInterestList;
    // Event
    ndn::Scheduler m_scheduler;
    //ndn::EventId m_reexpressingInterestId;

    // Callback
    NotificationCallback m_onUpdate;

    // Timer
    time::milliseconds m_eventInterestLifetime;
    time::milliseconds m_eventReplyFreshness;
    boost::mt19937 m_randomGenerator;
    boost::variate_generator<boost::mt19937&, boost::uniform_int<> > m_reexpressionJitter;

    // Security
    Name m_signingId;
    ndn::KeyChain m_keyChain;
    std::shared_ptr<Validator> m_validator;
  };


} // notificationLib

#endif //LOGIC_MANAGER_HPP
