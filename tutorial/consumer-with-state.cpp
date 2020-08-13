#include <../src/api.hpp>

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/encoding/buffer-stream.hpp>
#include <ndn-cxx/mgmt/nfd/face-status.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/validator-null.hpp>
#include <ndn-cxx/util/segment-fetcher.hpp>

#include <boost/asio.hpp>

#include <unordered_map>
#include <vector>
#include <iostream>


// global variable to support debug
int DEBUG = 0;

enum class Status
{
    Good,
    Bad
};

std::string status2str(int status)
{
    std::string rval;
    switch(status)
    {
    case 0:
        rval = "Good";
        break;
    case 1:
        rval = "Bad";
        break;
    default:
        break;
    }
    return rval;
}

struct DetailedEvent
{
    uint64_t    rxTime;
    double      latitude;
    double      longitude;
    Status      status;
    std::string entityName;

    DetailedEvent(uint64_t time, double lat, double lon, Status stat, std::string name) :
        rxTime(time), latitude(lat), longitude(lon), status(stat), entityName(name)
    { }

    DetailedEvent(uint64_t time, double lat, double lon, int stat, std::string name) :
        rxTime(time), latitude(lat), longitude(lon), entityName(name)
    {
        status = static_cast<Status>(stat);
    }

    DetailedEvent(const DetailedEvent& other) : 
        rxTime(other.rxTime), latitude(other.latitude), longitude(other.longitude),
        status(other.status), entityName(other.entityName)
    { }
    
    DetailedEvent(DetailedEvent&& other) noexcept :
        rxTime( std::move(other.rxTime) ),
        latitude( std::move(other.latitude) ),
        longitude( std::move(other.longitude) ),
        status( std::move(other.status) ),
        entityName( std::move(other.entityName) )
    { }

    DetailedEvent& operator=(const DetailedEvent& other)
    {
        if (this != &other)
        {
            rxTime = other.rxTime;
            latitude = other.latitude;
            longitude = other.longitude;
            status = other.status;
            entityName = other.entityName;
        }
        return *this;       
    }

    DetailedEvent& operator=(DetailedEvent&& other)
    {
        if (this != &other)
        {
            rxTime = std::move(other.rxTime);
            latitude = std::move(other.latitude);
            longitude = std::move(other.longitude);         
            status = std::move(other.status);
            entityName = std::move(other.entityName);
        }
        return *this;
    }
};
    
namespace ndn
{

typedef std::unordered_map<uint64_t, std::vector<Name>> NotificationMap;
  
class NotificationConsumer
{
public:
    NotificationConsumer(char* programName)
        : m_programName(programName)
    {

    }
    
    void
    usage()
    {
        std::cout << "\n Usage:\n " << m_programName <<
            ""
            " [-h] -f filters_file [-d debug_mode]\n"
            " request notifications for a namespace and its filters.\n"
            "\n"
            " \t-h - print this message and exit\n"
            " \t-f - file containing event filters\n"
            " \t-d - sets the debug mode, 1 - debug on, 0 - debug off (default)\n"
            "\n";
        exit(1);
    }

    void
    setFile(std::string file)
    {
        m_fileName = file;
        if (DEBUG) {
            std::cout << "set file name to " << m_fileName << std::endl;
        }
    }
    
    void
    listen()
    {
        m_face.processEvents();
    }

    void
    listen(int timeout)
    {
        time::milliseconds timeout_ms(timeout);
        while (true)
        {
            m_face.processEvents(timeout_ms);
            if (changes) {
                printStatus();
                changes = false;
            }
        }
    }
    
    
    void init()
    {
        if (m_notificationHandler != nullptr) {
            std::cout << "Trying to initialize notification handler, but it already exists";
            return;
        }

        m_notificationHandler = std::make_shared<notificationLib::api>(m_face,
                                                                       notificationLib::api::DEFAULT_NAME,
                                                                       notificationLib::api::DEFAULT_VALIDATOR);
        m_notificationHandler->init(m_fileName,
                                    std::bind(&NotificationConsumer::onNotificationUpdateWithTime, this, _1, _2));

        changes = false;      
        sleep(2);
    }

    void
    onNotificationUpdateWithTime(uint64_t receivedTime,
                                 const NotificationMap& notiMap)
    {
        std::vector<DetailedEvent> events;
        for (auto& map_itr : notiMap)
        {
            for (auto& noti : map_itr.second)
            {
                if (DEBUG) {
                    std::cout << "application received notification (t = "
                              << receivedTime << "): " << noti << std::endl;
                }

                int status = std::stoi( noti.get(-1).toUri() );
                double longitude = std::stod( noti.get(-2).toUri() );
                double latitude = std::stod( noti.get(-3).toUri() );
                std::string entityName( noti.get(-4).toUri() );
                
                events.emplace_back( DetailedEvent(receivedTime,
                                                   latitude, longitude,
                                                   status, entityName) );
            }
        }

        handleEvents( std::move(events) );
    }

    void
    handleEvents(std::vector<DetailedEvent>&& events)
    {
        for (auto &event : events)
        {
            auto mapItr = recentUpdates.find(event.entityName);
            // if name is already in map, keep the most recent update only
            if ( mapItr != recentUpdates.end() )
            {
                if (mapItr->second.rxTime < event.rxTime) {
                    mapItr->second = event;
                    changes = true;
                }
                continue;
            }
            recentUpdates.insert( std::make_pair(event.entityName, event) );
            changes = true;
        }
    }

    void printStatus()
    {
        std::cout << "**Status**\n";
        for (auto entry : recentUpdates)
        {
            std::cout << entry.first << " (t = " << entry.second.rxTime << "): "
                      << entry.second.latitude << "," << entry.second.longitude << "\t"
                      << status2str( (int) entry.second.status) << "\n";
        }
        std::cout << std::endl;
    }

private:
    std::shared_ptr<notificationLib::api> m_notificationHandler;
    
    Face m_face;
    Name m_name;
    KeyChain m_keyChain;
    security::ValidatorNull m_validator;

    std::string m_programName;
    std::string m_fileName;

    std::unordered_map<std::string, DetailedEvent> recentUpdates;
    bool changes;
};

} // namespace ndn

int
main(int argc, char* argv[])
{
    ndn::NotificationConsumer consumer(argv[0]);
    int option;
    bool filterSet = false;

    while ((option = getopt(argc, argv, "hf:d:")) != -1)
    {
        switch (option)
        {
        case 'f':
            consumer.setFile((std::string)(optarg));
            filterSet = true;
            break;
        case 'h':
            consumer.usage();
            break;
        case 'd':
            DEBUG = atoi(optarg);
            break;
        default:
            consumer.usage();
            break;
        }
    }

    argc -= optind;
    argv += optind;

    if(!filterSet)
    {
        consumer.usage();
        return 1;
    }

    consumer.init();
    consumer.listen(5000);
    std::cout << "end of program" << std::endl;
    return 0;
}
