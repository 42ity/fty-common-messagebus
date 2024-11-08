#include <catch2/catch.hpp>
#include "fty_common_messagebus.h"
#include "PingServer.h"
#include <czmq.h>
#include <memory>
#include <iostream>

TEST_CASE("MessageBus client/server")
{
    const std::string MLM_ENDPOINT("inproc://@/sync-request.test");
    const std::string PING_SERVER_NAME(messagebus::getClientId("ping-server"));
    const std::string PING_SERVER_QUEUE(PING_SERVER_NAME + ".queue");
    const std::string CLIENT_NAME(messagebus::getClientId("client"));

    // bind to mlm broker
    zactor_t* server = zactor_new(mlm_server, const_cast<char*>("Malamute"));
    REQUIRE(server);
    zstr_sendx(server, "BIND", MLM_ENDPOINT.c_str(), nullptr);
    //zstr_send(server, "VERBOSE");

    // instanciate a PING server
    auto pingServer = std::make_unique<PingServer>(MLM_ENDPOINT, PING_SERVER_NAME, PING_SERVER_QUEUE);
    REQUIRE(pingServer);

    // instanciate a client & connect
    messagebus::MessageBus* client = nullptr;
    REQUIRE_NOTHROW(client = messagebus::MlmMessageBus(MLM_ENDPOINT, CLIENT_NAME));
    REQUIRE_NOTHROW(client->connect());

    const int recvTimeoutSec{5};
    std::string subject;

    subject = "PING";
    std::cout << "== sync request " << subject << std::endl;
    {
        messagebus::Message msg;
        msg.metaData()[messagebus::Message::FROM] = CLIENT_NAME;
        msg.metaData()[messagebus::Message::TO] = PING_SERVER_NAME;
        msg.metaData()[messagebus::Message::CORRELATION_ID] = messagebus::generateUuid();
        msg.metaData()[messagebus::Message::SUBJECT] = subject;

        // send request & recv reply
        msg = client->request(PING_SERVER_QUEUE, msg, recvTimeoutSec);

        REQUIRE(!msg.isOnError());
        REQUIRE(msg.userData() == messagebus::UserData({"PONG"}));
    }

    subject = "PING-KO";
    std::cout << "== sync request " << subject << std::endl;
    {
        messagebus::Message msg;
        msg.metaData()[messagebus::Message::FROM] = CLIENT_NAME;
        msg.metaData()[messagebus::Message::TO] = PING_SERVER_NAME;
        msg.metaData()[messagebus::Message::CORRELATION_ID] = messagebus::generateUuid();
        msg.metaData()[messagebus::Message::SUBJECT] = subject;

        // send request & recv reply
        msg = client->request(PING_SERVER_QUEUE, msg, recvTimeoutSec);

        REQUIRE(msg.isOnError());
        REQUIRE(msg.userData() == messagebus::UserData({}));
    }

    subject = "throw-timeout";
    std::cout << "== sync request " << subject << std::endl;
    {
        messagebus::Message msg;
        msg.metaData()[messagebus::Message::FROM] = CLIENT_NAME;
        msg.metaData()[messagebus::Message::TO] = PING_SERVER_NAME;
        msg.metaData()[messagebus::Message::CORRELATION_ID] = messagebus::generateUuid();
        msg.metaData()[messagebus::Message::SUBJECT] = subject;

        // send request, expect a timeout exception
        REQUIRE_THROWS(client->request(PING_SERVER_QUEUE, msg, recvTimeoutSec));
    }

    std::cout << "== subscribe/unsubscribe" << std::endl;
    {
        std::function<void(messagebus::Message)> listener = [](messagebus::Message m)
        {
            std::cout << "clientListener - userdata size: " << m.userData().size() << std::endl;
        };

        std::string topic1 = "topic1";
        REQUIRE_NOTHROW(client->subscribe(topic1, listener));
        std::string topic2 = "topic2";
        REQUIRE_NOTHROW(client->subscribe(topic2, listener));
        REQUIRE_NOTHROW(client->subscribe(topic2, listener));

        REQUIRE_NOTHROW(client->unsubscribe(topic1, listener));
        REQUIRE_NOTHROW(client->unsubscribe(topic2, listener));

        REQUIRE_THROWS(client->unsubscribe("toopic", listener));
        REQUIRE_THROWS(client->unsubscribe(topic1, listener));
        REQUIRE_THROWS(client->unsubscribe(topic2, listener));
    }

    std::cout << "== publish" << std::endl;
    {
        // activate pingServer listening
        const std::string PUBLICATION_TOPIC(messagebus::getClientId("publication-topic"));
        REQUIRE_NOTHROW(pingServer->listen(PUBLICATION_TOPIC));

        messagebus::Message msg;
        msg.metaData()[messagebus::Message::FROM] = CLIENT_NAME;
        msg.metaData()[messagebus::Message::SUBJECT] = "publication-subject";

        msg.userData() = messagebus::UserData({"arg1"});
        REQUIRE_NOTHROW(client->publish(PUBLICATION_TOPIC, msg));
        msg.userData() = messagebus::UserData({"hello", "world"});
        REQUIRE_NOTHROW(client->publish(PUBLICATION_TOPIC, msg));

        REQUIRE_THROWS(client->publish("toopic", msg));

        usleep(1000);
    }

    if (client) delete client;
    pingServer.reset(); // delete *before* server
    zactor_destroy(&server);
}

/*TEST_CASE("server")
{
    //const std::string MLM_ENDPOINT("inproc://@/sync-request.test");
    const std::string MLM_ENDPOINT("ipc://@/malamute");
    const std::string PING_SERVER_NAME(messagebus::getClientId("ping-server"));
    const std::string PING_SERVER_QUEUE(PING_SERVER_NAME + ".queue");
    const std::string CLIENT_NAME(messagebus::getClientId("client"));

    // bind to mlm broker
    zactor_t* server = zactor_new(mlm_server, const_cast<char*>("Malamute"));
    REQUIRE(server);
    zstr_sendx(server, "BIND", MLM_ENDPOINT.c_str(), nullptr);
    zstr_send(server, "VERBOSE");

    // instanciate a PING server
    auto pingServer = std::make_unique<PingServer>(MLM_ENDPOINT, PING_SERVER_NAME, PING_SERVER_QUEUE);
    REQUIRE(pingServer);
    while(1) {
        usleep(100);
    }
    pingServer.reset(); // delete *before* server
    zactor_destroy(&server);
}*/

TEST_CASE("clients")
{
    const std::string MLM_ENDPOINT("inproc://@/sync-request.test");
    //const std::string MLM_ENDPOINT("ipc://@/malamute");
    const std::string PING_SERVER_NAME(messagebus::getClientId("ping-server"));
    const std::string PING_SERVER_QUEUE(PING_SERVER_NAME + ".queue");
    const std::string CLIENT_NAME(messagebus::getClientId("client"));

    // bind to mlm broker
    zactor_t* server = zactor_new(mlm_server, const_cast<char*>("Malamute"));
    REQUIRE(server);
    zstr_sendx(server, "BIND", MLM_ENDPOINT.c_str(), nullptr);
    //zstr_send(server, "VERBOSE");

    // instanciate a PING server
    auto pingServer = std::make_unique<PingServer>(MLM_ENDPOINT, PING_SERVER_NAME, PING_SERVER_QUEUE);
    REQUIRE(pingServer);

    bool noThrow = true;
    try {
        // Create ping server
        /*auto pingServerFct = [&]() {
            
            auto pingServer = std::make_unique<PingServer>(MLM_ENDPOINT, PING_SERVER_NAME, PING_SERVER_QUEUE);
            REQUIRE(pingServer);
            while(1) {
                usleep(100);
            }
        };*/

        // Synchronous
        auto sendSynch = [&](int num) {
            try {
                std::cout << "Debut thread " << num << std::endl;
                std::string clientName = CLIENT_NAME + "-" + std::to_string(num);
                messagebus::MessageBus* client = nullptr;
                REQUIRE_NOTHROW(client = messagebus::MlmMessageBus(MLM_ENDPOINT, clientName));
                REQUIRE_NOTHROW(client->connect());

                const int recvTimeoutSec{5};
                std::string subject = "PING";

                messagebus::Message msg;
                msg.metaData()[messagebus::Message::FROM] = clientName;
                msg.metaData()[messagebus::Message::TO] = PING_SERVER_NAME;
                msg.metaData()[messagebus::Message::CORRELATION_ID] = messagebus::generateUuid() + std::to_string(num);
                msg.metaData()[messagebus::Message::SUBJECT] = subject;

                std::cout << "REQUEST DEBUT " << num << std::endl;
                // send request & recv reply
                msg = client->request(PING_SERVER_QUEUE, msg, recvTimeoutSec);
                std::cout << "REQUEST FIN " << num << std::endl;

                REQUIRE(!msg.isOnError());
                REQUIRE(msg.userData() == messagebus::UserData({"PONG"}));

                if (client) delete client;

                std::cout << "Fin thread " << num << std::endl;
            } catch (std::exception& e) {
                std::cout << "EXCEPTION THREAD " << num << ": " << e.what() << std::endl;
                noThrow = false;
            }
            //std::this_thread::sleep_for(1000ms);
        };

        //std::thread myThread(pingServerFct);
        //myThread.detach();

        int nb = 200;
        //std::vector<std::thread> myThreads;
        for (int i = 1; i <= nb; i++) {            
            sendSynch(i);
            //usleep(100);
        }

        /*int nbThread = 20;
        std::vector<std::thread> myThreads;
        for (int i = 1; i <= nbThread; i++) {
            // Synch version
            myThreads.push_back(std::thread(sendSynch, i));
        }

        int i = 0;
        for (auto& t : myThreads) {
            std::cout << "TEST " << i++ << std::endl;
            t.join();
        }*/
    } catch (std::exception& e) {
        std::cout << "EXCEPTION TEST: " << e.what() << std::endl;
        noThrow = false;
    }
    REQUIRE(noThrow);

    pingServer.reset(); // delete *before* server
    zactor_destroy(&server);
}
