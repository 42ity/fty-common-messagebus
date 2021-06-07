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
#include "fty_common_messagebus_exception.h"
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

  static auto getMqttPropertiesFromMetaData(const messagebus::MetaData& metaData) -> const mqtt::properties
  {
    auto props = mqtt::properties{};
    for (const auto& data : metaData)
    {
      if (data.first == Message::REPLY_TO)
      {
        std::string correlationId = metaData.find(Message::CORRELATION_ID)->second;
        props.add({mqtt::property::CORRELATION_DATA, correlationId});
        props.add({mqtt::property::RESPONSE_TOPIC, data.second});
      }
      else if (data.first != Message::CORRELATION_ID)
      {
        props.add({mqtt::property::USER_PROPERTY, data.first, data.second});
      }
    }
    return props;
  }

  static auto getCorrelationId(const Message& message) -> const std::string
  {
    auto iterator = message.metaData().find(Message::CORRELATION_ID);
    if (iterator == message.metaData().end() || iterator->second == "")
    {
      throw MessageBusException("Request must have a correlation id.");
    }
    return iterator->second;
  }

  static auto getReplyQueue(const Message& message) -> const std::string
  {
    auto iterator = message.metaData().find(Message::REPLY_TO);
    if (iterator == message.metaData().end() || iterator->second == "")
    {
      throw MessageBusException("Request must have a reply queue.");
    }
    //return {iterator->second + messagebus::MQTT_DELIMITER + getCorrelationId(message)};
    return iterator->second;
  }

} // namespace

namespace messagebus
{
  /////////////////////////////////////////////////////////////////////////////

  using duration = int64_t;
  duration KEEP_ALIVE = 20;
  static auto constexpr QOS = mqtt::ReasonCode::GRANTED_QOS_2;
  static auto constexpr RETAINED = false; //true;
  auto constexpr TIMEOUT = std::chrono::seconds(10);

  MqttMessageBus::~MqttMessageBus()
  {
    // Cleaning all async clients
    if (isServiceAvailable())
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

    // Connection options
    auto connOpts = mqtt::connect_options_builder()
                      .clean_session()
                      .mqtt_version(MQTTVERSION_5)
                      .keep_alive_interval(std::chrono::seconds(KEEP_ALIVE))
                      .automatic_reconnect(true)
                      //.automatic_reconnect(std::chrono::seconds(2), std::chrono::seconds(30))
                      .clean_start(true)
                      .will(mqtt::message{WILL_TOPIC + m_clientName, {m_clientName + WILL_MSG}, QOS, true})
                      .finalize();

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
      log_error("Error to connect with the Mqtt server, raison: %s", exc.get_message().c_str());
    }
  }

  auto MqttMessageBus::isServiceAvailable() -> const bool
  {
    return (m_client && m_client->is_connected());
  }


  // Callback called when a message arrives.
  void MqttMessageBus::onMessageArrived(mqtt::const_message_ptr msg, MessageListener messageListener)
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
  void MqttMessageBus::onReqRepMsgArrived(mqtt::const_message_ptr msg)
  {
    log_trace("Message received from topic: '%s'", msg->get_topic().c_str());
    // build metaData message from mqtt properties
    auto metaData = getMetaDataFromMqttProperties(msg->get_properties());

    // Call the right one
    if (msg->get_properties().contains(mqtt::property::RESPONSE_TOPIC))
    {

      auto responseTopic = mqtt::get<std::string>(msg->get_properties(), mqtt::property::RESPONSE_TOPIC);

      auto iterator = m_subscriptions.find(msg->get_topic());
      if (iterator != m_subscriptions.end())
      {
        try
        {
          (iterator->second)(Message{metaData, msg->get_payload_str()});
        }
        catch (const std::exception& e)
        {
          log_error("Error in listener of queue '%s': '%s'", iterator->first.c_str(), e.what());
        }
        catch (...)
        {
          log_error("Error in listener of queue '%s': 'unknown error'", iterator->first.c_str());
        }
      }
      else
      {
        log_warning("Message skipped for %s", responseTopic.c_str());
      }
    }
    else
    {
      log_error("no response topic");
    }
    // TODO do it but core dump in terminate?
    //MqttMessageBus::unsubscribe(msg->get_topic());
  }

  void MqttMessageBus::publish(const std::string& topic, const Message& message)
  {
    if (isServiceAvailable())
    {
      log_debug("Publishing on topic: %s", topic.c_str());
      // Adding all meta data inside mqtt properties
      auto props = getMqttPropertiesFromMetaData(message.metaData());
      // Build the message
      auto pubMsg = mqtt::message_ptr_builder()
                      .topic(topic)
                      .payload(message.serialize())
                      .qos(QOS)
                      .properties(props)
                      .retained(false)
                      .finalize();
      // Publish it
      m_client->publish(pubMsg);
    }
  }

  void MqttMessageBus::subscribe(const std::string& topic, MessageListener messageListener)
  {
    if (isServiceAvailable())
    {
      log_debug("Subscribing on topic: %s", topic.c_str());
      m_client->set_message_callback([this, messageListener](mqtt::const_message_ptr msg) {
        // Wrapper from mqtt msg to Message
        onMessageArrived(msg, messageListener);
      });
      m_client->subscribe(topic, QOS);
    }
  }

  void MqttMessageBus::unsubscribe(const std::string& topic, MessageListener /*messageListener*/)
  {
    if (isServiceAvailable())
    {
      log_trace("%s - unsubscribed for topic '%s'", m_clientName.c_str(), topic.c_str());
      m_client->unsubscribe(topic)->wait();
    }
  }

  void MqttMessageBus::receive(const std::string& queue, MessageListener messageListener)
  {
    if (isServiceAvailable())
    {
      if (m_subscriptions.find(queue) == m_subscriptions.end())
      {
        m_subscriptions.emplace(queue, messageListener);
        log_debug("m_subscriptions emplaced: %s %d", queue.c_str(), m_subscriptions.size());
      }

      m_client->set_message_callback([this](mqtt::const_message_ptr msg) {
        const mqtt::properties& props = msg->get_properties();
        if (/*props.contains(mqtt::property::RESPONSE_TOPIC) ||*/ props.contains(mqtt::property::CORRELATION_DATA))
        {
          // Wrapper from mqtt msg to Message
          onReqRepMsgArrived(msg);
        }
        else
        {
          log_error("Missing mqtt properties for Req/Rep (i.e. CORRELATION_DATA or RESPONSE_TOPIC");
        }
      });

      log_debug("Waiting to receive msg from: %s", queue.c_str());
      m_client->subscribe(queue, QOS);
    }
  }

  void MqttMessageBus::sendRequest(const std::string& requestQueue, const Message& message)
  {
    if (isServiceAvailable())
    {
      // Adding all meta data inside mqtt properties
      auto props = getMqttPropertiesFromMetaData(message.metaData());

      auto replyTo = mqtt::get<std::string>(props, mqtt::property::RESPONSE_TOPIC);
      // if (m_subscriptions.find(replyTo) == m_subscriptions.end())
      // {
      //   m_subscriptions.emplace(replyTo, messageListener);
      //   log_debug("m_subscriptions emplaced: %s %d", replyTo.c_str(), m_subscriptions.size());
      // }
      log_debug("Send request to: %s and wait to reply queue %s", requestQueue.c_str(), replyTo.c_str());

      auto reqMsg = mqtt::message_ptr_builder()
                      .topic(requestQueue)
                      .payload(message.serialize())
                      .qos(QOS)
                      .properties(props)
                      .retained(RETAINED)
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
    if (isServiceAvailable())
    {
      // Adding all meta data inside mqtt properties
      auto props = getMqttPropertiesFromMetaData(message.metaData());

      log_debug("Send reply to: %s", (mqtt::get<std::string>(props, mqtt::property::RESPONSE_TOPIC)).c_str());
      auto replyMsg = mqtt::message_ptr_builder()
                        .topic(replyQueue)
                        .payload(message.serialize())
                        .qos(QOS)
                        .properties(props)
                        .retained(RETAINED)
                        .finalize();

      m_client->publish(replyMsg);
    }
  }

  Message MqttMessageBus::request(const std::string& requestQueue, const Message& message, int receiveTimeOut)
  {
    if (isServiceAvailable())
    {
      mqtt::const_message_ptr msg;
      auto replyQueue = getReplyQueue(message);

      m_client->subscribe(replyQueue, QOS);
      sendRequest(requestQueue, message);
      auto messageArrived = m_client->try_consume_message_for(&msg, std::chrono::seconds(receiveTimeOut));
      if (messageArrived)
      {
        return Message{getMetaDataFromMqttProperties(msg->get_properties()), msg->get_payload_str()};
      }
      else
      {
        throw MessageBusException("Request timed out of '" + std::to_string(receiveTimeOut) + "' seconds reached.");
      }
      //m_client->unsubscribe(replyQueue);
    }
    return Message{};
  }

} // namespace messagebus
