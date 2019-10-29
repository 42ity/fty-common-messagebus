/*  =========================================================================
    fty_common_messagebus_interface - class description

    Copyright (C) 2014 - 2019 Eaton

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

#include "fty_common_messagebus_classes.h"
#include <ctime>
#include <chrono>

namespace messagebus {
    
    const std::string Message::REPLY_TO = "_replyTo";
    const std::string Message::COORELATION_ID = "_correlationId";
    const std::string Message::FROM = "_from";
    const std::string Message::TO = "_to";
    const std::string Message::SUBJECT = "_subject";
    const std::string Message::STATUS = "_status";
    const std::string Message::QUEUE = "_queue";
    const std::string Message::REMOTE_USER = "_remote_user";

    MetaData& Message::metaData() {
        return m_metadata;
    }
    
    UserData& Message::userData() {
        return m_data;
    }

    const MetaData& Message::metaData() const {
        return m_metadata;
    }
    const UserData& Message::userData() const {
        return m_data;
    }
    
    const bool Message::isOnError() const {
        bool returnValue = false;
        auto iterator = m_metadata.find(Message::STATUS);
        if( iterator != m_metadata.end() && STATUS_KO == iterator->second) {
            returnValue = true;
        }
        return returnValue;
    }

    std::string generateUuid() {
        zuuid_t *uuid = zuuid_new ();
        std::string strUuid(zuuid_str_canonical (uuid));
        zuuid_destroy(&uuid);
        return strUuid;
    }
    
    std::string getClientId(const std::string &prefix) {
        std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds >(
            std::chrono::system_clock::now().time_since_epoch()
        );
        std::string clientId = prefix  + "-" + std::to_string(ms.count());
        return clientId;
    }
    
    MessageBus* MlmMessageBus(const std::string& endpoint, const std::string& clientName) {
        return new messagebus::MessageBusMalamute(endpoint, clientName);
    }
}
