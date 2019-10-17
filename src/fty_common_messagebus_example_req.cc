#include "fty_common_messagebus_classes.h"
#include <thread>
#include <chrono>

messagebus::MessageBus *requester;

bool _continue = true;

void my_handler(int s){
    printf("Caught signal %d\n",s);
    _continue = false;
}

int main (int argc, char *argv [])
{
    int total = 100;
    log_info ("fty_common_messagebus_example_requester - Binary");
    if( argc > 1 ) {
        log_info ("%s", argv[1]);
        total = atoi(argv[1]);
    }

    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    const char *endpoint = "ipc://@/malamute";

    std::string clientName = messagebus::getClientId("requester");

    requester = messagebus::MlmMessageBus(endpoint, clientName);
    requester->connect();

    int count = 0;
    int rcv = 0;
    int loose = 0;
    do {
        time_t rawtime;
        struct tm * timeinfo;
        char buffer[80];

        time (&rawtime);
        timeinfo = localtime(&rawtime);

        strftime(buffer,sizeof(buffer),"%d-%m-%Y %H:%M:%S",timeinfo);
        std::string str(buffer);

        // SYNC REQUEST
        messagebus::Message message;
        FooBar query = FooBar("doAction", std::to_string(count));
        message.userData() << query;
        message.metaData().clear();
        message.metaData().emplace(messagebus::Message::SUBJECT, "query");
        message.metaData().emplace(messagebus::Message::FROM, clientName);
        message.metaData().emplace(messagebus::Message::TO, "receiver");
        message.metaData().emplace(messagebus::Message::COORELATION_ID, messagebus::generateUuid());
        try{
            messagebus::Message resp = requester->request("doAction.queue.query", message, 5);
            log_info ("Response:");
            for (const auto& pair : resp.metaData()) {
                log_info ("  ** '%s' : '%s'", pair.first.c_str(), pair.second.c_str());
            }
            dto::UserData data = resp.userData();
            FooBar fooBar;
            data >> fooBar;
            log_info ("  * foo    : '%s'", fooBar.foo.c_str());
            log_info ("  * bar    : '%s'", fooBar.bar.c_str());
            rcv++;
        } catch (messagebus::MessageBusException& ex) {
            log_error("%s", ex.what());
            loose++;
        }
        count ++;

    } while (_continue == true && (count < total) );

    log_info ("**************************************************");
    log_info (" total  : %d", count);
    log_info (" receive: %d", rcv);
    log_info (" loose  : %d", loose);
    log_info ("**************************************************");

    delete requester;

    log_info ("fty_common_messagebus_example_requester - ");
    return 0;
}
