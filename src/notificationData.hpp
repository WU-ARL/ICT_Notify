#ifndef NOTIFICATIONLIB_NOTIFICATION_DATA_HPP
#define NOTIFICATIONLIB_NOTIFICATION_DATA_HPP

#include "common.hpp"
#include <ndn-cxx/encoding/tlv-nfd.hpp>
#include <ndn-cxx/security/key-chain.hpp>

namespace notificationLib
{

  namespace tlv
  {
    enum
    {
      EventDataReply  = 128
    };
  }

  class NotificationData : noncopyable
  {
  public:
    bool
    empty() const
    {
      return m_notificationList.empty();
    }

    void
    add(const Name& name)
    {
      m_notificationList.push_back(name);
    }

    //template<bool T>
    template<encoding::Tag T>
    size_t
    wireEncode(EncodingImpl<T>& encoder) const
    {
      size_t totalLength = 0;

      for (std::vector<Name>::const_reverse_iterator i = m_notificationList.rbegin();
           i != m_notificationList.rend(); ++i) {
        totalLength += i->wireEncode(encoder);
      }

      totalLength += encoder.prependVarNumber(totalLength);
      totalLength += encoder.prependVarNumber(tlv::EventDataReply);
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
      m_notificationList.clear();

      if (!wire.hasWire())
        std::cerr << "The supplied block does not contain wire format" << std::endl;

      if (wire.type() != tlv::EventDataReply)
        std::cerr << "Unexpected TLV type when decoding CollectorReply: " +
          boost::lexical_cast<std::string>(wire.type()) <<std::endl;

      wire.parse();

      for (Block::element_const_iterator it = wire.elements_begin();
           it != wire.elements_end(); it++)
      {
      //  if (it->type() == statusCollector::tlv::FaceStatus)
        {
          m_notificationList.push_back(Name(*it));
        }
        //else
        //  std::cerr << "No FaceStatus entry when decoding CollectorReply!!" << std::endl;
      }
    }
  public:
    std::vector<Name> m_notificationList;
  };

} // notificationLib

#endif // NOTIFICATIONLIB_NOTIFICATION_DATA_HPP
