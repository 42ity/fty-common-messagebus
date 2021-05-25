/**
********************************************************************************
**
**  Copyright (C) 2017-2021 Eaton
**
**  This software is confidential and licensed under Eaton Proprietary License
**  (EPL or EULA).
**  This software is not authorized to be used, duplicated or disclosed to
**  anyone without the prior written permission of Eaton.
**  Limitations, restrictions and exclusions of the Eaton applicable standard
**  terms and conditions, such as its EPL and EULA, apply.
**
********************************************************************************
**
**  \file LoggerCommonDef.hpp
**  \author X. Millieret (EATON)
**
********************************************************************************
**
**  \brief Eaton common logging definition
**
********************************************************************************
**
*/

#ifndef ETN_LOGGING_LOGGER_COMMON_DEF_HPP
#define ETN_LOGGING_LOGGER_COMMON_DEF_HPP

namespace etn::logging
{
  constexpr auto LOG_LVL_TRACE                = "trace";
  constexpr auto LOG_LVL_DEBUG                = "debug";
  constexpr auto LOG_LVL_INFO                 = "info";
  constexpr auto LOG_LVL_WARNING              = "warn";
  constexpr auto LOG_LVL_ERROR                = "error";
  constexpr auto LOG_LVL_CRITICAL             = "critical";
  constexpr auto LOG_LVL_OFF                  = "off";

  constexpr auto DEFAULT_LOG_PATTERN          = "[%Y-%m-%d %H:%M:%S:%e] [%n] [%^%l%$] [%s:%#] %v";
  constexpr auto STD_OUT_SINK                 = "stdout";
  constexpr auto STD_ERR_SINK                 = "stderr";
  constexpr auto SYSLOG_SINK                  = "syslog";
  constexpr auto DEFAULT_MAX_SIZE_FILE        = 1000000;
  constexpr auto DEFAULT_MAX_FILE             = 3;

  constexpr auto ETN_PQ_LOGGING_ENV_VAR       = "ETN_PQ_LOGGING";
  constexpr auto ETN_PQ_LOGGING_LOGGER_NAME   = "etn-pq-logging";

} // namespace etn::logging

#endif // ETN_LOGGING_LOGGER_COMMON_DEF_HPP
