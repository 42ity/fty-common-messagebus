/*  =========================================================================
    fty_common_messagebus_unix_socket_client - class description

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

#ifndef FTY_COMMON_MESSAGEBUS_UNIX_SOCKET_CLIENT_H_INCLUDED
#define FTY_COMMON_MESSAGEBUS_UNIX_SOCKET_CLIENT_H_INCLUDED

#include "fty_common_messagebus_classes.h"

#include <string>

#include "fty_common_messagebus_interface.h"
#include "fty_common_messagebus_exception.h"
#include "fty_common_messagebus_message.h"

#include <functional>
#include <map>
#include <mutex>

#include <thread>
#include <condition_variable>

#include <sys/socket.h>

namespace messagebus
{

    class MessageBusUnixSocketClient : public MessageBus
    {
      public:
        /**
         * @brief Construct a new Message Bus Unix Socket object
         * 
         * @param socketPath: Path to the server socket
         * @param maxClient: Maximum number of client for the server. If 0, the MessageBusUnixSocketClient object is consider as a client.
         */
        explicit MessageBusUnixSocketClient(const std::string& socketPath, const std::string& clientName);
        ~MessageBusUnixSocketClient();

        void connect() override;
        
         // Async topic  -> not implemented for socket for now
        void publish(const std::string& topic, const Message& message) override { throw MessageBusException("Function publish not supported by unix socket."); }
        void subscribe(const std::string& topic, MessageListener messageListener) override { throw MessageBusException("Function subscribe not supported by unix socket."); }
        void unsubscribe(const std::string& topic, MessageListener messageListener) override { throw MessageBusException("Function unsubscribe not supported by unix socket."); }

        // Async queue
        void sendRequest(const std::string& requestQueue, const Message& message) override;
        void sendRequest(const std::string& requestQueue, const Message& message, MessageListener messageListener) override { throw MessageBusException("Function sendRequest with MessageListener not supported by unix socket.");}
        void sendReply(const std::string& replyQueue, const Message& message) override;
        void receive(const std::string& queue, MessageListener messageListener) override;

        // Sync queue
        Message request(const std::string& requestQueue, Message message, int receiveTimeOut) override;
        
      private:
        void handleIncomingMessage(const Message & msg);
        void listenerClient();
        void connectToServer();

        void send(const std::string& requestQueue, const Message & message);
        

        std::string   m_socketPath;
        std::string   m_clientName;
        int           m_pipe[2];

        int           m_mainSocket;
        struct sockaddr_un m_addr;

        std::thread m_listenerThread;

        std::map<std::string, MessageListener> m_subscriptions;

        std::condition_variable m_cv;
        std::mutex m_cv_mtx;

        Message m_syncResponse;
        std::string m_syncUuid;
    };
    
} //namespace messagebus

void
fty_common_messagebus_unix_socket_client_test (bool verbose);

#endif
