/*  =========================================================================
    message.cpp - Common message bus wrapper

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

#include "fty/message.h"
#include <fty_common_messagebus_message.h>

namespace fty {

// ===========================================================================================================

template <typename T>
struct identify
{
    using type = T;
};

template <typename K, typename V>
static V value(
    const std::map<K, V>& map, const typename identify<K>::type& key, const typename identify<V>::type& def = {})
{
    auto it = map.find(key);
    if (it != map.end()) {
        return it->second;
    }
    return def;
}

// ===========================================================================================================

Message::Message(const messagebus::Message& msg)
    : pack::Node::Node()
{
    meta.to            = value(msg.metaData(), messagebus::Message::TO);
    meta.from          = value(msg.metaData(), messagebus::Message::FROM);
    meta.replyTo       = value(msg.metaData(), messagebus::Message::REPLY_TO);
    meta.subject       = value(msg.metaData(), messagebus::Message::SUBJECT);
    meta.timeout       = value(msg.metaData(), messagebus::Message::TIMEOUT);
    meta.correlationId = value(msg.metaData(), messagebus::Message::CORRELATION_ID);

    meta.status.fromString(value(msg.metaData(), messagebus::Message::STATUS, "ok"));

    setData(msg.userData());
}

messagebus::Message Message::toMessageBus() const
{
    messagebus::Message msg;

    msg.metaData()[messagebus::Message::TO]             = meta.to;
    msg.metaData()[messagebus::Message::FROM]           = meta.from;
    msg.metaData()[messagebus::Message::REPLY_TO]       = meta.replyTo;
    msg.metaData()[messagebus::Message::SUBJECT]        = meta.subject;
    msg.metaData()[messagebus::Message::TIMEOUT]        = meta.timeout;
    msg.metaData()[messagebus::Message::CORRELATION_ID] = meta.correlationId;
    msg.metaData()[messagebus::Message::STATUS]         = meta.status.asString();

    for(const auto& el : userData) {
        msg.userData().emplace_back(el);
    }

    return msg;
}

void Message::setData(const std::string& data)
{
    userData.clear();
    userData.append(data);
}

void Message::setData(const std::list<std::string>& data)
{
    userData.clear();
    for(const auto& str : data) {
        userData.append(str);
    }
}

std::ostream& operator<<(std::ostream& ss, Message::Status status)
{
    switch (status) {
    case Message::Status::Ok:
        ss << "ok";
        break;
    case Message::Status::Error:
        ss << "ko";
        break;
    }
    return ss;
}

std::istream& operator>>(std::istream& ss, Message::Status& status)
{
    std::string str;
    ss >> str;
    if (str == "ok") {
        status = Message::Status::Ok;
    } else if (str == "ko") {
        status = Message::Status::Error;
    }
    return ss;
}

// ===========================================================================================================

} // namespace fty
