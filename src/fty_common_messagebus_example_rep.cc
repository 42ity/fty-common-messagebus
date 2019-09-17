#include "fty_common_messagebus_classes.h"
#include <thread>
#include <chrono>

messagebus::MessageBus *receiver;

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
        FooBar fooBarr = FooBar("status::ok", fooBar.bar.c_str());
        messagebus::UserData data2;
        data2 << fooBarr;
        response.userData() = data2;
        response.metaData().emplace(messagebus::Message::SUBJECT, "response");
        response.metaData().emplace(messagebus::Message::TO, message.metaData().find(messagebus::Message::FROM)->second);
        response.metaData().emplace(messagebus::Message::COORELATION_ID, message.metaData().find(messagebus::Message::COORELATION_ID)->second);
        receiver->sendReply(message.metaData().find(messagebus::Message::REPLY_TO)->second, response);
    } else {
        log_info ("Old format, skip query...");
    }
}

volatile bool _continue = true;

void my_handler(int s){
    printf("Caught signal %d\n",s);
    _continue = false;
}

int main (int argc, char *argv [])
{
    log_info ("fty_common_messagebus_example - Binary");
    
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    
    const char *endpoint = "ipc://@/malamute";

    receiver = messagebus::connect(endpoint, "receiver");
    receiver->receive("doAction.queue.query", queryListener);
    
    do {

        std::this_thread::sleep_for (std::chrono::seconds(1));

    } while (_continue == true);

    delete receiver;

    log_info ("fty_common_messagebus_example - ");
    return 0;
}
