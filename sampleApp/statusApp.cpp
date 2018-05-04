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
#include <thread>

//#include <notificationLib/api.hpp>
#include <../src/api.hpp>
// global variable to support debug
int DEBUG = 0;

namespace ndn {
  class StatusApp
  {
  public:

    StatusApp(char* programName, std::shared_ptr<ndn::Face> notificationFace)
    : m_programName(programName)
    {

    }
    void
    usage()
    {
      std::cout << "\n Usage:\n " << m_programName <<
      ""
      " [-h] -f filters_file -p prefix -n user_name[-d debug_mode]\n"
      " request notifications for a namespace and its filters.\n"
      "\n"
      " \t-h - print this message and exit\n"
      " \t-f - configuration file \n"
      " \t-n - instance user name\n"
      " \t-d - sets the debug mode, 1 - debug on, 0 - debug off (default)\n"
      "\n";
      exit(1);
    }
    void
    setUserName(std::string name)
    {
      m_userName = name;
      if (DEBUG)
        std::cout << "set user name to " << m_userName << std::endl;
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

      m_notificationHandler = std::make_shared<notificationLib::api>(m_face,
                                                                     notificationLib::api::DEFAULT_NAME,
                                                                     notificationLib::api::DEFAULT_VALIDATOR);

      m_notificationHandler->init(m_fileName,
                                  std::bind(&StatusApp::onNotificationUpdateWithTime, this, _1, _2));

      // no need
      //m_notificationHandler->registerNotificationPrefix(m_eventName);
      sleep (2);
    }
    void onNotificationUpdateWithTime (uint64_t receivedTime, const std::map<uint64_t,std::vector<Name>>& notificationList)
    {
      for(auto const& iList: notificationList)
      {
        std::cout << "NOTIFICATION AT: " << iList.first << std::endl;
        for (size_t i = 0; i < iList.second.size(); i++) {
          std::cout << "  " << iList.second[i] << std::endl;
          }
      }

    }
    void onNotificationUpdate (const std::vector<Name>& nameList)
    {
      ndn::name::Component userName(m_userName);
      for (size_t i = 0; i < nameList.size(); i++) {
          // get user Name component
          if(nameList[i].get(3) != userName)
          {
            std::cout << " *******" << nameList[i].get(3) << " is "
                      << nameList[i].get(2) << "*******" << std::endl;
          }
        }
    }
    void
    setNotificationName(std::string name)
    {
      m_eventName = name;
      if (DEBUG)
        std::cout << "set event name to " << m_eventName << std::endl;
    }
    void readIOinput()
    {
      std::string input;
      std::cout << "offline" << std::endl;
      Name eventOn("/user/status/on/");
      Name eventOff("/user/status/off/");
      eventOn.append(m_userName);
      eventOff.append(m_userName);

      std::vector<Name> eventListOn;
      std::vector<Name> eventListOff;

      eventListOn.push_back(eventOn);
      eventListOff.push_back(eventOff);
      while (1)
      {
        std::getline(std::cin, input);
        if(input == "on")
        {
          m_notificationHandler->notify(m_eventName, eventListOn);
          std::cout << "....: ";
          std::getline(std::cin, input);
          while (input != "off")
          {
            std::cout << "....: ";
            std::getline(std::cin, input);
          }
          std::cout << "offline" << std::endl;
          m_notificationHandler->notify(m_eventName, eventListOff);

        }
        //std::cout << "Hello, World!!!!" << std::endl;
        //sleep(2);
      }
    }
    void start()
    {
      //lunch io thread
      std::thread t1(&StatusApp::readIOinput, this);

      // listen to notification events
      listen();
      t1.join();
    }

  private:
    std::string m_programName;
    std::string m_userName;
    std::string m_eventName;
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
  ndn::StatusApp statusApp(argv[0], nullptr);

  int option;
  bool userNameSet = false;
  bool filterSet = false;

  while ((option = getopt(argc, argv, "hf:n:d:")) != -1)
  {
    switch (option)
    {
      case 'f':
        statusApp.setFile((std::string)(optarg));
        filterSet = true;
        break;
      case 'n':
        statusApp.setUserName((std::string)(optarg));
        userNameSet = true;
        break;
      case 'h':
        statusApp.usage();
        break;
      case 'd':
        DEBUG = atoi(optarg);
        break;
      default:
        statusApp.usage();
        break;
    }
  }

  argc -= optind;
  argv += optind;

  if(!filterSet || !userNameSet) {
    statusApp.usage();
    return 1;
  }
  else
  {
    statusApp.setNotificationName("/wustl/test/service");
    statusApp.init();
  }

  statusApp.start();
  //std::thread t1(readIO);

  //std::cout << "MAIN!!!!" << std::endl;
  //t1.join();

  //statusApp.listen();

  return 0;
}
