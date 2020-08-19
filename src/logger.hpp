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
