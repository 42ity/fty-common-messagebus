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
    fty_common_messagebus_mqtt -
@discuss
@end
*/

#include "fty/messagebus/mqtt/fty_common_messagebus_mqtt.hpp"
#include "fty_common_messagebus_message.h"
#include <fty_log.h>

#include <mqtt/async_client.h>
#include <mqtt/properties.h>

namespace
{

  using namespace messagebus;

  // Callback called when a message arrives.
  static void onMessageArrived(mqtt::const_message_ptr msg, MessageListener messageListener)
  {
    log_trace("Message received from topic: '%s'", msg->get_topic().c_str());
    //log_trace("Payload to string : '%s'", msg->get_payload_str().c_str());

    // Call message listener with a mqtt message to Message convertion
    messageListener(Message{msg->get_payload_str()});
  }

  static auto getCorrelationId(const Message& message) -> std::string
  {
    auto iterator = message.metaData().find(Message::CORRELATION_ID);
    if (iterator == message.metaData().end() || iterator->second == "")
    {
      throw MessageBusException("Request must have a correlation id.");
    }
    return iterator->second;
  }

  static auto getReplyQueue(const Message& message) -> std::string
  {
    auto iterator = message.metaData().find(Message::REPLY_TO);
    if (iterator == message.metaData().end() || iterator->second == "")
    {
      throw MessageBusException("Request must have a reply queue.");
    }
    std::string queue(iterator->second);
    return {queue + messagebus::MQTT_DELIMITER + getCorrelationId(message)};
  }

} // namespace

namespace messagebus
{
  /////////////////////////////////////////////////////////////////////////////

  using duration = int64_t;
  duration KEEP_ALIVE = 20;
  auto constexpr QOS = mqtt::ReasonCode::GRANTED_QOS_1;
  auto constexpr TIMEOUT = std::chrono::seconds(10);

  MqttMessageBus::~MqttMessageBus()
  {
    // Cleaning all async clients
    if (m_client->is_connected())
    {
      log_debug("Cleaning: %s", m_clientName.c_str());
      m_client->disable_callbacks();
      m_client->stop_consuming();
      m_client->disconnect()->wait();
    }
  }

  void MqttMessageBus::connect()
  {
    mqtt::create_options opts(MQTTVERSION_5);

    m_client = std::make_shared<mqtt::async_client>(m_endpoint, messagebus::getClientId("etn"), opts);

    auto connOpts = mqtt::connect_options_builder()
                      .clean_session()
                      .mqtt_version(MQTTVERSION_5)
                      .keep_alive_interval(std::chrono::seconds(KEEP_ALIVE))
                      .automatic_reconnect(true)
                      //.automatic_reconnect(std::chrono::seconds(2), std::chrono::seconds(30))
                      .clean_start(true)
                      .finalize();

    m_client->set_connection_lost_handler([this](const std::string& cause) {
      MqttMessageBus::onConnectionLost(cause);
    });

    m_client->set_connected_handler([this](const std::string& cause) {
      MqttMessageBus::onConnected(cause);
    });

    m_client->set_update_connection_handler([this](const mqtt::connect_data& connData) {
      return MqttMessageBus::onConnectionUpdated(connData);
    });

    try
    {
      // Start consuming _before_ connecting, because we could get a flood
      // of stored messages as soon as the connection completes since
      // we're using a persistent (non-clean) session with the broker.
      m_client->start_consuming();
      mqtt::token_ptr conntok = m_client->connect(connOpts);
      conntok->wait();
      log_info("%s => connect status: %s", m_clientName.c_str(), m_client->is_connected() ? "true" : "false");
    }
    catch (const mqtt::exception& exc)
    {
      log_error("Error to connect with the Mqtt server, raison: %s", exc.get_error_str());
    }
  }

  // Callback called when connection lost.
  void MqttMessageBus::onConnectionLost(const std::string& cause)
  {
    log_error("Connection lost");
    if (!cause.empty())
    {
      log_error("raison: %s", cause.c_str());
    }
  }

  // Callback called for connection done.
  void MqttMessageBus::onConnected(const std::string& cause)
  {
    log_debug("Connected");
    if (!cause.empty())
    {
      log_debug("raison: %s", cause.c_str());
    }
  }

  // Callback called for connection updated.
  bool MqttMessageBus::onConnectionUpdated(const mqtt::connect_data& /*connData*/)
  {
    log_info("Connection updated");
    return true;
  }

  void MqttMessageBus::publish(const std::string& topic, const Message& message)
  {
    log_debug("Publishing on topic: %s", topic.c_str());
    mqtt::message_ptr pubmsg = mqtt::make_message(topic, message.serialize());
    pubmsg->set_qos(QOS);
    //mqtt::token_ptr tokPtr = m_client->publish(pubmsg);
    m_client->publish(pubmsg);
  }

  void MqttMessageBus::subscribe(const std::string& topic, MessageListener messageListener)
  {
    log_debug("Subscribing on topic: %s", topic.c_str());
    m_client->set_message_callback([messageListener](mqtt::const_message_ptr msg) {
      // Wrapper from mqtt msg to Message
      onMessageArrived(msg, messageListener);
    });
    m_client->subscribe(topic, QOS);
  }

  void MqttMessageBus::unsubscribe(const std::string& topic, MessageListener /*messageListener*/)
  {
    m_client->unsubscribe(topic)->wait();
    log_trace("%s - unsubscribed for topic '%s'", m_clientName.c_str(), topic.c_str());
  }

  void MqttMessageBus::receive(const std::string& queue, MessageListener messageListener)
  {
    m_client->set_message_callback([messageListener](mqtt::const_message_ptr msg) {
      log_debug("Received request from: %s", msg->get_topic().c_str());
      const mqtt::properties& props = msg->get_properties();
      if (props.contains(mqtt::property::RESPONSE_TOPIC) && props.contains(mqtt::property::CORRELATION_DATA))
      {

        mqtt::binary corrId = mqtt::get<std::string>(props, mqtt::property::CORRELATION_DATA);
        std::string replyTo = mqtt::get<std::string>(props, mqtt::property::RESPONSE_TOPIC);

        log_debug("Reply to: %s correlation data %s", replyTo.c_str(), corrId.c_str());
        // Wrapper from mqtt msg to Message
        onMessageArrived(msg, messageListener);
      }
      else
      {
        log_error("Missing mqtt properties for Req/Rep (i.e. CORRELATION_DATA or RESPONSE_TOPIC");
      }
    });

    log_debug("Waiting to receive request from: %s", queue.c_str());
    m_client->subscribe(queue, QOS);
  }

  void MqttMessageBus::sendRequest(const std::string& requestQueue, const Message& message)
  {
    if (m_client)
    {
      std::string queue(getReplyQueue(message));
      std::string correlationId(getCorrelationId(message));

      std::string replyQueue{queue + messagebus::MQTT_DELIMITER + correlationId};

      log_debug("Request queue: %s, reply queue", requestQueue.c_str(), replyQueue.c_str());

      mqtt::properties props{
        {mqtt::property::RESPONSE_TOPIC, replyQueue},
        {mqtt::property::CORRELATION_DATA, correlationId}};

      auto reqMsg = mqtt::message_ptr_builder()
                      .topic(requestQueue)
                      .payload(message.serialize())
                      .qos(QOS)
                      .properties(props)
                      .finalize();

      m_client->publish(reqMsg); //->wait_for(TIMEOUT);
      log_debug("Request sent");
    }
  }

  void MqttMessageBus::sendRequest(const std::string& requestQueue, const Message& message, MessageListener messageListener)
  {
    auto replyTo = getReplyQueue(message);

    receive(replyTo, messageListener);
    sendRequest(requestQueue, message);
  }

  void MqttMessageBus::sendReply(const std::string& replyQueue, const Message& message)
  {
    if (m_client)
    {
      log_debug("Sending reply to: %s", replyQueue.c_str());
      log_trace("Message serialized: %s", message.serialize().c_str());

      mqtt::properties props{
        {mqtt::property::RESPONSE_TOPIC, replyQueue},
        {mqtt::property::CORRELATION_DATA, getCorrelationId(message)}};

      mqtt::binary corrId = mqtt::get<std::string>(props, mqtt::property::CORRELATION_DATA);
      std::string replyTo = mqtt::get<std::string>(props, mqtt::property::RESPONSE_TOPIC);

      log_debug("Send reply to: %s correlation data %s", replyTo.c_str(), corrId.c_str());

      auto replyMsg = mqtt::message_ptr_builder()
                        .topic(replyQueue)
                        .payload(message.serialize())
                        .qos(QOS)
                        .properties(props)
                        .finalize();

      m_client->publish(replyMsg);
    }
  }

  Message MqttMessageBus::request(const std::string& requestQueue, const Message& message, int receiveTimeOut)
  {
    mqtt::const_message_ptr msg;
    auto replyTo = getReplyQueue(message);

    m_client->subscribe(replyTo, QOS);
    sendRequest(requestQueue, message);

    auto messagePresent = m_client->try_consume_message_for(&msg, std::chrono::seconds(receiveTimeOut));
    if (messagePresent)
    {
      return Message{msg->get_payload_str()};
    }
    else
    {
      throw MessageBusException("Request timed out of '" + std::to_string(receiveTimeOut) + "' seconds reached.");
    }
  }

} // namespace messagebus
