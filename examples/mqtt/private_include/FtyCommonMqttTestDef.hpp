/*  =========================================================================
    FtyCommonMqttTestDef.hpp - class description

    Copyright (C) 2014 - 2021 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
*/

#ifndef FTY_COMMON_MQTT_TEST_DEF_HPP
#define FTY_COMMON_MQTT_TEST_DEF_HPP

#include <string>

namespace messagebus
{
  // Default mqtt end point
  static auto constexpr DEFAULT_MQTT_END_POINT{"tcp://localhost:1883"};
  static auto constexpr SECURE_MQTT_END_POINT{"tcp://localhost:8883"};

  // Topic
  static auto constexpr SAMPLE_TOPIC{"ETN/T/SAMPLE/PUBSUB"};

  // Queues
  static auto constexpr REQUEST_QUEUE{"ETN/Q/REQUEST"};
  static const std::string REPLY_QUEUE = "ETN/Q/REPLY";

} // namespace messagebus

#endif // FTY_COMMON_MQTT_TEST_DEF_HPP
