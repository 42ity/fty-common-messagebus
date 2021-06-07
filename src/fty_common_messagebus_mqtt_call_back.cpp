/*  =========================================================================
    fty_common_messagebus_mqtt - class description

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

/*
@header
    fty_common_messagebus_mqtt_call_back.cpp -
@discuss
@end
*/

//#include "fty/messagebus/mqtt/fty_common_messagebus_mqtt.hpp"
#include "fty/messagebus/mqtt/fty_common_messagebus_mqtt_call_back.hpp"
#include "fty_common_messagebus_message.h"
#include <fty_log.h>

#include <mqtt/async_client.h>
#include <mqtt/properties.h>

namespace
{

  using namespace messagebus;

  static auto getMetaDataFromMqttProperties(const mqtt::properties& props) -> const messagebus::MetaData
  {
    auto metaData = messagebus::MetaData{};

    // User properties
    if (props.contains(mqtt::property::USER_PROPERTY))
    {
      std::string key, value;
      for (size_t i = 0; i < props.count(mqtt::property::USER_PROPERTY); i++)
      {
        std::tie(key, value) = mqtt::get<mqtt::string_pair>(props, mqtt::property::USER_PROPERTY, i);
        metaData.emplace(key, value);
      }
    }
    // Req/Rep pattern properties
    if (props.contains(mqtt::property::CORRELATION_DATA))
    {
      metaData.emplace(messagebus::Message::CORRELATION_ID, mqtt::get<std::string>(props, mqtt::property::CORRELATION_DATA));
    }

    if (props.contains(mqtt::property::RESPONSE_TOPIC))
    {
      metaData.emplace(messagebus::Message::REPLY_TO, mqtt::get<std::string>(props, mqtt::property::RESPONSE_TOPIC));
    }
    return metaData;
  }

} // namespace

namespace messagebus
{
  /////////////////////////////////////////////////////////////////////////////

  // Callback called when connection lost.
  void callback::connection_lost(const std::string& cause)
  {
    log_error("Connection lost");
    if (!cause.empty())
    {
      log_error("raison: %s", cause.c_str());
    }
  }

  // Callback called for connection done.
  void callback::onConnected(const std::string& cause)
  {
    log_debug("Connected");
    if (!cause.empty())
    {
      log_debug("raison: %s", cause.c_str());
    }
  }

  // Callback called for connection updated.
  bool callback::onConnectionUpdated(const mqtt::connect_data& /*connData*/)
  {
    log_info("Connection updated");
    return true;
  }

  // Callback called when a message arrives.
  void callback::onRequestArrived(mqtt::const_message_ptr msg, MessageListener messageListener)
  {
    log_trace("Message received from topic: '%s'", msg->get_topic().c_str());
    // build metaData message from mqtt properties
    auto metaData = getMetaDataFromMqttProperties(msg->get_properties());
    // Call message listener with a mqtt message to Message convertion
    messageListener(Message{metaData, msg->get_payload_str()});
    // TODO do it but core dump in terminate?
    if (metaData.find(Message::SUBJECT)->second == ANSWER_USER_PROPERTY)
    {
      //MqttMessageBus::unsubscribe(msg->get_topic());
    }
  }

  // Callback called when a message arrives.
  void callback::onMessageArrived(mqtt::const_message_ptr msg, MessageListener messageListener)
  {
    log_trace("Message received from topic: '%s'", msg->get_topic().c_str());
    // build metaData message from mqtt properties
    auto metaData = getMetaDataFromMqttProperties(msg->get_properties());
    // Call message listener with a mqtt message to Message convertion
    messageListener(Message{metaData, msg->get_payload_str()});
    // TODO do it but core dump in terminate?
    if (metaData.find(Message::SUBJECT)->second == ANSWER_USER_PROPERTY)
    {
      //MqttMessageBus::unsubscribe(msg->get_topic());
    }
  }

} // namespace messagebus
