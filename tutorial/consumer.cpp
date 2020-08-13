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
#include <sstream>
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

struct TutorialEvent
{
    double latitude;
    double longitude;
    Status status;

    TutorialEvent(double lat, double lon, Status stat) :
        latitude(lat), longitude(lon), status(stat)
    { }
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
      
        sleep(2);
    }

    void
    onNotificationUpdateWithTime(uint64_t receivedTime,
                                 const NotificationMap& notiMap)
    {
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

                std::cout << entityName << ": "
                          << latitude << "," << longitude << "\t"
                          << status2str(status) << std::endl;
            }
        }
    }

private:
    std::shared_ptr<notificationLib::api> m_notificationHandler;
    
    Face m_face;
    Name m_name;
    KeyChain m_keyChain;
    security::ValidatorNull m_validator;

    std::string m_programName;
    std::string m_fileName; 
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
    consumer.listen();
    return 0;
}
