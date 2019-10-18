/*  =========================================================================
    fty_common_messagebus_example - description

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
    fty_common_messagebus_example -
@discuss
@end
*/

#include "fty_common_messagebus_classes.h"
#include <thread>
#include <chrono>

messagebus::MessageBus *receiver;
messagebus::MessageBus *publisher;

void messageListener(messagebus::Message message) {
    log_info ("messageListener:");
    messagebus::MetaData metadata = message.metaData();
    for (const auto& pair : message.metaData()) {
        log_info ("  ** '%s' : '%s'", pair.first.c_str(), pair.second.c_str());
    }
    dto::UserData data = message.userData();
    FooBar fooBar;
    data >> fooBar;
    log_info ("  * foo    : '%s'", fooBar.foo.c_str());
    log_info ("  * bar    : '%s'", fooBar.bar.c_str());
}

void queryListener(messagebus::Message message) {
    log_info ("queryListener:");
    for (const auto& pair : message.metaData()) {
        log_info ("  ** '%s' : '%s'", pair.first.c_str(), pair.second.c_str());
    }
    dto::UserData data = message.userData();
    FooBar fooBar;
    data >> fooBar;
    log_info ("  * foo    : '%s'", fooBar.foo.c_str());
    log_info ("  * bar    : '%s'", fooBar.bar.c_str());

    if( message.metaData().size() != 0 ) {
        messagebus::Message response;
        messagebus::MetaData metadata;
        FooBar fooBarr = FooBar("status", "ok");
        dto::UserData data2;
        data2 << fooBarr;
        response.userData() = data2;
        response.metaData().emplace(messagebus::Message::SUBJECT, "response");
        response.metaData().emplace(messagebus::Message::TO, message.metaData().find(messagebus::Message::FROM)->second);
        response.metaData().emplace(messagebus::Message::COORELATION_ID, message.metaData().find(messagebus::Message::COORELATION_ID)->second);
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
    dto::UserData data = message.userData();
    FooBar fooBar;
    data >> fooBar;
    log_info ("  * foo    : '%s'", fooBar.foo.c_str());
    log_info ("  * bar    : '%s'", fooBar.bar.c_str());
}

void responseListener2(messagebus::Message message) {
    log_info ("Specific responseListener:");
    for (const auto& pair : message.metaData()) {
        log_info ("  ** '%s' : '%s'", pair.first.c_str(), pair.second.c_str());
    }
    dto::UserData data = message.userData();
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
    receiver->subscribe("discovery", messageListener);
    receiver->receive("doAction.queue.query", queryListener);
    // old mailbox mecanism
    receiver->receive("receiver", queryListener);

    publisher = messagebus::MlmMessageBus(endpoint, "publisher");
    publisher->connect();
    publisher->receive("doAction.queue.response", responseListener);
    std::this_thread::sleep_for (std::chrono::seconds(2));

    // PUBLISH
    messagebus::Message message;
    FooBar hello = FooBar("event", "hello");
    message.userData() << hello;
    message.metaData().clear();
    message.metaData().emplace("mykey", "myvalue");
    message.metaData().emplace(messagebus::Message::FROM, "publisher");
    message.metaData().emplace(messagebus::Message::SUBJECT, "discovery");
    publisher->publish("discovery", message);
        std::this_thread::sleep_for (std::chrono::seconds(5));

    // PUBLISH EXCEPTION
    try{
    publisher->publish("discovery2", message);
    } catch (messagebus::MessageBusException& ex) {
        log_error("%s", ex.what());
    }

    // REQUEST
    messagebus::Message message2;
    FooBar query1 = FooBar("doAction", "actionNothing");
    message2.userData() << query1;
    message2.metaData().clear();
    message2.metaData().emplace(messagebus::Message::COORELATION_ID, messagebus::generateUuid());
    message2.metaData().emplace(messagebus::Message::SUBJECT, "doAction");
    message2.metaData().emplace(messagebus::Message::FROM, "publisher");
    message2.metaData().emplace(messagebus::Message::TO, "receiver");
    message2.metaData().emplace(messagebus::Message::REPLY_TO, "doAction.queue.response");
    publisher->sendRequest("doAction.queue.query", message2);
    std::this_thread::sleep_for (std::chrono::seconds(5));
    
    // REQUEST 2
    messagebus::Message message6;
    FooBar query4 = FooBar("doAction", "actionNothing2");
    message6.userData() << query4;
    message6.metaData().clear();
    message6.metaData().emplace(messagebus::Message::COORELATION_ID, messagebus::generateUuid());
    message6.metaData().emplace(messagebus::Message::SUBJECT, "doAction");
    message6.metaData().emplace(messagebus::Message::FROM, "publisher");
    message6.metaData().emplace(messagebus::Message::TO, "receiver");
    message6.metaData().emplace(messagebus::Message::REPLY_TO, "doAction.queue.response2");
    publisher->sendRequest("doAction.queue.query", message6, responseListener2);
    std::this_thread::sleep_for (std::chrono::seconds(5));

    // REQUEST WITHOUT METADATA
    messagebus::Message message3;
    FooBar query2 = FooBar("doAction2", "actionNothing");
    message3.userData() << query2;
    message3.metaData().clear();
    publisher->sendRequest("receiver", message3);
    std::this_thread::sleep_for (std::chrono::seconds(5));

    // SYNC REQUEST
    messagebus::Message message5;
    FooBar query3 = FooBar("doAction3", "wait");
    message5.userData() << query3;
    message5.metaData().clear();
    message5.metaData().emplace(messagebus::Message::SUBJECT, "sync query");
    message5.metaData().emplace(messagebus::Message::FROM, "publisher");
    message5.metaData().emplace(messagebus::Message::TO, "receiver");
    try{
        publisher->request("doAction.queue.query", message5, 15);
    } catch (messagebus::MessageBusException& ex) {
        log_error("%s", ex.what());
    }
    message5.metaData().emplace(messagebus::Message::COORELATION_ID, messagebus::generateUuid());
    messagebus::Message resp = publisher->request("doAction.queue.query", message5, 15);
    log_info ("Response sync:");
    for (const auto& pair : resp.metaData()) {
        log_info ("  ** '%s' : '%s'", pair.first.c_str(), pair.second.c_str());
    }
    dto::UserData data = resp.userData();
    FooBar fooBar;
    data >> fooBar;
    log_info ("  * foo    : '%s'", fooBar.foo.c_str());
    log_info ("  * bar    : '%s'", fooBar.bar.c_str());

    // PUBLISH WITHOUT METADATA
    messagebus::Message message4;
    FooBar bye  = FooBar("event", "bye");
    message4.userData() << bye;
    message4.metaData().clear();
    publisher->publish("discovery", message4);
    std::this_thread::sleep_for (std::chrono::seconds(5));

    delete publisher;
    delete receiver;

    log_info ("fty_common_messagebus_example - ");
    return 0;
}
