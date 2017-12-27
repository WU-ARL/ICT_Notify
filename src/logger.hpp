#ifndef NOTIFICATIONLIB_LOGGER_HPP
#define NOTIFICATIONLIB_LOGGER_HPP

#include <ndn-cxx/util/logger.hpp>

#define INIT_LOGGER(name) NDN_LOG_INIT(notification.name)

#define _LOG_ERROR(x) NDN_LOG_ERROR(x)
#define _LOG_WARN(x)  NDN_LOG_WARN(x)
#define _LOG_INFO(x)  NDN_LOG_INFO(x)
#define _LOG_DEBUG(x) NDN_LOG_DEBUG(x)
#define _LOG_TRACE(x) NDN_LOG_TRACE(x)

#define _LOG_FUNCTION(x)     NDN_LOG_TRACE(__FUNCTION__ << "(" << x << ")")
#define _LOG_FUNCTION_NOARGS NDN_LOG_TRACE(__FUNCTION__ << "()")

#endif // NOTIFICATIONLIB_LOGGER_HPP
