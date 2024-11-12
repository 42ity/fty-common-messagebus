#pragma once

// messageBus simple PING server
// handle sync requests on queue:
//      subject = "PING": response = "PONG" (status OK)
//      subject = "PING-KO": response = "" (status KO)
//      else no response

#include <fty_common_messagebus_exception.h>
#include <fty_common_messagebus_message.h>
#include <fty_common_messagebus_interface.h>
#include <string>
#include <mutex>
#include <iostream>

class PingServer
{
public:
    PingServer() = delete;

    PingServer(const std::string& endpoint, const std::string& actorName, const std::string& queue)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cout << "== actor ctor" << std::endl;
        init(endpoint, actorName, queue);
    }

    ~PingServer()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cout << "== actor dtor" << std::endl;
        deinit();
    }

    void listen(const std::string& topic)
    {
        std::cout << "== actor listen: " << topic << std::endl;

        try {
            if (!m_client)
                throw std::runtime_error("client not initialized");

            // register stream handler
            m_client->subscribe(topic, [&](messagebus::Message m) {
                std::lock_guard<std::mutex> lock(m_mutex);
                handleStream(m);
            });
        }
        catch (const std::exception& e) {
            std::cerr << "== actor exception: " << e.what() << std::endl;
        }
    }

private: // methods
    void init(const std::string& endpoint, const std::string& actorName, const std::string& queue) noexcept
    {
        try {
            deinit();

            // create client & connect
            m_actorName = actorName;
            m_client = messagebus::MlmMessageBus(endpoint, m_actorName);
            if (!m_client) {
                throw std::runtime_error("MlmMessageBus() failed");
            }
            m_client->connect();

            // register request handler
            m_client->receive(queue, [&](messagebus::Message m) {
                std::lock_guard<std::mutex> lock(m_mutex);
                handleRequest(m);
            });
        }
        catch (const std::exception& e) {
            std::cerr << "== actor exception: " << e.what() << std::endl;
            deinit();
        }
    }

    void deinit() noexcept
    {
        m_actorName.clear();
        if (m_client) {
            delete m_client;
            m_client = nullptr;
        }
    }

    void handleRequest(const messagebus::Message& message) const noexcept
    {
        try {
            if (!m_client) {
                throw std::runtime_error("Client not initialized");
            }

            // incoming
            const std::string& from = message.metaData().at(messagebus::Message::FROM);
            const std::string& subject = message.metaData().at(messagebus::Message::SUBJECT);
            const std::string& cid = message.metaData().at(messagebus::Message::CORRELATION_ID);
            const std::string& replyTo = message.metaData().at(messagebus::Message::REPLY_TO);

            std::cout << "== actor request from " << from << " (subject: '" << subject << "')" << std::endl;

            // prepare response metaData
            messagebus::Message response;
            response.metaData()[messagebus::Message::SUBJECT] = subject;
            response.metaData()[messagebus::Message::FROM] = m_actorName;
            response.metaData()[messagebus::Message::TO] = from;
            response.metaData()[messagebus::Message::CORRELATION_ID] = cid;
            response.metaData()[messagebus::Message::STATUS] = messagebus::STATUS_OK;

            // handle request
            if (subject == "PING") {
                response.userData().push_back("PONG");
            }
            else if (subject == "PING-KO") {
                // response.isOnError() == true
                response.metaData()[messagebus::Message::STATUS] = messagebus::STATUS_KO;
            }
            else { // no reply
               throw std::runtime_error("subject '" + subject + "' not handled");
            }

            std::cout << "== actor reply" << std::endl;
            m_client->sendReply(replyTo, response);
        }
        catch (const std::exception& e) {
            std::cerr << "== actor request exception: " << e.what() << std::endl;
        }
        catch (...) {
            std::cerr << "== actor request unexpected exception" << std::endl;
        }
    }

    void handleStream(const messagebus::Message& message) const noexcept
    {
        try {
            // incoming (assume from & subject defined)
            const std::string& from = message.metaData().at(messagebus::Message::FROM);
            const std::string& subject = message.metaData().at(messagebus::Message::SUBJECT);

            std::cout << "== stream message from " << from << " (subject: '" << subject << "')" << std::endl;
            std::cout << "== stream message userdata size: " << message.userData().size()<< std::endl;
            int i = 0;
            for (const auto& s : message.userData()) {
                std::cout << "== stream message userdata " << (i++) << ": '" << s << "'" << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "== stream exception: " << e.what() << std::endl;
        }
        catch (...) {
            std::cerr << "== stream unexpected exception" << std::endl;
        }
    }

private: // members
    std::mutex m_mutex;
    messagebus::MessageBus* m_client{nullptr};
    std::string m_actorName;
}; // PingServer
