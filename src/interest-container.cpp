#include "interest-container.hpp"

namespace notificationLib {

UnsatisfiedInterest::UnsatisfiedInterest(const Interest& interest,
                                         const Name& name)
  : interest(interest)
  , name(name)
{
}

} // namespace notificationLib
