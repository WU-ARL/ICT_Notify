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
#include <sstream>
#include <iostream>

//#include <notificationLib/api.hpp>
#include <../src/api.hpp>
// global variable to support debug
int DEBUG = 0;

namespace ndn {
  class NotificationProducer
  {
  public:

    NotificationProducer(char* programName)
    : m_programName(programName)
    {
    }

    void
    usage()
    {
      std::cout << "\n Usage:\n " << m_programName <<
      ""
      " [-h] -f filters_file [-d debug_mode]\n"
      " Register and push event notifications.\n"
      "\n"
      " \t-h - print this message and exit\n"
      " \t-f - configuration file \n"
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
      // just give it a chance to read from file
      while (true)
      {
        std::string fullInput, name;
        std::vector<Name> eventList;

        m_face.processEvents(time::seconds(1));
        std::cout << "..: ";
        std::getline(std::cin, fullInput);

        std::vector<std::string> parsedText;
        boost::split(parsedText, fullInput, boost::is_any_of(";"), boost::token_compress_on);

        for(std::vector<std::string>::iterator it = parsedText.begin(); it != parsedText.end(); ++it)
        {
          Name event("/test/event/");
          event.append(*it);
          eventList.push_back(event);
        }

        m_notificationHandler->notify("/test/service", eventList);
        // sleep for a bit to avoid 100% cpu usge
        //usleep(1000);
      }
    }
    void init()
    {
      if (m_notificationHandler != nullptr) {
        std::cout << "Trying to initialize notification handler, but it already exists";
        return;
      }
      //std::shared_ptr<ndn::Face> facePtr(&m_notificationFace, NullDeleter<Face>());

        m_notificationHandler = std::make_shared<notificationLib::api>(m_face,
                                                                       notificationLib::api::DEFAULT_NAME,
                                                                       notificationLib::api::DEFAULT_VALIDATOR);

        m_notificationHandler->init(m_fileName,
                std::bind(&NotificationProducer::onNotificationUpdateWithTime, this, _1, _2));
        //m_notificationHandler->registerNotificationPrefix(m_name);

    }
    void onNotificationUpdateWithTime (uint64_t receivedTime, const std::map<uint64_t,std::vector<Name>>& notificationList)
    {
      for (size_t i = 0; i < notificationList.size(); i++) {
          //std::cout << " application received notification: " << nameList[i] << std::endl;
        }
    }
    // void onNotificationUpdate (const std::vector<Name>& nameList)
    // {
    //   for (size_t i = 0; i < nameList.size(); i++) {
    //       std::cout << " application received notification: " << nameList[i] << std::endl;
    //     }
    // }

  private:
    std::string m_programName;
    //Name m_name;
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
  ndn::NotificationProducer producer(argv[0]);
  int option;
  bool fileSet = false;

  while ((option = getopt(argc, argv, "hf:d:")) != -1)
  {
    switch (option)
    {
      case 'f':
        producer.setFile((std::string)(optarg));
        fileSet = true;
        break;
      case 'h':
        producer.usage();
        break;
      case 'd':
        DEBUG = atoi(optarg);
        break;
      default:
        producer.usage();
        break;
    }
  }

  argc -= optind;
  argv += optind;

  if(!fileSet) {
    producer.usage();
    return 1;
  }
  else
    producer.init();

  producer.listen();

  std::cout << "here";
  return 0;
}
