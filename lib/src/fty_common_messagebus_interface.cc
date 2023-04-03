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
#include "fty_common_messagebus_message.h"
#include "fty_common_messagebus_malamute.h"
#include <czmq.h>
#include <sstream>
#include <sys/random.h>

namespace messagebus {

    const std::string Message::REPLY_TO = "_replyTo";
    const std::string Message::CORRELATION_ID = "_correlationId";
    const std::string Message::FROM = "_from";
    const std::string Message::TO = "_to";
    const std::string Message::SUBJECT = "_subject";
    const std::string Message::STATUS = "_status";
    const std::string Message::TIMEOUT = "_timeout";

    Message::Message(const MetaData& metaData, const UserData& userData)
        : m_metadata(metaData)
        , m_data(userData)
    {
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
        auto it = m_metadata.find(Message::STATUS);
        return (it != m_metadata.end()) && (it->second == STATUS_KO);
    }

    //
    // helpers
    //

    // generate a random string of digits size
    // based on /dev/urandom generator
    static std::string srandom(int digits)
    {
        if (digits <= 0) {
            digits = 1;
        }

        std::ostringstream oss;

        while (digits > 0) {
            uint8_t byte{0};
            getrandom(&byte, 1, 0); // read /dev/urandom
            char s[3];
            snprintf(s, sizeof(s), "%02x", byte);
            oss << s[0];
            digits--;
            if (digits > 0) {
                oss << s[1];
                digits--;
            }
        }

        return oss.str();
    }

    std::string generateUuid()
    {
        zuuid_t* zuuid = zuuid_new();
        const char* uuid = zuuid ? zuuid_str_canonical(zuuid) : nullptr;
        std::string ret{uuid ? uuid : "<null-uuid>"};
        zuuid_destroy(&zuuid);
        return ret;
    }

    std::string getClientId(const std::string &prefix)
    {
        return prefix  + "-" + srandom(8);
    }

    MessageBus* MlmMessageBus(const std::string& endpoint, const std::string& clientName)
    {
        return new messagebus::MessageBusMalamute(endpoint, clientName);
    }
}
