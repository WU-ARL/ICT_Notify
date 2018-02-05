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
#include <sstream>
#include <iostream>

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
      " [-h] -n namespace [-d debug_mode]\n"
      " Register and push event notifications.\n"
      "\n"
      " \t-h - print this message and exit\n"
      " \t-n - register a namespace for notifications \n"
      " \t-d - sets the debug mode, 1 - debug on, 0 - debug off (default)\n"
      "\n";
      exit(1);
    }

    void
    setEventName(std::string name)
    {
      m_name = name;
      if (DEBUG)
        std::cout << "set name to " << m_name << std::endl;
    }
    void
    listen()
    {
      // just give it a chance to read from file
      while (true)
      {
        std::string fullInput, name;
        notificationLib::NotificationData eventList;

        m_face.processEvents(time::seconds(1));
        std::cout << "..: ";
        std::getline(std::cin, fullInput);

        std::vector<std::string> parsedText;
        boost::split(parsedText, fullInput, boost::is_any_of(";"), boost::token_compress_on);

        for(std::vector<std::string>::iterator it = parsedText.begin(); it != parsedText.end(); ++it)
        {
          Name event(m_name);
          event.append(*it);
          eventList.add(event);
        }

        m_notificationHandler->notify(m_name, eventList);
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

        m_notificationHandler->registerEventPrefix(m_name);

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
  ndn::NotificationProducer producer(argv[0]);
  int option;
  bool nameSet = false;
  bool filterSet = false;

  while ((option = getopt(argc, argv, "hn:d:")) != -1)
  {
    switch (option)
    {
      case 'n':
        producer.setEventName((std::string)(optarg));
        nameSet = true;
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

  if(!nameSet) {
    producer.usage();
    return 1;
  }
  else
    producer.init();

  producer.listen();

  std::cout << "here";
  return 0;
}
