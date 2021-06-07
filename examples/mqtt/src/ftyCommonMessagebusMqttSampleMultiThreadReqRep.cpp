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
    ftyCommonMessagebusMqttSampleMultithred.cpp -
@discuss
@end
*/

#include "FtyCommonMqttTestDef.hpp"
#include "FtyCommonMqttTestMathDto.h"
#include "fty_common_messagebus_exception.h"
#include "fty_common_messagebus_exception.h"
#include "fty_common_messagebus_interface.h"
#include "fty_common_messagebus_message.h"

#include <mqtt/async_client.h>

#include <cctype>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fty_log.h>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>

namespace
{

  /////////////////////////////////////////////////////////////////////////////

  messagebus::IMessageBus* mqttMsgBus;
  messagebus::IMessageBus* mqttMsgBus2;
  static bool _continue = true;
  static auto correlationIdSniffer = std::map<std::string,std::string>();

  static auto getClientName() -> const std::string
  {
    return messagebus::getClientId("MqttSampleStress");
  }

  static void signalHandler(int signal)
  {
    std::cout << "Signal " << signal << " received\n";
    _continue = false;
  }

  static auto buildRandom(int min, int max) -> int
  {
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> uni(min, max);
    return uni(rng);
  }

  void mathOperationListener(const messagebus::Message& message)
  {
    log_info("Question arrived");

    messagebus::UserData reqData = message.userData();
    MathOperation mathQuery = MathOperation();
    reqData >> mathQuery;
    auto mathResultResult = MathResult();

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
      mathResultResult.status = MathResult::STATUS_KO;
      mathResultResult.result = "Unsuported operation";
    }

    messagebus::Message response;
    messagebus::UserData responseData;

    responseData << mathResultResult;
    response.userData() = responseData;
    response.metaData().emplace(messagebus::Message::SUBJECT, messagebus::ANSWER_USER_PROPERTY);
    response.metaData().emplace(messagebus::Message::FROM, message.metaData().find(messagebus::Message::FROM)->second);
    response.metaData().emplace(messagebus::Message::CORRELATION_ID, message.metaData().find(messagebus::Message::CORRELATION_ID)->second);
    response.metaData().emplace(messagebus::Message::REPLY_TO, message.metaData().find(messagebus::Message::REPLY_TO)->second);

    mqttMsgBus->sendReply(message.metaData().find(messagebus::Message::REPLY_TO)->second, response);

    //_continue = false;
  }

  void responseListener(const messagebus::Message& message)
  {
    log_info("Answer arrived");
    messagebus::UserData data = message.userData();
    MathResult result;
    data >> result;
    log_info("  * status: '%s', result: %s", result.status.c_str(), result.result.c_str());

    auto iterator = message.metaData().find(messagebus::Message::CORRELATION_ID);
    if (iterator == message.metaData().end() || iterator->second == "")
    {
      throw messagebus::MessageBusException("Reply error not correlationId");
    }
    auto correlationId = iterator->second;

    iterator = message.metaData().find(messagebus::Message::FROM);
    if (iterator == message.metaData().end() || iterator->second == "")
    {
      throw messagebus::MessageBusException("Reply error not from");
    }
    auto rand = iterator->second;

    iterator = correlationIdSniffer.find(correlationId);
    if (iterator == correlationIdSniffer.end() || iterator->second == "")
    {
      throw messagebus::MessageBusException("Error on correlationIdSniffer");
    }
    if (iterator->second == rand)
    {
      log_info("The answer is correct");
      correlationIdSniffer.erase(iterator);
    }
    else
    {
      throw messagebus::MessageBusException("Reply error the answer is not correlated to the id");
    }
  }

  void replyerFunc(messagebus::IMessageBus* messageBus/*, const messagebus::Message& message*/)
  {
    messageBus->receive(messagebus::REQUEST_QUEUE, mathOperationListener);

    // auto response = messagebus::Message();
    // auto mathResultResult = MathResult(MathResult::STATUS_KO, "not yet available");
    // messagebus::UserData responseData;

    // responseData << mathResultResult;
    // response.userData() = responseData;

    // auto randomSleep = buildRandom(1, 1000);
    // log_info("Sleepin for %d: ", randomSleep);
    // std::this_thread::sleep_for(std::chrono::milliseconds(randomSleep));
    // messageBus->sendReply(message.metaData().find(messagebus::Message::REPLY_TO)->second, response);
  }

  void requesterFunc(messagebus::IMessageBus* messageBus)
  {
    auto correlationId = messagebus::generateUuid();
    auto replyTo = messagebus::REPLY_QUEUE + '/' + correlationId;

    auto rand = std::to_string(buildRandom(1, 10));

    messagebus::Message message;
    MathOperation query = MathOperation("add", "1", rand);
    message.userData() << query;
    message.metaData().clear();
    message.metaData().emplace(messagebus::Message::SUBJECT, messagebus::QUERY_USER_PROPERTY);
    message.metaData().emplace(messagebus::Message::FROM, rand);
    message.metaData().emplace(messagebus::Message::REPLY_TO, replyTo);
    message.metaData().emplace(messagebus::Message::CORRELATION_ID, correlationId);

    correlationIdSniffer.emplace(correlationId, rand);
    mqttMsgBus->receive(replyTo, responseListener);

    //replyerFunc(mqttMsgBus);
    // mqttMsgBus->receive(messagebus::REQUEST_QUEUE, mathOperationListener);
    // mqttMsgBus->sendRequest(messagebus::REQUEST_QUEUE, message);
    mqttMsgBus->sendRequest(messagebus::REQUEST_QUEUE, message, mathOperationListener);
  }



} // namespace

int main(int /*argc*/, char** argv)
{
  log_info("%s - starting...", argv[0]);

  // Install a signal handler
  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);

  mqttMsgBus = messagebus::MqttMsgBus(messagebus::DEFAULT_MQTT_END_POINT, getClientName());
  mqttMsgBus->connect();

  mqttMsgBus2 = messagebus::MqttMsgBus(messagebus::DEFAULT_MQTT_END_POINT, getClientName() + "3");
  mqttMsgBus2->connect();

  requesterFunc(mqttMsgBus);

  //replyerFunc(mqttMsgBus2);


  //std::thread replyer(replyerFunc, mqttMsgBus);
  //std::thread requester(requesterFunc, mqttMsgBus);

  while (_continue)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Close the counter and wait for the publisher thread to complete
  log_info("Shutting down...");
  log_info("Sniffer count %d", correlationIdSniffer.size());
  //requester.join();
  //replyer.join();

  delete mqttMsgBus;

  log_info("%s - end", argv[0]);
  return EXIT_SUCCESS;
}
