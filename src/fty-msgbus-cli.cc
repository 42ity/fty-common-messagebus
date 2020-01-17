/*  =========================================================================
    fty-msgbus-cli - description

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
    fty-msgbus-cli -
@discuss
@end
*/

#include "fty_common_messagebus_classes.h"

#include <sstream>
#include <iostream>
#include <unistd.h>
#include <csignal>
#include <mutex>

// Signal handler stuff.

volatile bool g_exit = false;
std::condition_variable g_cv;
std::mutex g_mutex;

void sigHandler(int)
{
    g_exit = true;
    g_cv.notify_one();
}

void setSignalHandler()
{
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = sigHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, nullptr);
}

// Command line parameters.

std::string endpoint, type, subject, topic, queue, destination, timeout = "5";
std::string clientName;
bool doMetadata = true;

void sendRequest(messagebus::MessageBus* msgbus, int argc, char** argv);
void receive(messagebus::MessageBus* msgbus, int argc, char** argv);
void subscribe(messagebus::MessageBus* msgbus, int argc, char** argv);

// Command line actions.

struct progAction {
    std::string arguments;
    std::string help;
    void(*fn)(messagebus::MessageBus*, int, char**);
} ;

const std::map<std::string, progAction> actions = {
    { "sendRequest", { "[userData]", "send a request with payload", sendRequest } },
    { "receive", { "", "listen on a queue and dump out received messages", receive } },
    { "subscribe", { "", "listen on a topic and dump out received messages", subscribe } },
} ;

const std::map<std::string, std::function<messagebus::MessageBus*()>> busTypes = {
    { "malamute", []() -> messagebus::MessageBus* { return messagebus::MlmMessageBus(endpoint, clientName); } },
} ;

// fty_proto_t and message dumper functions.

using Dumper = std::function<void(std::ostream&)>;

template <typename T>
Dumper s_makeDumper(fty_proto_t* proto, std::function<T(fty_proto_t*)> getter, std::function<void(T, std::ostream&)> dumper) {
    return std::bind(dumper, std::bind(getter, proto), std::placeholders::_1);
}

template <typename T>
void s_dumperToString(T t, std::ostream& os) {
    os << std::to_string(t) << "\n";
}

void s_dumperCstring(const char* str, std::ostream& os) {
    if (!str) {
        os << "(null)\n";
    }
    else {
        os << str << "\n";
    }
}

void dumperZlist(zlist_t* zlist, std::ostream& os) {
    if (!zlist) {
        os << "(null)\n";
    }
    else {
        os << "\n";
        char *item = (char *) zlist_first (zlist);
        while (item) {
            os << "\t\t";
            s_dumperCstring(item, os);
            item = (char *) zlist_next (zlist);
        }
    }
}

void s_dumperZhash(zhash_t* zhash, std::ostream& os) {
    if (!zhash) {
        os << "(null)\n";
    }
    else {
        os << "\n";
        char *item = (char *) zhash_first (zhash);
        while (item) {
            os << "\t\t" << zhash_cursor (zhash) << "=";
            s_dumperCstring(item, os);
            item = (char *) zhash_next (zhash);
        }
    }
}

void dumpFtyProto(fty_proto_t* proto, std::ostream& os) {
    std::map<std::string, std::function<void(std::ostream& os)>> properties {
        { "aux",            s_makeDumper<zhash_t*>     (proto, fty_proto_aux,         s_dumperZhash) },
        { "time",           s_makeDumper<uint64_t>     (proto, fty_proto_time,        s_dumperToString<uint64_t>) },
        { "ttl",            s_makeDumper<uint32_t>     (proto, fty_proto_ttl,         s_dumperToString<uint32_t>) },
        { "type",           s_makeDumper<const char*>  (proto, fty_proto_type,        s_dumperCstring) },
        { "name",           s_makeDumper<const char*>  (proto, fty_proto_name,        s_dumperCstring) },
        { "value",          s_makeDumper<const char*>  (proto, fty_proto_value,       s_dumperCstring) },
        { "unit",           s_makeDumper<const char*>  (proto, fty_proto_unit,        s_dumperCstring) },
        { "rule",           s_makeDumper<const char*>  (proto, fty_proto_rule,        s_dumperCstring) },
        { "state",          s_makeDumper<const char*>  (proto, fty_proto_state,       s_dumperCstring) },
        { "severity",       s_makeDumper<const char*>  (proto, fty_proto_severity,    s_dumperCstring) },
        { "description",    s_makeDumper<const char*>  (proto, fty_proto_description, s_dumperCstring) },
        { "action",         s_makeDumper<zlist_t*>     (proto, fty_proto_action,      dumperZlist) },
        { "operation",      s_makeDumper<const char*>  (proto, fty_proto_operation,   s_dumperCstring) },
        { "ext",            s_makeDumper<zhash_t*>     (proto, fty_proto_ext,         s_dumperZhash) },
    } ;

    os << "FTY_PROTO_" << fty_proto_command(proto) << "\n";
    for (const auto& property : properties) {
        os << "\t" << property.first << "=";
        property.second(os);
    }
}

void dumpMessage(const messagebus::Message& msg) {
    std::stringstream buffer;
    buffer << "--------------------------------------------------------------------------------\n";
    for (const auto& metadata : msg.metaData()) {
        buffer << "* " << metadata.first << ": " << metadata.second << "\n";
    }

    int cpt = 0;
    for (const auto & data : msg.userData()) {
        buffer << std::to_string(cpt) << ": ";

        fty_proto_t* proto = messagebus::decodeFtyProto(data);
        if (proto) {
            dumpFtyProto(proto, buffer);
        }
        else {
            buffer << data << "\n";
        }
        fty_proto_destroy(&proto);

        cpt++;
    }

    std::cout << buffer.str().c_str() << std::endl;
}

// Actions.

void receive(messagebus::MessageBus* msgbus, int argc, char** argv) {
    msgbus->receive(queue, [](messagebus::Message msg) { dumpMessage(msg); });

    // Wait until interrupt.
    setSignalHandler();
    std::unique_lock<std::mutex> lock(g_mutex);
    g_cv.wait(lock, [] { return g_exit; });
}

void subscribe(messagebus::MessageBus* msgbus, int argc, char** argv) {
    msgbus->subscribe(topic, [](messagebus::Message msg) { dumpMessage(msg); });

    // Wait until interrupt.
    setSignalHandler();
    std::unique_lock<std::mutex> lock(g_mutex);
    g_cv.wait(lock, [] { return g_exit; });
}

void sendRequest(messagebus::MessageBus* msgbus, int argc, char** argv) {
    messagebus::Message msg;

    // Build message metadata.
    if (doMetadata) {
        msg.metaData() =
        {
            { messagebus::Message::FROM, clientName },
            { messagebus::Message::REPLY_TO, clientName },
            { messagebus::Message::SUBJECT, subject },
            { messagebus::Message::CORRELATION_ID, messagebus::generateUuid() },
            { messagebus::Message::TO, destination },
            { messagebus::Message::TIMEOUT, timeout },
        };
    }

    // Build message payload.
    while (*argv) {
        msg.userData().emplace_back(*argv++);
    }

    dumpMessage(msg);
    msgbus->sendRequest(queue, msg);
}

[[noreturn]] void usage() {
    std::cerr << "Usage: fty-msgbus-cli [options] action ..." << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "\t-h                      this information" << std::endl;
    std::cerr << "\t-e endpoint             endpoint to connect to" << std::endl;
    std::cerr << "\t-s subject              subject of message" << std::endl;
    std::cerr << "\t-t topic                topic to use" << std::endl;
    std::cerr << "\t-T timeout              timeout to use" << std::endl;
    std::cerr << "\t-q queue                queue to use" << std::endl;
    std::cerr << "\t-d destination          destination (messagebus::Message::TO metadata)" << std::endl;
    std::cerr << "\t-x                      send message with no metadata (for old-school Malamute)" << std::endl;

    std::cerr << "\t-i type                 message bus type (";
    for (auto it = busTypes.begin(); it != busTypes.end(); it++) {
        if (it != busTypes.begin()) {
            std::cerr << ", ";
        }
        std::cerr << it->first;
    }
    std::cerr << ")" << std::endl;

    std::cerr << "\nActions:" << std::endl;
    for (const auto& i : actions) {
        int left = 24 - i.first.length() - i.second.arguments.length() - 2;
        std::cerr << "\t" << i.first << " " << i.second.arguments << std::string(left+1, ' ') << i.second.help << std::endl;
    }

    std::cerr << std::endl;
    exit(EXIT_FAILURE);
}

// Main.

int main(int argc, char** argv) {
    endpoint = MLM_DEFAULT_ENDPOINT;
    clientName = messagebus::getClientId("fty-msgbus-cli");
    type = "malamute";

    int c;
    while ((c = getopt(argc, argv, "he:s:t:T:q:d:xi:")) != -1) {
        switch (c) {
        case 'h':
            usage();
        case 'e':
            endpoint = optarg;
            break;
        case 's':
            subject = optarg;
            break;
        case 't':
            topic = optarg;
            break;
        case 'T':
            timeout = optarg;
            break;
        case 'q':
            queue = optarg;
            break;
        case 'd':
            destination = optarg;
            break;
        case 'x':
            doMetadata = false;
            break;
        case 'i':
            type = optarg;
            break;
        case ':':
            std::cerr << "Option -" << (char)optopt << " requires an operand" << std::endl;
            usage();
        case '?':
            std::cerr << "Unrecognized option: -" << (char)optopt << std::endl;
            usage();
        }
    }

    // Find bus.
    auto busIt = busTypes.find(type);
    if (busIt == busTypes.end()) {
        std::cerr << "Unknown message bus type '" << type << "'" << std::endl;
        usage();
    }

    // Find action.
    if (optind == argc) {
        std::cerr << "Action missing from arguments" << std::endl;
        usage();
    }
    auto actionIt = actions.find(argv[optind]);
    if (actionIt == actions.end()) {
        std::cerr << "Unknown action '" << argv[optind] << "'" << std::endl;
        usage();
    }

    // Do the requested work.
    auto msgBus = std::unique_ptr<messagebus::MessageBus>(busIt->second());
    msgBus->connect();
    actionIt->second.fn(msgBus.get(), argc-optind-1, argv+optind+1);

    return 0;
}
