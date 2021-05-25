/*  =========================================================================
    fty_common_messagebus_example_req - Provides message bus for agents

    Copyright (C) 2019 - 2020 Eaton

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

/*! \file   fty_common_messagebus_example_req.cc
    \brief  Provides message bus for agents - example
    \author Jean-Baptiste Boric <Jean-BaptisteBORIC@Eaton.com>
    \author Xavier Millieret <XavierMillieret@eaton.com>
    \author Clement Perrette <clementperrette@eaton.com>
*/

#include "fty_common_messagebus_dto.h"
#include "fty_common_messagebus_exception.h"
#include "fty_common_messagebus_interface.h"
#include "fty_common_messagebus_message.h"
#include <chrono>
#include <fty_log.h>
#include <malamute.h>
#include <thread>

messagebus::IMessageBus* requester;

bool _continue = true;

void my_handler(int s)
{
    printf("Caught signal %d\n", s);
    _continue = false;
}

int main(int argc, char** argv)
{
    int total = 100;
    log_info("fty_common_messagebus_example_requester - Binary");
    if (argc > 1) {
        log_info("%s", argv[1]);
        total = atoi(argv[1]);
    }

    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    const char* endpoint = "ipc://@/malamute";

    std::string clientName = messagebus::getClientId("requester");

    requester = messagebus::MlmMessageBus(endpoint, clientName);
    requester->connect();

    int count = 0;
    int rcv   = 0;
    int loose = 0;
    do {
        time_t     rawtime;
        struct tm* timeinfo;
        char       buffer[80];

        time(&rawtime);
        timeinfo = localtime(&rawtime);

        strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", timeinfo);
        std::string str(buffer);

        // SYNC REQUEST
        messagebus::Message message;
        FooBar              query = FooBar("doAction", std::to_string(count));
        message.userData() << query;
        message.metaData().clear();
        message.metaData().emplace(messagebus::Message::SUBJECT, "query");
        message.metaData().emplace(messagebus::Message::FROM, clientName);
        message.metaData().emplace(messagebus::Message::TO, "receiver");
        message.metaData().emplace(messagebus::Message::CORRELATION_ID, messagebus::generateUuid());
        try {
            messagebus::Message resp = requester->request("doAction.queue.query", message, 5);
            log_info("Response:");
            for (const auto& pair : resp.metaData()) {
                log_info("  ** '%s' : '%s'", pair.first.c_str(), pair.second.c_str());
            }
            messagebus::UserData data = resp.userData();
            FooBar               fooBar;
            data >> fooBar;
            log_info("  * foo    : '%s'", fooBar.foo.c_str());
            log_info("  * bar    : '%s'", fooBar.bar.c_str());
            rcv++;
        } catch (messagebus::MessageBusException& ex) {
            log_error("%s", ex.what());
            loose++;
        }
        count++;

    } while (_continue == true && (count < total));

    log_info("**************************************************");
    log_info(" total  : %d", count);
    log_info(" receive: %d", rcv);
    log_info(" loose  : %d", loose);
    log_info("**************************************************");

    delete requester;

    log_info("fty_common_messagebus_example_requester - ");
    return 0;
}
