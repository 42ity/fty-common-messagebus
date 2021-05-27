/*  =========================================================================
    fty_common_messagebus_mqtt_example - description

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
    fty_common_messagebus_mqtt_example -
@discuss
@end
*/

#include "ftyCommonMqttTestDef.hpp"
#include "fty_common_messagebus_dto.h"
#include "fty_common_messagebus_exception.h"
#include "fty_common_messagebus_interface.h"
#include "fty_common_messagebus_message.h"

#include <chrono>
#include <csignal>
#include <fty_log.h>
#include <iostream>
#include <thread>

// messagebus::IMessageBus* receiver;
//messagebus::IMessageBus* publisher;

static bool _continue = true;

namespace
{
  static void signal_handler(int signal)
  {
    std::cout << "Signal " << signal << " received\n";
    _continue = false;
  }

  void messageListener(messagebus::Message message)
  {
    log_info("messageListener:");
    //     messagebus::MetaData metadata = message.metaData();
    //     for (const auto& pair : message.metaData()) {
    //         log_info("  ** '%s' : '%s'", pair.first.c_str(), pair.second.c_str());
    //     }
    // messagebus::UserData data = message.userData();
    // FooBar fooBar;
    // data >> fooBar;
    // log_info("  * foo    : '%s'", fooBar.foo.c_str());
    // log_info("  * bar    : '%s'", fooBar.bar.c_str());
  }
} // namespace

int main(int /*argc*/, char** /*argv*/)
{
  log_info("%s - starting...", __FILE__);

  // Install a signal handler
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  // receiver = messagebus::MlmMessageBus(endpoint, "receiver");
  // receiver->connect();
  // receiver->subscribe("discovery", messageListener);
  // receiver->receive("doAction.queue.query", queryListener);
  // // old mailbox mecanism
  // receiver->receive("receiver", queryListener);

  auto publisher = messagebus::MqttMsgBus(messagebus::MQTT_END_POINT, "MqttPublisher");
  publisher->connect();
  //publisher->subscribe(SAMPLE_TOPIC, messageListener);

  auto receiver = messagebus::MqttMsgBus(messagebus::MQTT_END_POINT, "MqttReceiver");
  receiver->connect();
  receiver->subscribe(messagebus::SAMPLE_TOPIC, messageListener);

  //std::this_thread::sleep_for(std::chrono::seconds(2));

  // // PUBLISH
  messagebus::Message message;
  FooBar hello = FooBar("event", "hello");
  message.userData() << hello;
  // message.metaData().clear();
  // message.metaData().emplace("mykey", "myvalue");
  // message.metaData().emplace(messagebus::Message::FROM, "publisher");
  // message.metaData().emplace(messagebus::Message::SUBJECT, "discovery");
  publisher->publish(messagebus::SAMPLE_TOPIC, message);
  // std::this_thread::sleep_for(std::chrono::seconds(5));

  // // PUBLISH WITHOUT METADATA
  // messagebus::Message message4;
  // FooBar              bye = FooBar("event", "bye");
  // message4.userData() << bye;
  // message4.metaData().clear();
  // publisher->publish("discovery", message4);

  while (_continue)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  delete publisher;
  delete receiver;

  log_info("%s - end", __FILE__);
  return EXIT_SUCCESS;
}
