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

#include "fty_common_messagebus_exception.h"
#include "fty_common_messagebus_interface.h"
#include <mqtt/client.h>
#include <mqtt/message.h>

#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <string>

namespace messagebus
{
  using ClientPointer = std::shared_ptr<mqtt::async_client>;

  typedef void(MqttMessageListenerFn)(const char*, const char*);
  using MqttMessageListener = std::function<MqttMessageListenerFn>;

  //class mycallback : public virtual mqtt::callback, public action_listener;

  class MqttMessageBus : public IMessageBus
  {
  public:
    MqttMessageBus(const std::string& endpoint, const std::string& clientName)
      : m_endpoint(endpoint)
      , m_clientName(clientName){};

    ~MqttMessageBus();

    void connect() override;

    //Async topic
    void publish(const std::string& topic, const Message& message) override;
    //void publish2(const std::string& topic, const std::string& message);
    void subscribe(const std::string& topic, MessageListener messageListener) override;
    void unsubscribe(const std::string& topic, MessageListener messageListener) override;

    // Async queue
    //void sendRequest(const std::string& requestQueue, const Message& message) override;
    void sendRequest2(const std::string& requestQueue, const std::string& message);
    //   void sendRequest(const std::string& requestQueue, const Message& message, MessageListener messageListener) override;
    //   void sendReply(const std::string& replyQueue, const Message& message) override;
    void sendReply2(const std::string& replyQueue, const std::string& message);
    void receive(const std::string& queue, MessageListener messageListener) override;

    // Sync queue
    //Message request(const std::string& requestQueue, const Message& message, int receiveTimeOut) override;
    //auto request(const std::string& requestQueue, const std::string& message, int receiveTimeOut) -> std::string;

  private:
    ClientPointer client;
    ClientPointer clientReqRep;

    std::string m_endpoint{};
    std::string m_clientName{};

    //   std::string m_publishTopic;

    std::map<std::string, MessageListener> m_subscriptions;

    mqtt::callback m_callBack;

    void onMessageArrived(mqtt::const_message_ptr msg);
  };
} // namespace messagebus

#endif // ifndef FTY_COMMON_MESSAGEBUS_MQTT
