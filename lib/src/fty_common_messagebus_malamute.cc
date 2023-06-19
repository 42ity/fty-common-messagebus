/*  =========================================================================
    fty_common_messagebus_malamute - class description

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
    fty_common_messagebus_malamute -
@discuss
@end
*/

#include "fty_common_messagebus_malamute.h"
#include "fty_common_messagebus_message.h"

#include <fty_log.h>
#include <new>
#include <thread>

#define CONNECT_TIMEOUT_MS 1000
#define SENDTO_TIMEOUT_MS 5000

namespace messagebus {

    static std::string _popstrZmsg(zmsg_t* msg)
    {
        char* s = msg ? zmsg_popstr(msg) : nullptr;
        std::string value{s ? s : ""};
        zstr_free(&s);
        return value;
    }

    static Message _fromZmsg(zmsg_t* msg)
    {
        Message message;

        if (msg && (zmsg_size(msg) != 0))
        {
            std::string s = _popstrZmsg(msg);
            if (s == "__METADATA_START") {
                while (zmsg_size(msg) != 0) {
                    s = _popstrZmsg(msg);
                    if (s == "__METADATA_END") {
                        break;
                    }
                    std::string value = _popstrZmsg(msg);
                    message.metaData().emplace(s, value);
                }
            }
            else {
                message.userData().emplace_back(s);
            }

            while (zmsg_size(msg) != 0) {
                s = _popstrZmsg(msg);
                message.userData().emplace_back(s);
            }
        }

        return message;
    }

    static zmsg_t* _toZmsg(const Message& message)
    {
        zmsg_t* msg = zmsg_new();
        if (!msg) {
            log_error("zmsg_new() failed");
            return nullptr;
        }

        zmsg_addstr(msg, "__METADATA_START");
        for (const auto& pair : message.metaData()) {
            zmsg_addstr(msg, pair.first.c_str());
            zmsg_addstr(msg, pair.second.c_str());
        }
        zmsg_addstr(msg, "__METADATA_END");

        for (const auto& item : message.userData()) {
            zmsg_addstr(msg, item.c_str());
        }

        return msg;
    }

    MessageBusMalamute::MessageBusMalamute(const std::string& endpoint, const std::string& clientName)
        : m_endpoint(endpoint)
        , m_clientName(clientName)
    {
        // Create client
        m_client = mlm_client_new();
        if (!m_client) {
            log_error("%s - create mlm client failed", m_clientName.c_str());
            throw MessageBusException("Failed to create client.");
        }

        // Disables default SIGINT/SIGTERM handling in CZMQ
        zsys_handler_set(nullptr);
    }

    MessageBusMalamute::~MessageBusMalamute()
    {
        zactor_destroy(&m_actor);
        mlm_client_destroy(&m_client);
    }

    void MessageBusMalamute::connect()
    {
        if (!m_client) {
            log_error("%s - m_client not initialized", m_clientName.c_str());
            throw MessageBusException("Client not initialized.");
        }

        int r = mlm_client_connect(m_client, m_endpoint.c_str(), CONNECT_TIMEOUT_MS, m_clientName.c_str());
        if (r != 0) {
            throw MessageBusException("Connection failed.");
        }
        log_debug("%s - connection success", m_clientName.c_str());

        if (m_actor) {
            log_debug("%s - connect(): destroy previously created actor", m_clientName.c_str());
            zactor_destroy(&m_actor);
        }

        // Create listener thread.
        m_actor = zactor_new(listener, reinterpret_cast<void*>(this));
        if (!m_actor) {
            log_error("%s - create listener actor failed", m_clientName.c_str());
            throw MessageBusException("Failed to create listener actor.");
        }
        log_debug("%s - listener actor created", m_clientName.c_str());
    }

    void MessageBusMalamute::publish(const std::string& topic, const Message& message)
    {
        if (!m_client) {
            log_error("%s - m_client not initialized", m_clientName.c_str());
            throw MessageBusException("Client not initialized.");
        }

        if (m_publishTopic == "") { // first call
            int r = mlm_client_set_producer(m_client, topic.c_str());
            if (r != 0) {
                log_error("%s - set producer failed (topic: %s)", m_clientName.c_str(), topic.c_str());
                throw MessageBusException("Failed to set producer.");
            }
            m_publishTopic = topic;
            log_debug("%s - registered as stream producer (topic: %s)", m_clientName.c_str(), m_publishTopic.c_str());
        }
        else if (topic != m_publishTopic) {
            throw MessageBusException("Requires publishing to declared/unique topic.");
        }

        zmsg_t* msg = _toZmsg(message);
        if (!msg) {
            throw MessageBusException("Publish message is invalid.");
        }

        int r = mlm_client_send(m_client, m_publishTopic.c_str(), &msg);
        zmsg_destroy(&msg);
        if (r != 0) {
            log_error("%s - publish failed (topic: %s)", m_clientName.c_str(), m_publishTopic.c_str());
            throw MessageBusException("Failed to publish message.");
        }
        log_debug("%s - publish (topic: %s)", m_clientName.c_str(), m_publishTopic.c_str());
    }

    void MessageBusMalamute::subscribe(const std::string& topic, MessageListener messageListener)
    {
        if (!m_client) {
            log_error("%s - m_client not initialized", m_clientName.c_str());
            throw MessageBusException("Client not initialized.");
        }

        int r = mlm_client_set_consumer(m_client, topic.c_str(), ".*");
        if (r != 0) {
            log_error("%s - set consumer failed (topic: %s)", m_clientName.c_str(), topic.c_str());
            throw MessageBusException("Failed to set consumer.");
        }

        m_subscriptions.emplace(topic, messageListener);

        log_debug("%s - subscribed to topic '%s'", m_clientName.c_str(), topic.c_str());
    }

    void MessageBusMalamute::unsubscribe(const std::string& topic, MessageListener /*messageListener*/)
    {
        auto it = m_subscriptions.find(topic);
        if (it == m_subscriptions.end()) {
            throw MessageBusException("Trying to unsubscribe on non-subscribed topic.");
        }

        // Our current Malamute version is too old...
        log_warning("%s - mlm_client_remove_consumer() not implemented", m_clientName.c_str());

        m_subscriptions.erase(it);

        log_debug("%s - unsubscribed to topic '%s'", m_clientName.c_str(), topic.c_str());
    }

    void MessageBusMalamute::sendRequest(const std::string& requestQueue, const Message& message)
    {
        if (!m_client) {
            log_error("%s - m_client not initialized", m_clientName.c_str());
            throw MessageBusException("Client not initialized.");
        }

        auto it = message.metaData().find(Message::CORRELATION_ID);
        if (it == message.metaData().end() || it->second == "") {
            log_warning("%s - request should have a CORRELATION_ID field", m_clientName.c_str());
        }

        it = message.metaData().find(Message::REPLY_TO);
        if (it == message.metaData().end() || it->second == "") {
            log_warning("%s - request should have a REPLY_TO field", m_clientName.c_str());
        }

        std::string to = requestQueue; // default
        it = message.metaData().find(Message::TO);
        if (it == message.metaData().end() || it->second == "") {
            log_warning("%s - request should have a TO field", m_clientName.c_str());
        }
        else {
            to = it->second;
        }

        zmsg_t* msg = _toZmsg(message);
        if (!msg) {
            log_error("%s - sendRequest message is invalid.", m_clientName.c_str());
        }
        else {
            std::string subject = requestQueue;
            int r = mlm_client_sendto(m_client, to.c_str(), subject.c_str(), nullptr, SENDTO_TIMEOUT_MS, &msg);
            if (r != 0) {
                log_error("%s - sendRequest failed (to: %s, subject: %s)", m_clientName.c_str(), to.c_str(), subject.c_str());
            }
            else {
                log_debug("%s - sendRequest (to: %s, subject: %s)", m_clientName.c_str(), to.c_str(), subject.c_str());
            }
        }
        zmsg_destroy(&msg);
    }

    void MessageBusMalamute::sendRequest(const std::string& requestQueue, const Message& message, MessageListener messageListener)
    {
        if (!m_client) {
            log_error("%s - m_client not initialized", m_clientName.c_str());
            throw MessageBusException("Client not initialized.");
        }

        auto it = message.metaData().find(Message::REPLY_TO);
        if (it == message.metaData().end() || it->second == "") {
            throw MessageBusException("Request must have a REPLY_TO field.");
        }
        std::string recvQueue = it->second;

        receive(recvQueue, messageListener);
        sendRequest(requestQueue, message);
    }

    void MessageBusMalamute::sendReply(const std::string& replyQueue, const Message& message)
    {
        if (!m_client) {
            log_error("%s - m_client not initialized", m_clientName.c_str());
            throw MessageBusException("Client not initialized.");
        }

        auto it = message.metaData().find(Message::CORRELATION_ID);
        if (it == message.metaData().end() || it->second == "") {
            throw MessageBusException("Reply must have a CORRELATION_ID field.");
        }

        it = message.metaData().find(Message::TO);
        if (it == message.metaData().end() || it->second == "") {
            log_error("%s - Reply must have a TO field", m_clientName.c_str());
            throw MessageBusException("Reply must have a TO field.");
        }
        std::string to = it->second;

        zmsg_t* msg = _toZmsg(message);
        if (!msg) {
            log_error("%s - sendRequest message is invalid.", m_clientName.c_str());
        }
        else {
            std::string subject = replyQueue;
            int r = mlm_client_sendto(m_client, to.c_str(), subject.c_str(), nullptr, SENDTO_TIMEOUT_MS, &msg);
            if (r != 0) {
                log_error("%s - sendReply failed (to: %s, subject: %s)", m_clientName.c_str(), to.c_str(), subject.c_str());
            }
            else {
                log_debug("%s - sendReply (to: %s, subject: %s)", m_clientName.c_str(), to.c_str(), subject.c_str());
            }
        }
        zmsg_destroy(&msg);
    }

    void MessageBusMalamute::receive(const std::string& queue, MessageListener messageListener)
    {
        auto it = m_subscriptions.find(queue);
        if (it != m_subscriptions.end()) {
            throw MessageBusException("Already have queue map to listener");
        }

        m_subscriptions.emplace(queue, messageListener);

        log_trace("%s - receive from queue '%s'", m_clientName.c_str(), queue.c_str());
    }

    Message MessageBusMalamute::request(const std::string& requestQueue, const Message& message, int receiveTimeOutS)
    {
        if (!m_client) {
            log_error("%s - m_client not initialized", m_clientName.c_str());
            throw MessageBusException("Client not initialized.");
        }

        auto it = message.metaData().find(Message::CORRELATION_ID);
        if (it == message.metaData().end() || it->second == "") {
            throw MessageBusException("Request must have a CORRELATION_ID field.");
        }
        std::string syncUuid = it->second;

        it = message.metaData().find(Message::TO);
        if (it == message.metaData().end() || it->second == "") {
            throw MessageBusException("Request must have a TO field.");
        }
        std::string to = it->second;

        zmsg_t* msg = nullptr;
        {
            Message temp(message);
            // Complete metadata
            temp.metaData().emplace(Message::TIMEOUT, std::to_string(receiveTimeOutS));
            temp.metaData().emplace(Message::REPLY_TO, m_clientName);
            msg = _toZmsg(temp);
        }
        if (!msg) {
            throw MessageBusException("request msg is null");
        }

        std::unique_lock<std::mutex> lock(m_cv_mtx);
        m_syncUuid = syncUuid;

        std::string subject = requestQueue;
        int r = mlm_client_sendto(m_client, to.c_str(), subject.c_str(), nullptr, SENDTO_TIMEOUT_MS, &msg);
        zmsg_destroy(&msg);
        if (r != 0) {
            log_error("%s - Request failed (to: %s, subject: %s)", m_clientName.c_str(), to.c_str(), subject.c_str());
            m_syncUuid = "";
            throw MessageBusException("Request sendto failed");
        }
        log_debug("%s - Request (to: %s, subject: %s)", m_clientName.c_str(), to.c_str(), subject.c_str());

        if (m_cv.wait_for(lock, std::chrono::seconds(receiveTimeOutS)) == std::cv_status::timeout) {
            m_syncUuid = "";
            throw MessageBusException("Request timed out.");
        }

        return m_syncResponse;
    }

    void MessageBusMalamute::listener(zsock_t* pipe, void* arg)
    {
        if (!(pipe && arg)) {
            log_error("Listener - null pipe/arg parameter");
            throw MessageBusException("Listener receive null pipe or arg parameter.");
        }

        MessageBusMalamute* mbm = reinterpret_cast<MessageBusMalamute*>(arg);
        if (!mbm) {
            log_error("Listener - Invalid MessageBusMalamute* argument");
            throw MessageBusException("Listener MessageBusMalamute* cast failed.");
        }

        mbm->listenerMainloop(pipe);
    }

    void MessageBusMalamute::listenerMainloop(zsock_t* pipe)
    {
        zpoller_t* poller = zpoller_new(pipe, mlm_client_msgpipe(m_client), nullptr);
        if (!poller) {
            log_error("%s - zpoller_new() failed", m_clientName.c_str());
            return;
        }

        zsock_signal(pipe, 0);

        log_debug("%s - listener mainloop ready", m_clientName.c_str());

        const int POLL_TIMEOUT_MS = 10000;

        while (!zsys_interrupted) {
            void* which = zpoller_wait(poller, POLL_TIMEOUT_MS);

            if (!which) {
                if (zpoller_terminated(poller) || zsys_interrupted) {
                    break;
                }
            }
            else if (which == pipe) {
                zmsg_t* msg = zmsg_recv(pipe);
                std::string command = _popstrZmsg(msg);
                bool term{false};

                if (command == "$TERM") { // CZMQ $TERM command implementation
                    log_debug("%s - $TERM", m_clientName.c_str());
                    term = true;
                }
                else {
                    log_warning("%s - received '%s' on pipe, ignored", m_clientName.c_str(), command.c_str());
                }

                zmsg_destroy(&msg);
                if (term) {
                    break;
                }
            }
            else if (which == mlm_client_msgpipe(m_client)) {
                zmsg_t* msg = mlm_client_recv(m_client);
                if (!msg) {
                    log_error("%s - mlm_client_recv() returns null. Abort.", m_clientName.c_str());
                    break;
                }

                const char* subject = mlm_client_subject(m_client);
                const char* from = mlm_client_sender(m_client);
                std::string command{mlm_client_command(m_client)};

                if (command == "MAILBOX DELIVER") {
                    listenerHandleMailbox(subject, from, msg);
                }
                else if (command == "STREAM DELIVER") {
                    listenerHandleStream(subject, from, msg);
                }
                else {
                    log_warning("%s - unknown command '%s'", m_clientName.c_str(), command.c_str());
                }

                zmsg_destroy(&msg);
            }
        }

        zpoller_destroy(&poller);

        log_debug("%s - listener mainloop terminated", m_clientName.c_str());
    }

    void MessageBusMalamute::listenerHandleMailbox (const char* subject, const char* from, zmsg_t* msg)
    {
        log_debug("%s - received mailbox message from '%s' subject '%s'", m_clientName.c_str(), from, subject);

        Message message = _fromZmsg(msg);

        bool recvSyncResponse = false;

        if (m_syncUuid != "") {
            auto it = message.metaData().find(Message::CORRELATION_ID);
            if (it != message.metaData().end() && m_syncUuid == it->second) {
                std::unique_lock<std::mutex> lock(m_cv_mtx);
                m_syncResponse = message;
                m_cv.notify_one();
                m_syncUuid = "";
                recvSyncResponse = true;
            }
        }

        if (!recvSyncResponse) {
            auto it = m_subscriptions.find(subject);
            if (it != m_subscriptions.end()) {
                try {
                    (it->second)(message);
                }
                catch (const std::exception& e) {
                    log_error("%s - Error in listener of queue '%s': '%s'", m_clientName.c_str(), it->first.c_str(), e.what());
                }
                catch (...) {
                    log_error("%s - Error in listener of queue '%s': 'unknown error'", m_clientName.c_str(), it->first.c_str());
                }
            }
            else {
                log_warning("%s - Message skipped (from: %s, subject: %s)", m_clientName.c_str(), from, subject);
            }
        }
    }

    void MessageBusMalamute::listenerHandleStream (const char* subject, const char* from, zmsg_t* msg)
    {
        log_trace("%s - received stream message from '%s' subject '%s'", m_clientName.c_str(), from, subject);

        auto it = m_subscriptions.find(subject);
        if (it != m_subscriptions.end()) {
            try {
                Message message = _fromZmsg(msg);
                (it->second)(message);
            }
            catch (const std::exception& e) {
                log_error("%s - Error in listener of topic '%s': '%s'", m_clientName.c_str(), it->first.c_str(), e.what());
            }
            catch (...) {
                log_error("%s - Error in listener of topic '%s': 'unknown error'", m_clientName.c_str(), it->first.c_str());
            }
        }
    }

}
