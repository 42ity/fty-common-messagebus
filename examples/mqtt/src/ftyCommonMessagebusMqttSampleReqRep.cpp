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

#include "fty_common_messagebus_dto.h"
#include "fty_common_messagebus_exception.h"
#include "fty_common_messagebus_interface.h"
#include "fty_common_messagebus_message.h"
#include <chrono>
#include <experimental/filesystem>
#include <fty_log.h>
#include <thread>

auto constexpr MQTT_END_POINT{"tcp://localhost:1883"};
auto constexpr SAMPLE_TOPIC{"eaton.sample.pubsub"};

void messageListener(messagebus::Message message)
{
  log_info("Req messageListener:");
}

int main(int /*argc*/, char** /*argv*/)
{
  log_info("%s - starting...", __FILE__);

  std::string clientName = messagebus::getClientId("requester");

  auto requester = messagebus::MqttMsgBus(MQTT_END_POINT, "MqttRequester");
  requester->connect();

  messagebus::Message message;
  FooBar query = FooBar("doAction", std::to_string(0));
  message.userData() << query;
  message.metaData().clear();
  message.metaData().emplace(messagebus::Message::SUBJECT, "query");
  message.metaData().emplace(messagebus::Message::FROM, clientName);
  message.metaData().emplace(messagebus::Message::TO, "receiver");
  message.metaData().emplace(messagebus::Message::REPLY_TO, "replyQueue");
  message.metaData().emplace(messagebus::Message::CORRELATION_ID, messagebus::generateUuid());

  requester->sendRequest("requestQueue", message, messageListener);
  std::this_thread::sleep_for(std::chrono::seconds(5));

  delete requester;

  log_info("%s - end", __FILE__);
  return EXIT_SUCCESS;
}
