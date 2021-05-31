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
#include "FtyCommonMqttTestMathDto.h"
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
    log_info("Response arrived");
    messagebus::UserData data = message.userData();
    MathResult result;
    data >> result;
    log_info("  * status: '%s', result: %s", result.status.c_str(), result.result.c_str());

    _continue = false;
  }

} // namespace

int main(int argc, char** argv)
{
  if (argc < 4)
  {
    std::cout << "USAGE: " << argv[0] << " <add|mult> <num1> <num2>" << std::endl;
    return EXIT_FAILURE;
  }

  log_info("%s - starting...", argv[0]);

  // Install a signal handler
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  std::string clientName = messagebus::getClientId("MqttSampleMathRequester");
  std::string correlationId = messagebus::generateUuid();

  auto requester = messagebus::MqttMsgBus(messagebus::MQTT_END_POINT, clientName);
  requester->connect();

  messagebus::Message message;
  MathOperation query = MathOperation(argv[1], argv[2], argv[3]);
  message.userData() << query;
  message.metaData().clear();
  message.metaData().emplace(messagebus::Message::SUBJECT, "query");
  message.metaData().emplace(messagebus::Message::FROM, clientName);
  //message.metaData().emplace(messagebus::Message::TO, "receiver");
  message.metaData().emplace(messagebus::Message::REPLY_TO, messagebus::REPLY_QUEUE);
  message.metaData().emplace(messagebus::Message::CORRELATION_ID, correlationId);

  std::string replyTo = "ETN_Q_REPLY/" + correlationId;

  // Req/Rep call (in 2 times)
  // requester->receive(replyTo, responseMessageListener);
  // requester->sendRequest(messagebus::REQUEST_QUEUE, message);

  // Or Req/Rep call (in 1 times)
  requester->sendRequest(messagebus::REQUEST_QUEUE, message , responseMessageListener);

  while (_continue)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  delete requester;

  log_info("%s - end", argv[0]);
  return EXIT_SUCCESS;
}
