  /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014, Washington University in St. Louis,
 *
 */

#include <boost/asio.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/mgmt/nfd/face-status.hpp>
#include <ndn-cxx/encoding/buffer-stream.hpp>
#include <unordered_set>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/segment-fetcher.hpp>
#include <ndn-cxx/util/time.hpp>
#include <ndn-cxx/security/validator-null.hpp>

//#include <notificationLib/api.hpp>
#include <../src/api.hpp>

// global variable to support debug
int DEBUG = 0;

namespace ndn {
  class NotificationConsumer
  {
  public:

    NotificationConsumer(char* programName, std::shared_ptr<ndn::Face> notificationFace)
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
      if (DEBUG)
        std::cout << "set file name to " << m_fileName << std::endl;
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
      //std::shared_ptr<ndn::Face> facePtr(&m_face, NullDeleter<Face>());

      //m_notificationHandler = std::make_shared<notificationLib::api>(m_name, m_face,
        //                                              std::bind(&NotificationConsumer::onNotificationUpdate, this, _1));
      m_notificationHandler = std::make_shared<notificationLib::api>(m_face,
                                                                     notificationLib::api::DEFAULT_NAME,
                                                                     notificationLib::api::DEFAULT_VALIDATOR);
      //m_notificationHandler->subscribe(m_name,
        //                               std::bind(&NotificationConsumer::onNotificationUpdate, this, _1));

      // subscribe using a configuration file
      //std::string file("/Users/hila/Documents/ndn/notificationlibrary/sampleApp/config_regex");

      // m_notificationHandler->init(m_fileName,
      //                             std::bind(&NotificationConsumer::onNotificationUpdate, this, _1));

      m_notificationHandler->init(m_fileName,
                                  std::bind(&NotificationConsumer::onNotificationUpdateWithTime, this, _1, _2));
      sleep (2);
    }

    void onNotificationUpdateWithTime (uint64_t receivedTime, const std::unordered_map<uint64_t,std::vector<Name>>& notificationList)
    {
      for (size_t i = 0; i < notificationList.size(); i++) {
          //std::cout << " application received notification: " << nameList[i] << std::endl;
        }
    }
    void onNotificationUpdate (const std::vector<Name>& nameList)
    {
      for (size_t i = 0; i < nameList.size(); i++) {
          std::cout << " application received notification: " << nameList[i] << std::endl;
        }
    }
  private:
    std::string m_programName;
    Name m_name;
    std::string m_fileName;
    std::shared_ptr<notificationLib::api> m_notificationHandler;
    Face m_face;
    //Face& m_notificationFace;
    KeyChain m_keyChain;
    security::ValidatorNull m_validator;
  };
} // namespace ndn

int
main(int argc, char* argv[])
{
  ndn::NotificationConsumer consumer(argv[0], nullptr);
  int option;
  bool nameSet = false;
  bool filterSet = false;

  // ndn::time::system_clock::TimePoint now = ndn::time::system_clock::now();
  // std::cout << "sizeof TimePoint is: " << sizeof(ndn::time::system_clock::TimePoint) << std::endl;
  // std::cout << "sizeof uint64_t is: " << sizeof(uint64_t) << std::endl;

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

  if(!filterSet) {
    consumer.usage();
    return 1;
  }
  else
    consumer.init();

  consumer.listen();

  return 0;
}
