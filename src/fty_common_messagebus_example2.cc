/*  =========================================================================
    fty_common_messagebus_example - description

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
    fty_common_messagebus_example -
@discuss
@end
*/

#include "fty_common_messagebus_classes.h"
#include <thread>
#include <chrono>

messagebus::MessageBus *receiver;
messagebus::MessageBus *publisher;

void queryListener(messagebus::Message message) {
    log_info ("queryListener:");
    for (const auto& pair : message.metaData()) {
        log_info ("  ** '%s' : '%s'", pair.first.c_str(), pair.second.c_str());
    }
    messagebus::UserData data = message.userData();
    FooBar fooBar;
    data >> fooBar;
    log_info ("  * foo    : '%s'", fooBar.foo.c_str());
    log_info ("  * bar    : '%s'", fooBar.bar.c_str());

    if( message.metaData().size() != 0 ) {
        messagebus::Message response;
        messagebus::MetaData metadata;
        FooBar fooBarr = FooBar("status", "ok");
        messagebus::UserData data2;
        data2 << fooBarr;
        response.userData() = data2;
        response.metaData().emplace(messagebus::Message::SUBJECT, "response");
        response.metaData().emplace(messagebus::Message::TO, message.metaData().find(messagebus::Message::FROM)->second);
        response.metaData().emplace(messagebus::Message::CORRELATION_ID, message.metaData().find(messagebus::Message::CORRELATION_ID)->second);
        if( fooBar.bar == "wait") {
                std::this_thread::sleep_for (std::chrono::seconds(10));
        }
        publisher->sendReply(message.metaData().find(messagebus::Message::REPLY_TO)->second, response);
    } else {
        log_info ("Old format, skip query...");
    }
}

void responseListener(messagebus::Message message) {
    log_info ("responseListener:");
    for (const auto& pair : message.metaData()) {
        log_info ("  ** '%s' : '%s'", pair.first.c_str(), pair.second.c_str());
    }
    messagebus::UserData data = message.userData();
    FooBar fooBar;
    data >> fooBar;
    log_info ("  * foo    : '%s'", fooBar.foo.c_str());
    log_info ("  * bar    : '%s'", fooBar.bar.c_str());
}

int main (int argc, char *argv [])
{
    log_info ("fty_common_messagebus_example - Binary");

    const char *endpoint = "ipc://@/malamute";

    receiver = messagebus::MlmMessageBus(endpoint, "receiver");
    receiver->connect();
    
    publisher = messagebus::MlmMessageBus(endpoint, "publisher");
    publisher->connect();
    
    receiver->receive("doAction.queue.query", queryListener);
    publisher->receive("doAction.queue.response", responseListener);
    std::this_thread::sleep_for (std::chrono::seconds(2));

    // REQUEST
    messagebus::Message message;
    FooBar query1 = FooBar("doAction", "wait");
    message.userData() << query1;
    message.metaData().clear();
    message.metaData().emplace(messagebus::Message::CORRELATION_ID, messagebus::generateUuid());
    message.metaData().emplace(messagebus::Message::SUBJECT, "doAction");
    message.metaData().emplace(messagebus::Message::FROM, "publisher");
    message.metaData().emplace(messagebus::Message::TO, "receiver");
    message.metaData().emplace(messagebus::Message::REPLY_TO, "doAction.queue.response");
    publisher->sendRequest("doAction.queue.query", message);
    std::this_thread::sleep_for (std::chrono::seconds(2));

    // REQUEST 2
    messagebus::Message message2;
    FooBar query2 = FooBar("doAction", "wait");
    message2.userData() << query2;
    message2.metaData().clear();
    message2.metaData().emplace(messagebus::Message::CORRELATION_ID, messagebus::generateUuid());
    message2.metaData().emplace(messagebus::Message::SUBJECT, "doAction");
    message2.metaData().emplace(messagebus::Message::FROM, "publisher");
    message2.metaData().emplace(messagebus::Message::TO, "receiver");
    message2.metaData().emplace(messagebus::Message::REPLY_TO, "doAction.queue.response");
    publisher->sendRequest("doAction.queue.query", message2);
    std::this_thread::sleep_for (std::chrono::seconds(15));

    delete publisher;
    delete receiver;

    log_info ("fty_common_messagebus_example - ");
    return 0;
}
