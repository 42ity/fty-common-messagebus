/*  ====================================================================================================================
    message.h - Common message bus wrapper

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
    ====================================================================================================================
*/

#pragma once
#include <pack/pack.h>
#include <list>
#include <string>

// =====================================================================================================================

namespace messagebus {
class Message;
}

// =====================================================================================================================

namespace fty {

/// Common message bus message temporary wrapper
class Message : public pack::Node
{
public:
    enum class Status
    {
        Ok,
        Error
    };

    struct Meta : public pack::Node
    {
        pack::String         replyTo       = FIELD("reply-to");
        mutable pack::String from          = FIELD("from");
        mutable pack::String to            = FIELD("to");
        pack::String         subject       = FIELD("subject");
        pack::Enum<Status>   status        = FIELD("status");
        pack::String         timeout       = FIELD("timeout");
        mutable pack::String correlationId = FIELD("correlation-id");

        using pack::Node::Node;
        META(Meta, replyTo, from, to, subject, status, timeout, correlationId);
    };

    using Data = pack::StringList;

public:
    Meta meta     = FIELD("meta-data");
    Data userData = FIELD("user-data");

public:
    using pack::Node::Node;
    META(Message, userData, meta);

public:
    explicit Message(const messagebus::Message& msg);
    messagebus::Message toMessageBus() const;
    void setData(const std::string& data);
    void setData(const std::list<std::string>& data);
};

std::ostream& operator<<(std::ostream& ss, Message::Status status);
std::istream& operator>>(std::istream& ss, Message::Status& status);

} // namespace fty

// =====================================================================================================================
