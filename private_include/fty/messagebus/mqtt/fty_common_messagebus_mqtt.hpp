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

#ifndef FTY_COMMON_MESSAGEBUS_MQTT
#define FTY_COMMON_MESSAGEBUS_MQTT


#include "fty_common_messagebus_interface.h"
#include "fty/messagebus/mqtt/fty_common_messagebus_mqtt_call_back.hpp"
#include <mqtt/client.h>
#include <mqtt/message.h>

#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <string>

namespace messagebus
{
  // Default mqtt end point
  static auto constexpr DEFAULT_MQTT_END_POINT{"tcp://localhost:1883"};
  static auto constexpr SECURE_MQTT_END_POINT{"tcp://localhost:8883"};

  // Mqtt default delimiter
  static auto constexpr MQTT_DELIMITER{'/'};

  // Mqtt will topic
  static auto constexpr WILL_TOPIC{"/etn/t/service/status/"};

  // Mqtt will message
  static auto constexpr WILL_MSG{" died unexpectedly"};

  using ClientPointer = std::shared_ptr<mqtt::async_client>;

  class MqttMessageBus : public IMessageBus
  {
  public:
    MqttMessageBus(const std::string& endpoint, const std::string& clientName)
      : m_endpoint(endpoint)
      , m_clientName(clientName){};

    ~MqttMessageBus();

    void connect() override;

    // Pub/Sub pattern
    void publish(const std::string& topic, const Message& message) override;
    void subscribe(const std::string& topic, MessageListener messageListener) override;
    void unsubscribe(const std::string& topic, MessageListener messageListener = {}) override;

    // Req/Rep pattern
    void sendRequest(const std::string& requestQueue, const Message& message) override;
    void sendRequest(const std::string& requestQueue, const Message& message, MessageListener messageListener) override;
    void sendReply(const std::string& replyQueue, const Message& message) override;
    void receive(const std::string& queue, MessageListener messageListener) override;

    // Sync queue
    Message request(const std::string& requestQueue, const Message& message, int receiveTimeOut) override;

  private:
    ClientPointer m_client;

    std::string m_endpoint{};
    std::string m_clientName{};

    // Call back
    callback cb;

    void onMessageArrived(mqtt::const_message_ptr msg, MessageListener messageListener);
  };
} // namespace messagebus

#endif // ifndef FTY_COMMON_MESSAGEBUS_MQTT
