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

#include "mqtt/async_client.h"
#include "mqtt/properties.h"
//#include "fty_common_messagebus_message.h"

#include <fty_log.h>
#include <iostream>
#include <vector>

namespace
{

  /**
  * A base action listener.
  */
  class action_listener : public virtual mqtt::iaction_listener
  {
  protected:
    void on_failure(const mqtt::token& tok) override
    {
      std::cout << "\tListener failure for token: "
                << tok.get_message_id() << std::endl;
    }

    void on_success(const mqtt::token& tok) override
    {
      std::cout << "\tListener success for token: "
                << tok.get_message_id() << std::endl;
    }
  };

  /**
  * A callback class for use with the main MQTT client.
  */
  class callback : public virtual mqtt::callback,
                   public action_listener
  {
  public:
    void connection_lost(const std::string& cause) override
    {
      std::cout << "\nConnection lost" << std::endl;
      if (!cause.empty())
        std::cout << "\tcause: " << cause << std::endl;
    }

    void delivery_complete(mqtt::delivery_token_ptr tok) override
    {
      std::cout << "\tDelivery complete for token: "
                << (tok ? tok->get_message_id() : -1) << std::endl;
    }

    // Callback for when a message arrives.
    void message_arrived(mqtt::const_message_ptr msg) override
    {
      std::cout << "Message arrived" << std::endl;
      std::cout << "\ttopic: '" << msg->get_topic() << "'" << std::endl;
      std::cout << "\tpayload: '" << msg->to_string() << "'\n"
                << std::endl;
    }
  };

} // namespace

namespace messagebus
{
  /////////////////////////////////////////////////////////////////////////////

  using duration = int64_t;
  auto constexpr SERVER_ADDRESS{"tcp://localhost:1883"};
  auto constexpr CLIENT_ID{"rpc_math_srvr"};
  duration KEEP_ALIVE = 20;
  constexpr int QOS = mqtt::ReasonCode::GRANTED_QOS_1;

  auto constexpr TIMEOUT = std::chrono::seconds(10);
  const char* PAYLOAD1 = "Hello World!";

  MqttMessageBus::MqttMessageBus(const std::string& /*endpoint*/, const std::string& /*clientName*/)
  {
  }

  MqttMessageBus::~MqttMessageBus()
  {
    if (client->is_connected())
    {
      client->disable_callbacks();
      client->stop_consuming();
      client->disconnect()->wait();
    }
  }

  void MqttMessageBus::connect()
  {
    mqtt::create_options opts(MQTTVERSION_5);

    client = std::make_shared<mqtt::async_client>(SERVER_ADDRESS, CLIENT_ID, opts);
    auto connOpts = mqtt::connect_options_builder()
                      .clean_session()
                      .mqtt_version(MQTTVERSION_5)
                      .keep_alive_interval(std::chrono::seconds(KEEP_ALIVE))
                      .automatic_reconnect(std::chrono::seconds(2), std::chrono::seconds(30))
                      .clean_start(true)
                      .finalize();

    callback cb;
    client->set_callback(cb);
    try
    {
      // Start consuming _before_ connecting, because we could get a flood
      // of stored messages as soon as the connection completes since
      // we're using a persistent (non-clean) session with the broker.
      client->start_consuming();
      mqtt::token_ptr conntok = client->connect(connOpts);
      conntok->wait();
      log_info("Connect status: %b", client->is_connected());
    }
    catch (const mqtt::exception& exc)
    {
      log_error("Error to connect with the Mqtt server, raison: %s", exc.get_error_str());
    }
  }

  void MqttMessageBus::publish2(const std::string& topic, const std::string& message)
  {
    log_info("Publishing on topic: %s...", topic);
    mqtt::message_ptr pubmsg = mqtt::make_message(topic, message);
    pubmsg->set_qos(QOS);
    client->publish(pubmsg)->wait_for(TIMEOUT);
  }

  void subscribe(const std::string& /*topic*/, MessageListener /*messageListener*/)
  {

  }

  void MqttMessageBus::subscribe2(const std::string& topic /*, MessageListener messageListener*/)
  {
    log_info("Subscribing on topic: %s...", topic);
    client->subscribe(topic, QOS);
  }

  void MqttMessageBus::unsubscribe2(const std::string& topic /*, MessageListener messageListener*/)
  {
    client->unsubscribe(topic)->wait();
  }

  void MqttMessageBus::sendRequest2(const std::string& /*requestQueue*/, const std::string& /*message*/)
  {
    if (client)
    {
      std::string reqTopic = "requestQueue/test/";
      std::string repTopic = "repliesQueue/clientId";

      mqtt::token_ptr tokPtr = client->subscribe(repTopic, QOS);
      tokPtr->wait();

      if (int(tokPtr->get_reason_code()) != QOS)
      {
        log_error("Error: Server doesn't support reply QoS: %s", tokPtr->get_reason_code());
      }
      else
      {
        mqtt::properties props{
          {mqtt::property::RESPONSE_TOPIC, repTopic},
          {mqtt::property::CORRELATION_DATA, "1"}};

        std::string reqArgs{"requestTest"};

        auto pubmsg = mqtt::message_ptr_builder()
                        .topic(reqTopic)
                        .payload(reqArgs)
                        .qos(QOS)
                        .properties(props)
                        .finalize();

        client->publish(pubmsg)->wait_for(TIMEOUT);
      }
    }
  }

  void MqttMessageBus::sendReply2(const std::string& /*replyQueue*/, const std::string& /*message*/)
  {
    mqtt::create_options createOpts(MQTTVERSION_5);
    mqtt::client cli(SERVER_ADDRESS, CLIENT_ID, createOpts);

    auto connOpts = mqtt::connect_options_builder()
                      .mqtt_version(MQTTVERSION_5)
                      .keep_alive_interval(std::chrono::seconds(20))
                      .clean_start(true)
                      .finalize();
    try
    {
      const std::vector<std::string> topics{"requests/math", "requests/math/#"};
      const std::vector<int> qos{1, 1};

      cli.connect(connOpts);
      cli.subscribe(topics, qos);

      //auto msg = cli.try_consume_message_for(std::chrono::seconds(5));
      bool msg = true;
      if (msg)
      {
        // const mqtt::properties& props = msg->get_properties();
        // if (props.contains(mqtt::property::RESPONSE_TOPIC) && props.contains(mqtt::property::CORRELATION_DATA))
        // {
        //   mqtt::binary corr_id = mqtt::get<std::string>(props, mqtt::property::CORRELATION_DATA);
        //   std::string reply_to = mqtt::get<std::string>(props, mqtt::property::RESPONSE_TOPIC);
        //   auto reply_msg = mqtt::message::create(reply_to, "response", 1, false);
        //   cli.publish(reply_msg);
        // }

        // std::cout << "  Result: " << msg->to_string() << std::endl;
      }
      else
      {
        std::cerr << "Didn't receive a reply from the service." << std::endl;
      }
    }
    catch (const mqtt::exception& exc)
    {
      log_error("Error to send a reply, raison: %s", exc.get_error_str());
    }
  }

  void MqttMessageBus::receive(const std::string& /*queue*/, MessageListener /*messageListener*/)
  {
    // mqtt::create_options createOpts(MQTTVERSION_5);
    // mqtt::async_client cli(SERVER_ADDRESS, CLIENT_ID, createOpts);

    // auto connOpts = mqtt::connect_options_builder()
    //                   .mqtt_version(MQTTVERSION_5)
    //                   .keep_alive_interval(std::chrono::seconds(20))
    //                   .clean_start(true)
    //                   .finalize();
    // try
    // {
    //   cli.start_consuming();
    //   mqtt::token_ptr tok = cli.connect(connOpts);
    //   auto connRsp = tok->get_connect_response();

    //   std::string clientId = mqtt::get<std::string>(connRsp.get_properties(),
    //                                 mqtt::property::ASSIGNED_CLIENT_IDENTIFER);

    //   // So now we can create a unique RPC response topic using
    //   // the assigned (unique) client ID.

    //   std::string repTopic = "replies/" + clientId + "/math";
    //   tok = cli.subscribe(repTopic, QOS);
    //   tok->wait();
    // }
    // catch (const mqtt::exception& exc)
    // {
    //   log_error("Error to send a reply, raison: %s", exc.get_error_str());
    // }
  }

  //auto MqttMessageBus::request(const std::string& /*requestQueue*/, const std::string& /*message*/, int /*receiveTimeOut*/) -> std::string
  // Message MqttMessageBus::request(const std::string& /*requestQueue*/, const std::string& /*message*/, int /*receiveTimeOut*/)
  // {
  //   return Message{};
  // }

} // namespace messagebus
