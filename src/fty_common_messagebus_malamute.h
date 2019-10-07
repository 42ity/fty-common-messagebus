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

#ifndef FTY_COMMON_MESSAGEBUS_MALAMUTE_H_INCLUDED
#define FTY_COMMON_MESSAGEBUS_MALAMUTE_H_INCLUDED

#include <string>

#include "fty_common_messagebus_interface.h"
#include "fty_common_messagebus_exception.h"
#include "fty_common_messagebus_message.h"

#include <functional>
#include <map>
#include <mutex>
#include <condition_variable>

namespace messagebus {

typedef void(MalamuteMessageListenerFn)(const char *, const char *, zmsg_t **);
using MalamuteMessageListener = std::function<MalamuteMessageListenerFn>;

class MessageBusMalamute : public MessageBus {
  public:
    MessageBusMalamute(const std::string& endpoint, const std::string& clientName);
    ~MessageBusMalamute();

     // Async topic
    void publish(const std::string& topic, const Message& message) override;
    void subscribe(const std::string& topic, MessageListener messageListener) override;
    void unsubscribe(const std::string& topic, MessageListener messageListener) override;

    // Async queue
    void sendRequest(const std::string& requestQueue, const Message& message) override;
    void sendRequest(const std::string& requestQueue, const Message& message, MessageListener messageListener) override;
    void sendReply(const std::string& replyQueue, const Message& message) override;
    void receive(const std::string& queue, MessageListener messageListener) override;

    // Sync queue
    Message request(const std::string& requestQueue, Message message, int receiveTimeOut) override;

  private:
    static void listener(zsock_t *pipe, void* ptr);
    void listenerMainloop(zsock_t *pipe);
    void listenerHandleMailbox (const char *, const char *, zmsg_t *);
    void listenerHandleStream (const char *, const char *, zmsg_t *);

    mlm_client_t *m_client;
    std::string   m_clientName;
    std::string   m_publishTopic;

    zactor_t     *m_actor;
    std::map<std::string, MessageListener> m_subscriptions;

    std::condition_variable m_cv;
    std::mutex m_cv_mtx;
    Message m_syncResponse;
    std::string m_syncUuid;
};

}

void fty_common_messagebus_malamute_test (bool verbose);

#endif
