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

#ifndef NOTIFICATIONLIB_NOTIFICATION_DATA_HPP
#define NOTIFICATIONLIB_NOTIFICATION_DATA_HPP

#include "common.hpp"
#include <ndn-cxx/encoding/tlv-nfd.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <iostream>

namespace notificationLib
{

  namespace tlv
  {
    enum
    {
      NotificationList = 130,
      EventEntry = 132,
      Timestamp = 133,
      NotificationDataReply = 134,
      DataEntry = 135,
      DataList  = 136,
      Type      = 137,
      IBFEntryCount = 138,
      IBFEntryKeySum = 139,
      IBFEntryKeyCheck = 140,
      IBFEntryValueSum = 141,
      IBFEntryIndex = 142,
      IBFEntry = 143,
      IBFTable = 143,
      ListEntry = 144,
      ListTable = 145
    };
  }
  // namespace dataType
  // {
  //
  // }
  class NotificationData : noncopyable
  {
  public:
    enum dataType
    {
      DataContainer = 1,
      EventsContainer = 2,
      Unknown = 3
    };
    class NotificationDataList
    {
    public:
      NotificationDataList()
      {
        m_dataList.clear();
      };
      ~NotificationDataList()
      {};

      bool
      empty() const
      {
        return m_dataList.empty();
      }

      void
      addData(const uint64_t timestampKey, shared_ptr<Data>& dataPtr)
      {
          m_dataList[timestampKey] = dataPtr;
      }

      //template<bool T>
      template<encoding::Tag T>
      size_t
      wireEncode(EncodingImpl<T>& encoder) const
      {
        size_t totalLength = 0;

        for (auto i: m_dataList)
        {
          size_t entryLength = 0;
          entryLength += prependNonNegativeIntegerBlock(encoder, tlv::Timestamp, i.first);
          entryLength += i.second->wireEncode(encoder);
          entryLength += encoder.prependVarNumber(totalLength);
          entryLength += encoder.prependVarNumber(tlv::DataEntry);
          totalLength += entryLength;
        }

        totalLength += encoder.prependVarNumber(totalLength);
        totalLength += encoder.prependVarNumber(tlv::DataList);
        return totalLength;
      }

      Block
      wireEncode() const
      {
        Block block;

        EncodingEstimator estimator;
        size_t estimatedSize = wireEncode(estimator);

        EncodingBuffer buffer(estimatedSize);
        wireEncode(buffer);

        return buffer.block();
      }

      void
      wireDecode(const Block& wire)
      {
        m_dataList.clear();

        if (!wire.hasWire())
          std::cerr << "The supplied block does not contain wire format" << std::endl;

        if (wire.type() != tlv::DataList)
          std::cerr << "Unexpected TLV type when decoding DataList: " +
            boost::lexical_cast<std::string>(wire.type()) <<std::endl;

        wire.parse();

        for (Block::element_const_iterator it = wire.elements_begin();
             it != wire.elements_end(); it++)
        {
          if (it->type() == tlv::DataEntry)
          {
            it->parse();

            shared_ptr<Data> data;
            data->wireDecode(*it->elements_begin());
            ++it;

            if (it != it->elements_end())
              m_dataList[readNonNegativeInteger(*it)] = data;
            else
              std::cerr << "No timestamp number for data " << std::endl;
          }
          //else
          //  std::cerr << "No FaceStatus entry when decoding CollectorReply!!" << std::endl;
        }
      }
    private:
      // list of events if this is of type DataList
      std::unordered_map<uint64_t,shared_ptr<Data>> m_dataList;
    }; //end class NotificationDataList

    class NotificationEventsList
    {
    public:
      NotificationEventsList()
      {
        m_eventsListPerTimestamp.clear();
      };
      ~NotificationEventsList()
      { };

      bool
      empty() const
      {
        return m_eventsListPerTimestamp.empty();
      }

      // void
      // addEvent(const Name& name)
      // {
      //   m_eventsList.push_back(name);
      // }
      void
      set(std::unordered_map<uint64_t,std::vector<Name>>& notificationList)
      {
        //m_eventsList.insert(m_eventsList.end(),nameList.begin(), nameList.begin());
        m_eventsListPerTimestamp = notificationList;
      }
      void
      addEvents(const uint64_t timestampKey, const std::vector<Name>& nameList)
      {
        //m_eventsList.insert(m_eventsList.end(),nameList.begin(), nameList.begin());
        m_eventsListPerTimestamp[timestampKey] = nameList;
      }
      std::vector<Name>&
      getEventList(uint64_t timestampKey)
      {
        return m_eventsListPerTimestamp[timestampKey];
      }
      std::unordered_map<uint64_t,std::vector<Name>>&
      getEventList()
      {
        return m_eventsListPerTimestamp;
      }
      //template<bool T>
      template<encoding::Tag T>
      size_t
      wireEncode(EncodingImpl<T>& encoder) const
      {
        size_t totalLength = 0;
        // go over the unordered_map
        for (auto i: m_eventsListPerTimestamp)
        {
          size_t entryLength = 0;

          entryLength += prependNonNegativeIntegerBlock(encoder, tlv::Timestamp, i.first);

          // encode the list of events for timestamp
          const std::vector<Name>& events = i.second;
          for (std::vector<Name>::const_reverse_iterator iName = events.rbegin();
               iName != events.rend(); ++iName)
          {
            entryLength += iName->wireEncode(encoder);
          }
          entryLength += encoder.prependVarNumber(entryLength);
          entryLength += encoder.prependVarNumber(tlv::EventEntry);
          totalLength += entryLength;
        }

        totalLength += encoder.prependVarNumber(totalLength);
        totalLength += encoder.prependVarNumber(tlv::NotificationList);
        return totalLength;
        // size_t totalLength = 0;
        //
        // for (std::vector<Name>::const_reverse_iterator i = m_eventsList.rbegin();
        //      i != m_eventsList.rend(); ++i) {
        //   totalLength += i->wireEncode(encoder);
        // }
        //
        // totalLength += encoder.prependVarNumber(totalLength);
        // totalLength += encoder.prependVarNumber(tlv::NotificationList);
        // return totalLength;
      }

      Block
      wireEncode() const
      {
        Block block;

        EncodingEstimator estimator;
        size_t estimatedSize = wireEncode(estimator);

        EncodingBuffer buffer(estimatedSize);
        wireEncode(buffer);

        return buffer.block();
      }

      void
      wireDecode(const Block& wire)
      {
        m_eventsListPerTimestamp.clear();

        if (!wire.hasWire())
          std::cerr << "The supplied block does not contain wire format" << std::endl;

        if (wire.type() != tlv::NotificationList)
          std::cerr << "Unexpected TLV type when decoding reply: " +
            boost::lexical_cast<std::string>(wire.type()) <<std::endl;

        wire.parse();

        // for each entry
        for (Block::element_const_iterator it = wire.elements_begin();
             it != wire.elements_end(); it++)
        {
          if (it->type() == tlv::EventEntry)
          {
            it->parse();
            Block::element_const_iterator entryIt = it->elements_begin();
            std::vector<Name> events;
            uint64_t timestamp = 0;

            while(entryIt != it->elements_end())
            {
              if (entryIt->type() == tlv::Name)
              {
                events.push_back(Name(*entryIt));
              }
              else if (entryIt->type() == tlv::Timestamp)
                timestamp = readNonNegativeInteger(*entryIt);

              ++entryIt;
            }
            if(timestamp == 0 || events.empty())
              std::cerr << "Failed encoding  -  no timestamp or events in entry" <<std::endl;
            else
              m_eventsListPerTimestamp[timestamp] = events;
          }
          else
          {
            std::cerr << "Unexpected TLV type when decoding reply: " +
                      boost::lexical_cast<std::string>(wire.type()) <<std::endl;
          }
        }

      }
    private:
      // list of events per timestamp if this is of type EventList
      std::unordered_map<uint64_t,std::vector<Name>> m_eventsListPerTimestamp;

    }; // end class NotificationEventsList


    NotificationData()
    {
      m_type = dataType::Unknown;
    };
    NotificationData(std::unordered_map<uint64_t,std::vector<Name>>& notificationList)
    {
      m_type = dataType::EventsContainer;
      m_eventsObj.set(notificationList);
    };

    ~NotificationData()
    {
    };

    void
    setType(dataType type)
    {
      m_type = type;
    }

    bool
    empty() const
    {
      if(m_type == dataType::DataContainer)
        return m_dataObj.empty();
      else if(m_type == dataType::EventsContainer)
        return m_eventsObj.empty();

      return true;
    }

    // void
    // addEvent(const Name& name)
    // {
    //   if(m_type == dataType::EventsContainer)
    //     m_eventsObj.addEvent(name);
    // }

    void
    addEvents(const uint64_t timestamp, const std::vector<Name>& nameList)
    {
      if(m_type == dataType::EventsContainer)
        m_eventsObj.addEvents(timestamp, nameList);
    }

    void
    addData(const uint64_t timestampKey, shared_ptr<Data>& dataPtr)
    {
      if(m_type == dataType::DataContainer)
        m_dataObj.addData(timestampKey, dataPtr);
    }

    //template<bool T>
    template<encoding::Tag T>
    size_t
    wireEncode(EncodingImpl<T>& encoder) const
    {
      size_t totalLength = 0;

      if(m_type == dataType::Unknown)
        return 0;

      if(m_type == dataType::DataContainer)
        totalLength =  m_dataObj.wireEncode(encoder);
      else if (m_type == dataType::EventsContainer)
        totalLength = m_eventsObj.wireEncode(encoder);

      totalLength += prependNonNegativeIntegerBlock(encoder, tlv::Type, m_type);
      totalLength += encoder.prependVarNumber(totalLength);
      totalLength += encoder.prependVarNumber(tlv::NotificationDataReply);

      return totalLength;
    }

    Block
    wireEncode() const
    {
      Block block;

      EncodingEstimator estimator;
      size_t estimatedSize = wireEncode(estimator);

      EncodingBuffer buffer(estimatedSize);
      wireEncode(buffer);

      return buffer.block();
    }

    void
    wireDecode(const Block& wire)
    {
      if (!wire.hasWire())
        std::cerr << "The supplied block does not contain wire format" << std::endl;

      if (wire.type() != tlv::NotificationDataReply)
        std::cerr << "Unexpected TLV type when decoding NotificationDataReply: " +
          boost::lexical_cast<std::string>(wire.type()) <<std::endl;

      wire.parse();

      for (Block::element_const_iterator it = wire.elements_begin();
           it != wire.elements_end(); it++)
      {
        if (it->type() == tlv::Type)
        {
          m_type = NotificationData::dataType(readNonNegativeInteger(*it));

          ++it;
          if (it != it->elements_end())
          {
            if (m_type == dataType::DataContainer)
              m_dataObj.wireDecode(*it);
            else if (m_type == dataType::EventsContainer)
              m_eventsObj.wireDecode(*it);
            else
              std::cerr << "Unknown data type" << std::endl;
          }
          else
            std::cerr << "End of decoding while expeciting events or data TLV" << std::endl;
        }
        else
          std::cerr << "Expecting data type TLV" << std::endl;
      }
    }

  public:
    NotificationData::dataType m_type;

    // list of events if this is of type EventList
    NotificationEventsList m_eventsObj;

    // list of events if this is of type DataList
    NotificationDataList m_dataObj;

  }; // end  class NotificationData

} // notificationLib

#endif // NOTIFICATIONLIB_NOTIFICATION_DATA_HPP
