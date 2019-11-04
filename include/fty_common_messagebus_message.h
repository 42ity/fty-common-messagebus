/*  =========================================================================
    fty_common_messagebus_message - class description

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

#ifndef FTY_COMMON_MESSAGEBUS_MESSAGE_H_INCLUDED
#define FTY_COMMON_MESSAGEBUS_MESSAGE_H_INCLUDED

#include <string>
#include <map>
#include <list>

namespace messagebus {

    using UserData = std::list<std::string>;
    using MetaData = std::map<std::string, std::string>;

    const static std::string STATUS_OK = "ok";
    const static std::string STATUS_KO = "ko";

    class Message {
      public:
        Message() = default;
        Message(const MetaData& metaData, const UserData& userData = {});
        ~Message() = default;

        const static std::string REPLY_TO;
        const static std::string CORRELATION_ID;
        const static std::string TO;
        const static std::string FROM;
        const static std::string SUBJECT;
        const static std::string STATUS;
        const static std::string TIMEOUT;

        MetaData& metaData();
        UserData& userData();

        const MetaData& metaData() const;
        const UserData& userData() const;
        const bool isOnError() const;

      private:
        MetaData m_metadata;
        UserData m_data;
    } ;

}

#endif