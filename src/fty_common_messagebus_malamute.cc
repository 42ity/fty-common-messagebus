/*  =========================================================================
    fty_common_messagebus_malamute - class description

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
    fty_common_messagebus_malamute -
@discuss
@end
*/

#include "fty_common_messagebus_classes.h"

#include <new>
#include <thread>

namespace messagebus {

    static Message _fromZmsg(zmsg_t *msg) {
        Message message;
        zframe_t *item;

        if( zmsg_size(msg) ) {
            item = zmsg_pop(msg);
            std::string key((const char *)zframe_data(item), zframe_size(item));
            zframe_destroy(&item);
            if( key == "__METADATA_START" ) {
                while ((item = zmsg_pop(msg))) {
                    key = std::string((const char *)zframe_data(item), zframe_size(item));
                    zframe_destroy(&item);
                    if (key == "__METADATA_END") {
                        break;
                    }
                    zframe_t *zvalue = zmsg_pop(msg);
                    std::string value((const char *)zframe_data(zvalue), zframe_size(zvalue));
                    zframe_destroy(&item);
                    message.metaData().emplace(key, value);
                }
            }
            else {
                message.userData().emplace_back(key);
            }
            while ((item = zmsg_pop(msg))) {
                message.userData().emplace_back((const char *)zframe_data(item), zframe_size(item));
                zframe_destroy(&item);
            }
        }
        return message;
    }

    static zmsg_t* _toZmsg(const Message& message) {
        zmsg_t *msg = zmsg_new();
        // Add meta data in message only if raw option is not specified in meta data
        auto iterator = message.metaData().find(Message::RAW);
        if (iterator == message.metaData().end()) {
            zmsg_addstr(msg, "__METADATA_START");
            for (const auto& pair : message.metaData()) {
                zmsg_addmem(msg, pair.first.c_str(), pair.first.size());
                zmsg_addmem(msg, pair.second.c_str(), pair.second.size());
            }
            zmsg_addstr(msg, "__METADATA_END");
        }
        for(const auto& item : message.userData()) {
            zmsg_addmem(msg, item.c_str(), item.size());
        }

        return msg;
    }

    MessageBusMalamute::MessageBusMalamute(const std::string& endpoint, const std::string& clientName) {
        m_clientName = clientName;
        m_endpoint = endpoint;
        m_publishTopic = "";
        m_syncUuid = "";

        // Create Malamute connection.
        m_client = mlm_client_new();
        if (!m_client) {
            throw std::bad_alloc();
        }

        zsys_handler_set (nullptr);
    }

    MessageBusMalamute::~MessageBusMalamute() {
        zactor_destroy(&m_actor);
        mlm_client_destroy(&m_client);
    }


    void MessageBusMalamute::connect() {
        if (mlm_client_connect (m_client, m_endpoint.c_str(), 1000, m_clientName.c_str()) == -1) {
            throw MessageBusException("Failed to connect to Malamute server.");
        }
        log_trace ("%s - connected to Malamute server", m_clientName.c_str());

        // Create listener thread.
        m_actor = zactor_new (listener, reinterpret_cast<void*>(this));
        if (!m_actor) {
            throw std::bad_alloc();
        }
    }

    void MessageBusMalamute::publish(const std::string& topic, const Message& message) {
        if( m_publishTopic == "" ) {
            m_publishTopic = topic;
            if (mlm_client_set_producer (m_client, m_publishTopic.c_str()) == -1) {
                throw MessageBusException("Failed to set producer on Malamute connection.");
            }
            log_trace ("%s - registered as stream producter on '%s'", m_clientName.c_str(), m_publishTopic.c_str());
        }

        if( topic != m_publishTopic ) {
            throw MessageBusException("MessageBusMalamute requires publishing to declared topic.");
        }

        zmsg_t *msg = _toZmsg (message);
        log_trace ("%s - publishing on topic '%s'", m_clientName.c_str(), m_publishTopic.c_str());
        mlm_client_send (m_client, topic.c_str(), &msg);
    }

    void MessageBusMalamute::subscribe(const std::string& topic, MessageListener messageListener) {
        if (mlm_client_set_consumer (m_client, topic.c_str(), "") == -1) {
            throw MessageBusException("Failed to set consumer on Malamute connection.");
        }

        m_subscriptions.emplace (topic, messageListener);
        log_trace ("%s - subscribed to topic '%s'", m_clientName.c_str(), topic.c_str());
    }

    void MessageBusMalamute::unsubscribe(const std::string& topic, MessageListener messageListener) {
        auto iterator = m_subscriptions.find (topic);

        if (iterator == m_subscriptions.end ()) {
            throw MessageBusException("Trying to unsubscribe on non-subscribed topic.");
        }

        // Our current Malamute version is too old...
        log_warning ("%s - mlm_client_remove_consumer() not implemented", m_clientName.c_str());

        m_subscriptions.erase (iterator);
        log_trace ("%s - unsubscribed to topic '%s'", m_clientName.c_str(), topic.c_str());
    }

    void MessageBusMalamute::sendRequest(const std::string& requestQueue, const Message& message) {
        std::string to = requestQueue.c_str();
        std::string subject = requestQueue.c_str();

        auto iterator = message.metaData().find(Message::CORRELATION_ID);
        if( iterator == message.metaData().end() || iterator->second == "" ) {
            log_warning("%s - request should have a correlation id", m_clientName.c_str());
        }
        iterator = message.metaData().find(Message::REPLY_TO);
        if( iterator == message.metaData().end() || iterator->second == "" ) {
            log_warning("%s - request should have a reply to field", m_clientName.c_str());
        }
        iterator = message.metaData().find(Message::TO);
        if( iterator == message.metaData().end() || iterator->second == "" ) {
            log_warning("%s - request should have a to field", m_clientName.c_str());
        } else {
            to = iterator->second;
            subject = requestQueue;
        }
        iterator = message.metaData().find(Message::SUBJECT);
        if( iterator != message.metaData().end()) {
            if (iterator->second == "") {
                log_warning("%s - request should have a subject field not empty", m_clientName.c_str());
            } else {
                subject = iterator->second;
            }
        }
        zmsg_t *msg = _toZmsg (message);

        //Todo: Check error code after sendto
        mlm_client_sendto (m_client, to.c_str(), subject.c_str(), nullptr, 200, &msg);
    }

    void MessageBusMalamute::sendRequest(const std::string& requestQueue, const Message& message, MessageListener messageListener) {
        auto iterator = message.metaData().find(Message::REPLY_TO);
        if( iterator == message.metaData().end() || iterator->second == "" ) {
            throw MessageBusException("Request must have a reply to queue.");
        }
        std::string queue(iterator->second);
        receive(queue, messageListener);
        sendRequest(requestQueue, message);
    }

    void MessageBusMalamute::sendReply(const std::string& replyQueue, const Message& message) {
        auto iterator = message.metaData().find(Message::CORRELATION_ID);
        if( iterator == message.metaData().end() || iterator->second == "" ) {
            throw MessageBusException("Reply must have a correlation id.");
        }
        iterator = message.metaData().find(Message::TO);
        if( iterator == message.metaData().end() || iterator->second == "" ) {
            log_warning("%s - request should have a to field", m_clientName.c_str());
        }
        zmsg_t *msg = _toZmsg (message);

        //Todo: Check error code after sendto
        mlm_client_sendto (m_client, iterator->second.c_str(), replyQueue.c_str(), nullptr, 200, &msg);
    }

    void MessageBusMalamute::receive(const std::string& queue, MessageListener messageListener) {
        auto iterator = m_subscriptions.find (queue);
        if (iterator != m_subscriptions.end ()) {
            throw MessageBusException("Already have queue map to listener");
        }
        m_subscriptions.emplace (queue, messageListener);
        log_trace ("%s - receive from queue '%s'", m_clientName.c_str(), queue.c_str());
    }

    Message MessageBusMalamute::request(const std::string& requestQueue, const Message & message, int receiveTimeOut) {
        
        auto iterator = message.metaData().find(Message::CORRELATION_ID);

        if( iterator == message.metaData().end() || iterator->second == "" ) {
            throw MessageBusException("Request must have a correlation id.");
        }
        m_syncUuid = iterator->second;
        iterator = message.metaData().find(Message::TO);
        if( iterator == message.metaData().end() || iterator->second == "" ) {
            throw MessageBusException("Request must have a to field.");
        }

        Message msg(message);
        // Adding metadata timeout.
        msg.metaData().emplace(Message::TIMEOUT, std::to_string(receiveTimeOut));

        std::unique_lock<std::mutex> lock(m_cv_mtx);
        msg.metaData().emplace(Message::REPLY_TO, m_clientName);
        zmsg_t *msgMlm = _toZmsg (msg);

        //Todo: Check error code after sendto
        mlm_client_sendto (m_client, iterator->second.c_str(), requestQueue.c_str(), nullptr, 200, &msgMlm);

        if(m_cv.wait_for(lock, std::chrono::seconds(receiveTimeOut)) == std::cv_status::timeout) {
            throw MessageBusException("Request timed out.");
        }
        else {
            return m_syncResponse;
        }
    }

    void MessageBusMalamute::listener(zsock_t *pipe, void *args) {
        MessageBusMalamute *mbm = reinterpret_cast<MessageBusMalamute *>(args);
        mbm->listenerMainloop(pipe);
    }

    void MessageBusMalamute::listenerMainloop(zsock_t *pipe)
    {
        zpoller_t *poller = zpoller_new (pipe, mlm_client_msgpipe (m_client), nullptr);
        zsock_signal (pipe, 0);
        log_trace ("%s - listener mainloop ready", m_clientName.c_str());

        bool stopping = false;
        while (!stopping) {
            void *which = zpoller_wait (poller, -1);

            if (which == pipe) {
                zmsg_t *message = zmsg_recv (pipe);
                char *actor_command = zmsg_popstr (message);
                zmsg_destroy (&message);

                //  $TERM actor command implementation is required by zactor_t interface
                if (streq (actor_command, "$TERM")) {
                    stopping = true;
                    zstr_free (&actor_command);
                }
                else {
                    log_warning ("%s - received '%s' on pipe, ignored", actor_command ? actor_command : "(null)");
                    zstr_free (&actor_command);
                }
            }
            else if (which == mlm_client_msgpipe (m_client)) {
                zmsg_t *message = mlm_client_recv (m_client);
                if (message == nullptr) {
                    stopping = true;
                }
                else {
                    const char *subject = mlm_client_subject (m_client);
                    const char *from = mlm_client_sender (m_client);
                    const char *command = mlm_client_command (m_client);

                    if (streq (command, "MAILBOX DELIVER")) {
                        listenerHandleMailbox (subject, from, message);
                    } else if (streq (command, "STREAM DELIVER")) {
                        listenerHandleStream (subject, from, message);
                    } else {
                        log_error ("%s - unknown malamute pattern '%s' from '%s' subject '%s'", m_clientName.c_str(), command, from, subject);
                    }
                    zmsg_destroy (&message);
                }
            }
        }

        zpoller_destroy (&poller);

        log_debug ("%s - listener mainloop terminated", m_clientName.c_str());
    }

    void MessageBusMalamute::listenerHandleMailbox (const char *subject, const char *from, zmsg_t *message)
    {
        log_debug ("%s - received mailbox message from '%s' subject '%s'", m_clientName.c_str(), from, subject);

        Message msg = _fromZmsg(message);

        bool syncResponse = false;
        if( m_syncUuid != "" ) {
            auto it = msg.metaData().find(Message::CORRELATION_ID);
            if( it != msg.metaData().end() ) {
                if( m_syncUuid == it->second ) {
                    std::unique_lock<std::mutex> lock(m_cv_mtx);
                    m_syncResponse = msg;
                    m_cv.notify_one();
                    m_syncUuid = "";
                    syncResponse = true;
                }
            }
        }
        if( syncResponse == false ) {
            // FIXME: Workaround for malamute, if the author of the message is not found with the subject,
            // then try with the address of the message which correspond to the name of the topic.

            auto iterator = m_subscriptions.find (subject);
            if (iterator != m_subscriptions.end ()) {
                try {
                    (iterator->second)(msg);
                }
                catch(const std::exception& e) {
                    log_error("Error in listener of queue '%s': '%s'", iterator->first.c_str(), e.what());
                }
                catch(...) {
                    log_error("Error in listener of queue '%s': 'unknown error'", iterator->first.c_str());
                }
            }
            // then try with address of message (= <topic name>)
            else {
                const char *address = mlm_client_address(m_client);
                if (address) {
                    iterator = m_subscriptions.find (address);
                    if (iterator != m_subscriptions.end ()) {
                        //iterator->second(msg);
                        std::thread (iterator->second, msg).detach();
                    }
                    else
                    {
                        log_warning("Message skipped");
                    }
                }
            }
        }
    }

    void MessageBusMalamute::listenerHandleStream (const char *subject, const char *from, zmsg_t *message)
    {
        log_trace ("%s - received stream message from '%s' subject '%s'", m_clientName.c_str(), from, subject);
        Message msg = _fromZmsg(message);

        // FIXME: Workaround for malamute, if the author of the message is not found with the subject,
        // then try with the address of the message which correspond to the name of the topic.

        // Try first with subject
        auto iterator = m_subscriptions.find (subject);
        if (iterator != m_subscriptions.end ()) {
                try {
                    (iterator->second)(msg);
                }
                catch(const std::exception& e) {
                    log_error("Error in listener of topic '%s': '%s'", iterator->first.c_str(), e.what());
                }
                catch(...) {
                    log_error("Error in listener of topic '%s': 'unknown error'", iterator->first.c_str());
                }
        }
        // then try with address of message (= <topic name>)
        else {
            const char *address = mlm_client_address(m_client);
            if (address) {
                iterator = m_subscriptions.find (address);
                if (iterator != m_subscriptions.end ()) {
                    //iterator->second(msg);
                    std::thread (iterator->second, msg).detach();
                }
                else
                {
                    log_warning("Message skipped");
                }
            }
        }
    }

    }
    //  --------------------------------------------------------------------------
    //  Self test of this class

    // If your selftest reads SCMed fixture data, please keep it in
    // src/selftest-ro; if your test creates filesystem objects, please
    // do so under src/selftest-rw.
    // The following pattern is suggested for C selftest code:
    //    char *filename = nullptr;
    //    filename = zsys_sprintf ("%s/%s", SELFTEST_DIR_RO, "mytemplate.file");
    //    assert (filename);
    //    ... use the "filename" for I/O ...
    //    zstr_free (&filename);
    // This way the same "filename" variable can be reused for many subtests.
    #define SELFTEST_DIR_RO "src/selftest-ro"
    #define SELFTEST_DIR_RW "src/selftest-rw"



    void
    fty_common_messagebus_malamute_test (bool verbose)
    {
        printf (" * fty_common_messagebus_malamute_selftest: ");

        //  @end
        printf ("OK\n");
    }
