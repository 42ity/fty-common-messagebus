/*  =========================================================================
    fty_common_messagebus_interface - class description

    Copyright (C) 2014 - 2020 Eaton

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
    fty_common_messagebus_interface -
@discuss
@end
*/

#include "fty_common_messagebus_interface.h"
#include "fty/messagebus/mqtt/fty_common_messagebus_mqtt.hpp"
#include "fty_common_messagebus_malamute.h"
#include "fty_common_messagebus_message.h"

#include "jsoncpp/json/json.h"

#include <chrono>
#include <ctime>
#include <czmq.h>

namespace messagebus
{

  const std::string Message::REPLY_TO = "_replyTo";
  const std::string Message::CORRELATION_ID = "_correlationId";
  const std::string Message::FROM = "_from";
  const std::string Message::TO = "_to";
  const std::string Message::SUBJECT = "_subject";
  const std::string Message::STATUS = "_status";
  const std::string Message::TIMEOUT = "_timeout";
  const std::string Message::META_DATA = "metaData";
  const std::string Message::USER_DATA = "userData";

  Message::Message(const MetaData& metaData, const UserData& userData)
    : m_metadata(metaData)
    , m_data(userData)
  {
  }

  Message::Message(const std::string& input)
  {
    deSerialize(input);
  }

  MetaData& Message::metaData()
  {
    return m_metadata;
  }

  UserData& Message::userData()
  {
    return m_data;
  }

  const MetaData& Message::metaData() const
  {
    return m_metadata;
  }
  const UserData& Message::userData() const
  {
    return m_data;
  }

  bool Message::isOnError() const
  {
    bool returnValue = false;
    auto iterator = m_metadata.find(Message::STATUS);
    if (iterator != m_metadata.end() && STATUS_KO == iterator->second)
    {
      returnValue = true;
    }
    return returnValue;
  }

  auto Message::serialize() const -> std::string const
  {
    Json::Value root;

    // user values
    Json::Value userValues(Json::arrayValue);
    // Iterate over all user values.
    for (const auto& value : m_data)
    {
      userValues.append(value);
    }
    root[USER_DATA] = userValues;

    // Iterate over all meta data
    for (const auto& metadata : m_metadata)
    {
      root[META_DATA][metadata.first] = metadata.second;
    }
    return Json::writeString(Json::StreamWriterBuilder{}, root);
  }

  void Message::deSerialize(const std::string& input)
  {
    Json::Value root;
    Json::Reader reader;
    bool parsingStatus = reader.parse(input.c_str(), root);
    if (!parsingStatus)
    {
      std::cout << "Failed to parse " << reader.getFormattedErrorMessages() << std::endl;
    }
    else
    {
      // User data
      const Json::Value& userDataArray = root[USER_DATA];
      for (unsigned int i = 0; i < userDataArray.size(); i++)
      {
        m_data.push_back(userDataArray[i].asString());
      }

      // Meta data
      const Json::Value& metaDataObj = root[META_DATA];
      m_metadata.emplace(REPLY_TO, metaDataObj.get(REPLY_TO, "").asString());
      m_metadata.emplace(CORRELATION_ID, metaDataObj.get(CORRELATION_ID, "").asString());
      m_metadata.emplace(FROM, metaDataObj.get(FROM, "").asString());
      m_metadata.emplace(TO, metaDataObj.get(TO, "").asString());
      m_metadata.emplace(SUBJECT, metaDataObj.get(SUBJECT, "").asString());
      m_metadata.emplace(REPLY_TO, metaDataObj.get(STATUS, "").asString());
    }
  }

  // TODO remove this in helpers
  std::string generateUuid()
  {
    zuuid_t* uuid = zuuid_new();
    std::string strUuid(zuuid_str_canonical(uuid));
    zuuid_destroy(&uuid);
    return strUuid;
  }

  std::string getClientId(const std::string& prefix)
  {
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch());
    std::string clientId = prefix + "-" + std::to_string(ms.count());
    return clientId;
  }

  // TODO remove this and use template insteadof
  // IMessageBus* MlmMessageBus(const std::string& _endpoint, const std::string& _clientName) {
  //     return new messagebus::MessageBusMalamute(_endpoint, _clientName);
  // }

  IMessageBus* MqttMsgBus(const std::string& _endpoint, const std::string& _clientName)
  {
    return new messagebus::MqttMessageBus(_endpoint, _clientName);
  }
} // namespace messagebus
