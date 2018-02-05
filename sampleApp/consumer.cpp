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
#include <ndn-cxx/security/validator-null.hpp>
#include <notificationLib/api.hpp>

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
      " [-h] -n namespace -f filters_file [-d debug_mode]\n"
      " request notifications for a namespace and its filters.\n"
      "\n"
      " \t-h - print this message and exit\n"
      " \t-n - register a namespace for notifications \n"
      " \t-f - file containing filters for notifications\n"
      " \t-d - sets the debug mode, 1 - debug on, 0 - debug off (default)\n"
      "\n";
      exit(1);
    }

    void
    setName(std::string name)
    {
      m_name = name;
      if (DEBUG)
        std::cout << "set name to " << m_name << std::endl;
    }
    void
    setFilter(std::string file)
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
      m_notificationHandler->subscribe(m_name,
                                       std::bind(&NotificationConsumer::onNotificationUpdate, this, _1));

    }

    void onNotificationUpdate (const std::vector<Name>& nameList)
    {
      std::cout << "GOT NOTIFICATION!!"<< std::endl;
      for (size_t i = 0; i < nameList.size(); i++) {
          std::cout << "received notification: " << nameList[i] << std::endl;
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

  while ((option = getopt(argc, argv, "hn:f:d:")) != -1)
  {
    switch (option)
    {
      case 'n':
        consumer.setName((std::string)(optarg));
        nameSet = true;
        break;
      case 'f':
        consumer.setFilter((std::string)(optarg));
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

  if(!nameSet || !filterSet) {
    consumer.usage();
    return 1;
  }
  else
    consumer.init();

  consumer.listen();

  return 0;
}
