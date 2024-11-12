#include <catch2/catch.hpp>
#include "fty_common_messagebus.h"
#include "../src/fty_common_messagebus_malamute.h" //MessageBusMalamute
#include "PingServer.h"
#include <czmq.h>
#include <memory>
#include <iostream>

TEST_CASE("MessageBus client/server")
{
    const std::string ENDPOINT("inproc://@/sync-request.test");
    const std::string PING_SERVER_NAME(messagebus::getClientId("ping-server"));
    const std::string PING_SERVER_QUEUE(PING_SERVER_NAME + ".queue");
    const std::string CLIENT_NAME(messagebus::getClientId("client"));

    // bind to mlm broker
    zactor_t* server = zactor_new(mlm_server, const_cast<char*>("Malamute"));
    REQUIRE(server);
    zstr_sendx(server, "BIND", ENDPOINT.c_str(), nullptr);
    //zstr_send(server, "VERBOSE");

    // instanciate a PING server
    auto pingServer = std::make_unique<PingServer>(ENDPOINT, PING_SERVER_NAME, PING_SERVER_QUEUE);
    REQUIRE(pingServer);

    // instanciate a client & connect
    messagebus::MessageBus* client = nullptr;
    REQUIRE_NOTHROW(client = messagebus::MlmMessageBus(ENDPOINT, CLIENT_NAME));
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
    //const std::string ENDPOINT("inproc://@/sync-request.test");
    const std::string ENDPOINT("ipc://@/malamute");
    const std::string PING_SERVER_NAME(messagebus::getClientId("ping-server"));
    const std::string PING_SERVER_QUEUE(PING_SERVER_NAME + ".queue");
    const std::string CLIENT_NAME(messagebus::getClientId("client"));

    // bind to mlm broker
    zactor_t* server = zactor_new(mlm_server, const_cast<char*>("Malamute"));
    REQUIRE(server);
    zstr_sendx(server, "BIND", ENDPOINT.c_str(), nullptr);
    zstr_send(server, "VERBOSE");

    // instanciate a PING server
    auto pingServer = std::make_unique<PingServer>(ENDPOINT, PING_SERVER_NAME, PING_SERVER_QUEUE);
    REQUIRE(pingServer);
    while(1) {
        usleep(100);
    }
    pingServer.reset(); // delete *before* server
    zactor_destroy(&server);
}*/

TEST_CASE("clients")
{
    const std::string ENDPOINT("inproc://@/sync-request-test");
    const std::string PING_SERVER_NAME(messagebus::getClientId("ping-server"));
    const std::string PING_SERVER_QUEUE(PING_SERVER_NAME + ".queue");
    const std::string CLIENT_NAME(messagebus::getClientId("client"));

    // bind to mlm broker
    zactor_t* server = zactor_new(mlm_server, const_cast<char*>("Malamute"));
    REQUIRE(server);
    zstr_sendx(server, "BIND", ENDPOINT.c_str(), nullptr);
    //zstr_send(server, "VERBOSE");

    // instanciate a PING server
    auto pingServer = std::make_unique<PingServer>(ENDPOINT, PING_SERVER_NAME, PING_SERVER_QUEUE);
    REQUIRE(pingServer);

    bool exceptionThrown = false;
    std::string TEST_title;

    try {
        // Create ping server
        /*auto pingServerFct = [&]() {

            auto pingServer = std::make_unique<PingServer>(ENDPOINT, PING_SERVER_NAME, PING_SERVER_QUEUE);
            REQUIRE(pingServer);
            while(1) {
                usleep(100);
            }
        };*/

        // Synchronous PING request
        auto sendSynch = [&](messagebus::MessageBus* clientExt, size_t num) {
            try {
                std::cout << "Start sendSynch " << num << std::endl;

                messagebus::MessageBus* client = nullptr; // local
                if (clientExt) {
                    client = clientExt;
                }
                else {
                    std::string clientName = CLIENT_NAME + "-" + std::to_string(num);
                    REQUIRE_NOTHROW(client = messagebus::MlmMessageBus(ENDPOINT, clientName));
                    REQUIRE(client);
                    REQUIRE_NOTHROW(client->connect());
                }

                const std::string clientName = dynamic_cast<messagebus::MessageBusMalamute*>(client)->clientName();
                const std::string subject = "PING";
                const int recvTimeoutSec{5};

                messagebus::Message msg;
                msg.metaData()[messagebus::Message::FROM] = clientName;
                msg.metaData()[messagebus::Message::SUBJECT] = subject;
                msg.metaData()[messagebus::Message::TO] = PING_SERVER_NAME;
                msg.metaData()[messagebus::Message::CORRELATION_ID] = messagebus::generateUuid() + "-" + std::to_string(num);

                // send request & recv reply
                std::cout << "REQUEST START " << num << std::endl;
                auto reply = client->request(PING_SERVER_QUEUE, msg, recvTimeoutSec);
                std::cout << "REQUEST END " << num << std::endl;

                REQUIRE(!reply.isOnError());
                REQUIRE(reply.userData() == messagebus::UserData({"PONG"}));

                if (client != clientExt) {
                    delete client;
                }

                std::cout << "End sendSynch " << num << std::endl;
            }
            catch (const std::exception& e) {
                std::cout << "EXCEPTION sendSynch " << num << ": " << e.what() << std::endl;
                exceptionThrown = true;
            }
        };

        //std::thread myThread(pingServerFct);
        //myThread.detach();

        TEST_title = "sendSync with unique client";
        if (1) {
            std::cout << "== " << TEST_title << std::endl;

            messagebus::MessageBus* client = nullptr;
            const std::string clientNameUnique = CLIENT_NAME + "-unique";
            REQUIRE_NOTHROW(client = messagebus::MlmMessageBus(ENDPOINT, clientNameUnique));
            REQUIRE(client);
            REQUIRE_NOTHROW(client->connect());

            size_t nbCalls = 10000;
            for (size_t i = 1; i <= nbCalls; i++) {
                sendSynch(client, i);
            }

            delete client;
        }

        TEST_title = "sendSync with multiple clients";
        if (1) {
            std::cout << "== " << TEST_title << std::endl;

            size_t nbCalls = 10000;
            for (size_t i = 1; i <= nbCalls; i++) {
                sendSynch(nullptr, i);
            }
        }

        TEST_title = "threaded sendSync with unique client";
        if (1) {
            std::cout << "== " << TEST_title << std::endl;

            messagebus::MessageBus* client = nullptr;
            const std::string clientNameUnique = CLIENT_NAME + "-unique";
            REQUIRE_NOTHROW(client = messagebus::MlmMessageBus(ENDPOINT, clientNameUnique));
            REQUIRE(client);
            REQUIRE_NOTHROW(client->connect());

            size_t nbThreads = 100;
            std::vector<std::thread> threads;
            for (size_t i = 1; i <= nbThreads; i++) {
                threads.push_back(std::thread(sendSynch, client, i));
            }

            size_t i = 1;
            for (auto& t : threads) {
                std::cout << "TEST " << i++ << std::endl;
                t.join();
            }

            delete client;
        }

        TEST_title = "threaded sendSync with multiple clients";
        if (1) {
            std::cout << "== " << TEST_title << std::endl;

            size_t nbThreads = 100;
            std::vector<std::thread> threads;
            for (size_t i = 1; i <= nbThreads; i++) {
                threads.push_back(std::thread(sendSynch, nullptr, i));
            }

            size_t i = 1;
            for (auto& t : threads) {
                std::cout << "TEST " << i++ << std::endl;
                t.join();
            }
        }
    }
    catch (const std::exception& e) {
        std::cout << "EXCEPTION TEST <" << TEST_title << ">: " << e.what() << std::endl;
        exceptionThrown = true;
    }

    REQUIRE(exceptionThrown == false);

    pingServer.reset(); // delete *before* server
    zactor_destroy(&server);
}
