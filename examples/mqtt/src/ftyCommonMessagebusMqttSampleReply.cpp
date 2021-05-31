/*  =========================================================================
    ftyCommonMessagebusMqttSampleRep.cpp - description

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
    ftyCommonMessagebusMqttSampleRep.cpp -
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
#include <fty_log.h>
#include <ostream>
#include <iostream>
#include <thread>

messagebus::IMessageBus* replyer;
static bool _continue = true;

namespace
{
  static void signal_handler(int signal)
  {
    std::cout << "Signal " << signal << " received\n";
    _continue = false;
  }

  void replyerMessageListener(messagebus::Message message)
  {
    log_info("Replyer messageListener");

    messagebus::UserData reqData = message.userData();
    MathOperation mathQuery = MathOperation();
    reqData >> mathQuery;
    auto mathResultResult = MathResult("ok");

    if (mathQuery.operation == "add")
    {
      mathResultResult.result = std::to_string(std::stoi(mathQuery.param_1) + std::stoi(mathQuery.param_2));
    }
    else if (mathQuery.operation == "mult")
    {
      mathResultResult.result = std::to_string(std::stoi(mathQuery.param_1) * std::stoi(mathQuery.param_2));
    }
    else
    {
      mathResultResult.status = "unsuproted operation";
    }

    std::cout << "mathResultResult.result " << mathResultResult.result << std::endl;
    std::cout << "mathResultResult.status " << mathResultResult.status << std::endl;

    messagebus::Message response;
    messagebus::UserData responseData;

    responseData << mathResultResult;
    response.userData() = responseData;
    response.metaData().emplace(messagebus::Message::SUBJECT, "response");
    response.metaData().emplace(
      messagebus::Message::TO, message.metaData().find(messagebus::Message::FROM)->second);
    response.metaData().emplace(
      messagebus::Message::CORRELATION_ID, message.metaData().find(messagebus::Message::CORRELATION_ID)->second);

    std::string replyTo = message.metaData().find(messagebus::Message::REPLY_TO)->second + "/" + message.metaData().find(messagebus::Message::CORRELATION_ID)->second;
    replyer->sendReply(replyTo, response);

    _continue = false;
  }

} // namespace

int main(int /*argc*/, char** argv)
{
  log_info("%s - starting...", argv[0]);

  // Install a signal handler
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  std::string clientName = messagebus::getClientId("MqttSampleMathReplyer");

  replyer = messagebus::MqttMsgBus(messagebus::MQTT_END_POINT, clientName);
  replyer->connect();
  replyer->receive(messagebus::REQUEST_QUEUE, replyerMessageListener);

  while (_continue)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  delete replyer;

  log_info("%s - end", argv[0]);
  return EXIT_SUCCESS;
}
