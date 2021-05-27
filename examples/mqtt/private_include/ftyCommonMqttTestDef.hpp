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
**  \file ftyCommonMqttTestDef.hpp
**  \author X. Millieret (EATON)
**
********************************************************************************
**
**  \brief ftyCommonMqttTestDef
**
********************************************************************************
**
*/

#ifndef FTY_COMMON_MQTT_TEST_DEF_HPP
#define FTY_COMMON_MQTT_TEST_DEF_HPP

namespace messagebus
{
  auto constexpr MQTT_END_POINT{"tcp://localhost:1883"};

  auto constexpr SAMPLE_TOPIC{"eaton/sample/pubsub"};

  auto constexpr REQUEST_QUEUE{"ETN_Q_REQUEST"};
  auto constexpr REPLY_QUEUE{"ETN_Q_REPLY"};

} // namespace messagebus

#endif // FTY_COMMON_MQTT_TEST_DEF_HPP
