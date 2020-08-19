/* -*- Mode:C++; c-file-style:"bsd"; indent-tabs-mode:nil; -*- */
/**
 * Copyright 2020 Washington University in St. Louis
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. 
 */

#include <../src/api.hpp>

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/encoding/buffer-stream.hpp>
#include <ndn-cxx/mgmt/nfd/face-status.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/validator-null.hpp>
#include <ndn-cxx/util/segment-fetcher.hpp>

#include <boost/asio.hpp>

#include <unordered_map>
#include <iostream>


// global variable to support debug
int DEBUG = 0;

enum class Status
{
    Good,
    Bad
};

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
  
class NotificationProducer
{
public:
    NotificationProducer(char* programName)
        : m_programName(programName)
    { }

    void
    usage()
    {
        std::cout << "\n Usage:\n " << m_programName <<
            ""
            " [-h] -f filters_file -n name [-d debug_mode]\n"
            " Register and push event notifications.\n"
            "\n"
            " \t-h - print this message and exit\n"
            " \t-f - configuration file \n"
            " \t-n - name of producer publishing events\n"
            " \t-d - sets the debug mode, 1 - debug on, 0 - debug off (default)\n"
            "\n";
        exit(1);
    }

    void setEntityName(std::string entityName)
    {
        m_entityName = entityName;
    }

    void
    setFile(std::string file)
    {
        m_fileName = file;
        if (DEBUG)
            std::cout << "set file name to " << m_fileName << std::endl;
    }

    void init()
    {
        if (m_notificationHandler != nullptr)
        {
            std::cout << "Trying to initialize notification handler, but it already exists." << std::endl;
            return;
        }

        m_notificationHandler = std::make_shared<notificationLib::api>(m_face,
                                                                       notificationLib::api::DEFAULT_NAME,
                                                                       notificationLib::api::DEFAULT_VALIDATOR);
        m_notificationHandler->init(m_fileName,
                                    std::bind(&NotificationProducer::onNotificationUpdateWithTime, this, _1, _2));
    }
    
    void
    generate()
    {
        std::random_device rd;
        std::mt19937 gen( rd() );
        std::uniform_real_distribution<> latDist(-90.0, 90.0);
        std::uniform_real_distribution<> longDist(-180.0, 180.0);

        
        while (true)
        {
            m_face.processEvents( time::seconds(1) );
            
            // generate a random event
            TutorialEvent event { latDist(gen), longDist(gen), Status(std::rand() % 2) };

            // create our event by adding additional named components
            Name eventName("/test/event/");
            eventName.append(m_entityName);
            eventName.append( std::to_string(event.latitude) );
            eventName.append( std::to_string(event.longitude) );
            eventName.append( std::to_string(static_cast<int>(event.status)) );

            // add our event to an eventList
            std::vector<Name> eventList;
            eventList.push_back(eventName);

            // publish the event list under the notify namespace
            std::cout << "sending event = " << eventName << std::endl;
            m_notificationHandler->notify("/wustl/test/service", eventList);
            
            sleep(1);
        }
    }

    void
    onNotificationUpdateWithTime (uint64_t receivedTime, const NotificationMap& notiMap)
    {
        for (auto& map_itr : notiMap)
        {
            for (auto& noti : map_itr.second)
            {
                std::cout << "application received notification: " << noti << std::endl;
            }
        }
    }
    
private:
    std::shared_ptr<notificationLib::api> m_notificationHandler;

    Face m_face;
    KeyChain m_keyChain;
    security::ValidatorNull m_validator;

    std::string m_programName;
    std::string m_fileName;
    std::string m_entityName;
};

} // namespace ndn

int
main(int argc, char* argv[])
{
    ndn::NotificationProducer producer(argv[0]);
    int option;
    bool fileSet = false;
    bool nameSet = false;

    while ((option = getopt(argc, argv, "hf:n:d:")) != -1)
    {
        switch (option)
        {           
        case 'f':
            producer.setFile((std::string)(optarg));
            fileSet = true;
            break;
        case 'n':
            producer.setEntityName((std::string)(optarg));
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
    
    if(!fileSet) {
        producer.usage();
        return 1;
    }

    if(!nameSet) {
        producer.usage();
        return 2;
    }
    
    producer.init();
    producer.generate();
    return 0;
}
