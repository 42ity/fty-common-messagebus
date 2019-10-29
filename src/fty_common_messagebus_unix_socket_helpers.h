/*  =========================================================================
    fty_common_messagebus_unix_socket_helpers - class description

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

#ifndef FTY_COMMON_MESSAGEBUS_UNIX_SOCKET_HELPERS_H_INCLUDED
#define FTY_COMMON_MESSAGEBUS_UNIX_SOCKET_HELPERS_H_INCLUDED

#include "fty_common_messagebus_message.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

namespace messagebus
{
    Message recvMessage(int socket);
    void sendMessage(int socket, const Message & msg);
    void sendFrames(int socket, const std::vector<std::string> & payload);
    std::vector<std::string> recvFrames(int socket);
}

#endif
