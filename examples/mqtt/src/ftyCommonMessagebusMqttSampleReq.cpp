/*  =========================================================================
    ftyCommonMessagebusMqttSamplesReqRep - description

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
    ftyCommonMessagebusMqttSamplesReqRep -
@discuss
@end
*/

#include "FtyCommonMqttTestDef.hpp"
#include "fty_common_messagebus_dto.h"
#include "fty_common_messagebus_exception.h"
#include "fty_common_messagebus_interface.h"
#include "fty_common_messagebus_message.h"

#include <chrono>
#include <csignal>
#include <experimental/filesystem>
#include <fty_log.h>
#include <iostream>
#include <thread>

messagebus::IMessageBus* requester;
static bool _continue = true;

namespace
{
  static void signal_handler(int signal)
  {
    std::cout << "Signal " << signal << " received\n";
    _continue = false;
  }

  void responseMessageListener(messagebus::Message message)
  {
    log_info("Requester messageListener");

    for (const auto& pair : message.metaData())
    {
      log_info("  ** '%s' : '%s'", pair.first.c_str(), pair.second.c_str());
    }
    messagebus::UserData data = message.userData();
    FooBar fooBar;
    data >> fooBar;
    log_info("  * foo    : '%s'", fooBar.foo.c_str());
    log_info("  * bar    : '%s'", fooBar.bar.c_str());
  }
} // namespace

int main(int /*argc*/, char** /*argv*/)
{
  log_info("%s - starting...", __FILE__);

  // Install a signal handler
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  std::string clientName = messagebus::getClientId("requester");

  requester = messagebus::MqttMsgBus(messagebus::MQTT_END_POINT, clientName);
  requester->connect();

  messagebus::Message message;
  FooBar query = FooBar("doAction", std::to_string(0));
  message.userData() << query;
  message.metaData().clear();
  message.metaData().emplace(messagebus::Message::SUBJECT, "query");
  message.metaData().emplace(messagebus::Message::FROM, clientName);
  message.metaData().emplace(messagebus::Message::TO, "receiver");
  message.metaData().emplace(messagebus::Message::REPLY_TO, messagebus::REPLY_QUEUE);
  message.metaData().emplace(messagebus::Message::CORRELATION_ID, messagebus::generateUuid());

  requester->sendRequest(messagebus::REQUEST_QUEUE, message, responseMessageListener);

  while (_continue)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  delete requester;

  log_info("%s - end", __FILE__);
  return EXIT_SUCCESS;
}
