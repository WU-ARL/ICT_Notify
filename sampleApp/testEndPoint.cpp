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
#include <fstream>
#include <csignal>

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
    , m_randomGenerator(std::random_device{}())
    , m_eventNameDist(1,5)
    , m_eventSchedDist(1,20)
    , m_eventsCounter(0)
    , m_scheduler(m_face.getIoService())
    , m_totalEventsToSend(0)
    {

    }

    void
    usage()
    {
      std::cout << "\n Usage:\n " << m_programName <<
      ""
      " [-h] -f filters_file  -n events_number -l log_name [-d debug_mode]\n"
      " Register and push event notifications.\n"
      "\n"
      " \t-h - print this message and exit\n"
      " \t-f - configuration file \n"
      " \t-n - total number of events to send \n"
      " \t-l - log file name \n"
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
      //m_allSentEvents.reserve(m_totalEventsToSend);

      m_sentEventId =
        m_scheduler.scheduleEvent(ndn::time::milliseconds(m_eventSchedDist(m_randomGenerator)),
                                  bind(&NotificationProducer::sendEvent,this));

      while (true)
      {
        // if (m_allSentEvents.size() >= m_totalEventsToSend)
        // {
        //   // cancel last scheduled event
        //   m_scheduler.cancelEvent(m_sentEventId);
        //
        //   // print logs
        //   Log();
        //   exit;
        // }

        m_face.processEvents(time::seconds(1));
      }
    }
    void init()
    {
      if (m_notificationHandler != nullptr) {
        std::cout << "Trying to initialize notification handler, but it already exists";
        return;
      }
      //std::shared_ptr<ndn::Face> facePtr(&m_notificationFace, NullDeleter<Face>());
      // register signal SIGINT and signal handler
      signal(SIGINT, NotificationProducer::signalHandler);
      signal(SIGTERM, NotificationProducer::signalHandler);

      m_notificationHandler = std::make_shared<notificationLib::api>(m_face,
                                                                     notificationLib::api::DEFAULT_NAME,
                                                                     notificationLib::api::DEFAULT_VALIDATOR);
     sleep(5);
      m_notificationHandler->init(m_fileName,
              std::bind(&NotificationProducer::onNotificationUpdateWithTime, this, _1, _2));
        //m_notificationHandler->registerNotificationPrefix(m_name);

    }
    static void signalHandler( int signum )
    {
      me->Log();
      exit(signum);
    }

    Name
    getRandomEvent()
    {
      Name event("/test/event/");
      event.appendNumber(m_eventNameDist(m_randomGenerator));

      return event;
    }
    void onNotificationUpdateWithTime (uint64_t receivedTime, const std::map<uint64_t,std::vector<Name>>& notificationList)
    {
      std::cout << "print to m_receivedStream" << std::endl;
      m_receivedStream << "Recieved At: \t " << "Sent At: \t " << "Names" << std::endl;
      for(auto const& iList: notificationList)
      {
        m_allRecievedEvents[iList.first] = iList.second;
        m_receivedStream << receivedTime << ", ";
        m_receivedStream << iList.first << ", ";
        for (size_t i = 0; i < iList.second.size(); i++)
        {
          m_receivedStream << ", " << iList.second[i];
        }
        m_receivedStream << std::endl;
      }
    }
    void Log()
    {
      std::ofstream output(m_logFile.c_str());

      output << "Events sent: ";
      for(auto const& iSent: m_allSentEvents)
      {
        output << iSent << ", ";
      }
      output << std::endl;

      if(m_receivedStream.rdbuf()!= NULL)
        output << m_receivedStream.rdbuf();
    }
    void setTotalEventsNum(int num)
    {
      m_totalEventsToSend = num;
    }
    void setLogFileName(std::string fileName)
    {
      m_logFile = fileName;
    }
    void sendEvent()
    {
      if (m_allSentEvents.size() < m_totalEventsToSend)
      {
        Name event;
        std::vector<Name> eventList;

        event = getRandomEvent();
        eventList.push_back(event);
        m_allSentEvents.push_back(event);
        m_eventsCounter++;

        m_notificationHandler->notify("/wustl/test/service", eventList);

        // schedule the next sending
        m_sentEventId =
          m_scheduler.scheduleEvent(ndn::time::milliseconds(m_eventSchedDist(m_randomGenerator)),
                                    bind(&NotificationProducer::sendEvent,this));
      }

    }

    // void onNotificationUpdate (const std::vector<Name>& nameList)
    // {
    //   for (size_t i = 0; i < nameList.size(); i++) {
    //       std::cout << " application received notification: " << nameList[i] << std::endl;
    //     }
    // }
    static NotificationProducer* me;
  private:
    std::string m_programName;
    //Name m_name;
    std::string m_fileName;
    std::shared_ptr<notificationLib::api> m_notificationHandler;
    Face m_face;
    //Face& m_notificationFace;
    KeyChain m_keyChain;
    security::ValidatorNull m_validator;
    int m_totalEventsToSend;
    int m_eventsCounter;
    ndn::Scheduler m_scheduler;
    ndn::EventId m_sentEventId;
    std::vector<Name> m_allSentEvents;
    std::map<uint64_t,std::vector<Name>> m_allRecievedEvents;
    std::mt19937 m_randomGenerator;
    std::uniform_int_distribution<> m_eventNameDist;
    std::uniform_int_distribution<> m_eventSchedDist;
    std::string m_logFile;
    std::ostringstream m_receivedStream;

  };
} // namespace ndn

ndn::NotificationProducer producer("NotificationProducer");
ndn::NotificationProducer* ndn::NotificationProducer::me = &producer;

int
main(int argc, char* argv[])
{
  int option;
  bool fileSet = false;
  bool LogSet = false;

  while ((option = getopt(argc, argv, "hf:n:l:d:")) != -1)
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
      case 'n':
        producer.setTotalEventsNum(atoi(optarg));
        break;
      case 'l':
        producer.setLogFileName(optarg);
        LogSet = true;
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

  if(!fileSet || !LogSet ) {
    producer.usage();
    return 1;
  }
  else
    producer.init();

  producer.listen();

  std::cout << "here";
  return 0;
}