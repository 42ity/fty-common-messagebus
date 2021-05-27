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
// To remove
#include "fty_common_messagebus_dto.h"

#include "mqtt/async_client.h"
#include "mqtt/properties.h"

#include <fty_log.h>
#include <iostream>
#include <vector>

namespace
{

  using namespace messagebus;

  // Callback called when a message arrives.
  static void onMessageArrived(mqtt::const_message_ptr msg, MessageListener /*messageListener*/)
  {
    log_trace("Message received from topic: '%s' ", msg->get_topic().c_str());
    std::cout << msg->get_payload().size() << std::endl;

    /*char* my_s_bytes =s reinterpret_cast<char*>(&msg->get_payload());*/
    /*mqtt::binary_ref&*/ auto ref = msg->get_payload_ref();

    //FooBar my_s_bytes = reinterpret_cast<FooBar>(ref);

    // FooBar fooBar;
    // msg->get_payload() >> fooBar;

    //std::cout << msg->get_payload << std::endl;
    //std::cout << msg->get_payload_str() << std::endl;

    // std::string arr[10];
    // std::copy(msg->get_payload().begin(), msg->get_payload().data().end(), arr);

    // Message message{};

    // // Meta data
    // message.metaData().emplace(Message::SUBJECT, msg->get_topic());
    // User data
    //message.userData().emplace_back(std::copy(msg->get_payload().begin(), msg->get_payload().end(), msg->get_payload().size()););

    //messageListener(message);
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
                      .automatic_reconnect(std::chrono::seconds(2), std::chrono::seconds(30))
                      .clean_start(true)
                      .finalize();

    // m_client->set_message_callback([](mqtt::const_message_ptr msg) {
    //   //MqttMessageBus::onMessageArrived(msg);
    //   std::cout << msg->get_payload_str() << std::endl;
    // });

    m_client->set_connection_lost_handler([this](const std::string& cause) {
      MqttMessageBus::onConnectionLost(cause);
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

  //Callback called for connection updated.
  bool MqttMessageBus::onConnectionUpdated(const mqtt::connect_data& /*connData*/)
  {
    log_info("Connection updates");
    return true;
  }

  void MqttMessageBus::publish(const std::string& topic, const Message& message)
  {
    log_debug("Publishing on topic: %s", topic.c_str());

    messagebus::UserData data = message.userData();
    FooBar fooBar;
    data >> fooBar;

    //mqtt::buffer_ref<messagebus::UserData> bufferRef(&data);
    //char* my_s_bytes = reinterpret_cast<char*>(&fooBar);
    mqtt::message_ptr pubmsg = mqtt::make_message(topic, static_cast<char*>(static_cast<void*>(&fooBar))); //, data.front().size(), QOS, false);
    //mqtt::message_ptr pubmsg = mqtt::make_message(topic, "");//, data.front().size(), QOS, false);
    //mqtt::message_ptr pubmsg = mqtt::make_message(topic, bufferRef); //, data.front().size(), QOS, false);

    pubmsg->set_qos(QOS);
    //mqtt::token_ptr tokPtr = m_client->publish(pubmsg);
    m_client->publish(pubmsg);

  }

  void MqttMessageBus::subscribe(const std::string& topic, MessageListener messageListener)
  {
    log_debug("Subscribing on topic: %s", topic.c_str());
    //m_subscriptions.emplace(topic, messageListener);
    m_client->set_message_callback([messageListener](mqtt::const_message_ptr msg) {
      // Wrapper from mqtt msg to Message
      onMessageArrived(msg, messageListener);
    });
    m_client->subscribe(topic, QOS);
  }

  void MqttMessageBus::unsubscribe(const std::string& topic, MessageListener /*messageListener*/)
  {
    // auto iterator = m_subscriptions.find(topic);
    // if (iterator == m_subscriptions.end())
    // {
    //   throw MessageBusException("Trying to unsubscribe on non-subscribed topic.");
    // }

    // m_subscriptions.erase(iterator);

    m_client->unsubscribe(topic)->wait();
    log_trace("%s - unsubscribed to topic '%s'", m_clientName.c_str(), topic.c_str());
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

        //auto replyMsg = mqtt::message::create(replyTo, "response", 1, false);
        // Wrapper from mqtt msg to Message
        onMessageArrived(msg, messageListener);
      }
    });

    log_debug("Waiting to receive request from: %s", queue.c_str());
    m_client->subscribe(queue, QOS);
  }

  void MqttMessageBus::sendRequest(const std::string& requestQueue, const Message& message)
  {
    if (m_client)
    {
      auto iterator = message.metaData().find(Message::REPLY_TO);
      if (iterator == message.metaData().end() || iterator->second == "")
      {
        throw MessageBusException("Request must have a reply queue.");
      }
      std::string queue(iterator->second);

      iterator = message.metaData().find(Message::CORRELATION_ID);
      if (iterator == message.metaData().end() || iterator->second == "")
      {
        throw MessageBusException("Request must have a correlation id.");
      }
      std::string correlationId(iterator->second);
      std::string replyQueue{queue + "/" + correlationId};

      log_debug("Send request to: %s", requestQueue.c_str());
      log_debug("Reply queue: %s", replyQueue.c_str());

      mqtt::properties props{
        {mqtt::property::RESPONSE_TOPIC, replyQueue},
        {mqtt::property::CORRELATION_DATA, correlationId}};

      std::string reqArgs{"requestTest"};

      auto pubmsg = mqtt::message_ptr_builder()
                      .topic(requestQueue)
                      .payload(reqArgs)
                      .qos(QOS)
                      .properties(props)
                      .finalize();

      m_client->publish(pubmsg); //->wait_for(TIMEOUT);
      log_debug("Request sent");
      //}
    }
  }

  void MqttMessageBus::sendRequest(const std::string& requestQueue, const Message& message, MessageListener messageListener)
  {
    auto iterator = message.metaData().find(Message::REPLY_TO);
    if (iterator == message.metaData().end() || iterator->second == "")
    {
      throw MessageBusException("Request must have a reply queue.");
    }
    std::string queue(iterator->second);
    iterator = message.metaData().find(Message::CORRELATION_ID);
    if (iterator == message.metaData().end() || iterator->second == "")
    {
      throw MessageBusException("Request must have a correlation id.");
    }

    receive(requestQueue, messageListener);
    sendRequest(requestQueue, message);
  }

  void MqttMessageBus::sendReply(const std::string& replyQueue, const Message& /*message*/)
  {
    if (m_client)
    {
      log_debug("Sending reply to: %s", replyQueue.c_str());
      auto replyMsg = mqtt::message::create(replyQueue, "message_to_string", 1, false);
      m_client->publish(replyMsg);
    }
  }

  Message MqttMessageBus::request(const std::string& /*requestQueue*/, const Message& /*message*/, int /*receiveTimeOut*/)
  {
    return Message{};
  }

} // namespace messagebus
