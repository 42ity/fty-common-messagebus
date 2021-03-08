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

/*
@header
    fty_common_messagebus_unix_socket_client -
@discuss
@end
*/
#include "fty_common_messagebus_classes.h"

#include "fty_common_messagebus_unix_socket_client.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>


int socketConnect (int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len)
{
    return connect(__fd, __addr, __len);
}


namespace messagebus
{
    MessageBusUnixSocketClient::MessageBusUnixSocketClient(const std::string& socketPath, const std::string& clientName)
        :   m_socketPath(socketPath), m_clientName(clientName), m_mainSocket(-1)
    {
        m_syncUuid = "";

        m_pipe[0] = -1;
        m_pipe[1] = -1;

        if (pipe(m_pipe) < 0)
        {
            throw MessageBusException("Impossible to create the pipe: " + std::string(strerror(errno)));
        }

    }

    MessageBusUnixSocketClient::~MessageBusUnixSocketClient()
    {
        //stop the listener
        if(m_listenerThread.joinable())
        {
            write(m_pipe[1], "S", 1);
            m_listenerThread.join();
        }
        
        //close the socket
        if(m_mainSocket != -1)
        {
            close(m_mainSocket);
        }

        // close the pipe
        close(m_pipe[0]);
        close(m_pipe[1]);
        
    }

    void MessageBusUnixSocketClient::connect()
    {
        connectToServer();
        
        //launch the listener
        m_listenerThread = std::thread(&MessageBusUnixSocketClient::listenerClient, this);
    }

    void MessageBusUnixSocketClient::connectToServer()
    {
        //setup the socket
        m_mainSocket = socket(AF_UNIX, SOCK_STREAM, PF_UNSPEC);
        
        if (m_mainSocket == -1)
        {
            throw MessageBusException("Impossible to create unix socket "+m_socketPath+": " + std::string(strerror(errno)));
        }

        //setup the addr
        memset(&m_addr, 0, sizeof(struct sockaddr_un));

        m_addr.sun_family = AF_UNIX;
        strncpy(m_addr.sun_path, m_socketPath.c_str(), sizeof(m_addr.sun_path) - 1);

        int ret = socketConnect(m_mainSocket, (const struct sockaddr *) &m_addr, sizeof(struct sockaddr_un));
        if (ret == -1)
        {
            throw MessageBusException("Impossible to connect to server using the socket "+m_socketPath+": " + std::string(strerror(errno)));
        }
        
        //send the clientName
        sendFrames(m_mainSocket, {m_clientName});
        
        if(recvFrames(m_mainSocket).at(0) != "OK")
        {
            throw MessageBusException("Impossible to connect to server using the socket "+m_socketPath+": Server refuse the connection");
        }

        log_debug("%s - connected through '%s'", m_clientName.c_str(), m_socketPath.c_str());
    }

    void MessageBusUnixSocketClient::listenerClient()
    {
        bool stopRequested = false;
        
        // Clear the reference set of socket
        fd_set socketsSet;
        FD_ZERO(&socketsSet);

        // Add the server socket
        FD_SET(m_mainSocket, &socketsSet);
        int lastSocket = m_mainSocket;
        
        //Add the pipe
        FD_SET(m_pipe[0], &socketsSet);
        if (m_pipe[0] > lastSocket)
        {
            // Keep track of the maximum
            lastSocket = m_pipe[0];
        }

        //infini loop for handling connection
        for (;;)
        {
            fd_set tmpSockets = socketsSet;

            // Detect activity on the sockets
            if (select(lastSocket+1, &tmpSockets, NULL, NULL, NULL) == -1)
            {
              if(stopRequested)
              {
                break;
              }
              else
              {
                //error case
                continue;
              }
            }

            // Run through the existing connections looking for data to be read
            for (int socket = 0; socket <= lastSocket; socket++)
            {
                if (!FD_ISSET(socket, &tmpSockets))
                {
                    // Nothing change on the socket
                    continue;
                }

                if(socket == m_pipe[0]) //request to stop which stop the 
                {
                    char c;
                
                    if (read(m_pipe[0], &c, 1) != 1)
                    {
                        //error
                    }
                    else
                    {
                        if( c == 'S' )
                        {
                            stopRequested = true;
                            break;
                        }
                    }
                }
                else
                {
                    try
                    {
                        // We received request

                        //get credential info
                        struct ucred cred;
                        int lenCredStruct = sizeof(struct ucred);

                        if (getsockopt(socket, SOL_SOCKET, SO_PEERCRED, &cred, (socklen_t*)(&lenCredStruct)) == -1)
                        {
                            throw MessageBusException("Impossible to get sender info: " + std::string(strerror(errno)));                            
                        }

                        struct passwd *pws;
                        pws = getpwuid(cred.uid);

                        std::string sender(pws->pw_name);

                        //log_debug("=== New connection  from %s with PID %i, with UID %i and GID %i\n",pws->pw_name,cred.pid, cred.uid, cred.gid);

                        //Get frames
                        Message msg = recvMessage(socket);  

                        msg.metaData()[Message::REMOTE_USER] = sender;                      

                        //Execute the request
                        handleIncomingMessage(msg);
                    }
                    catch(const std::exception & e)
                    {
                        //show must go one
                        close(m_mainSocket);
                        log_debug("%s - Error: '%s'", m_clientName.c_str(), e.what());
                        connectToServer();
                    }
                    
                }

                //check if we need to leave
                if(stopRequested)
                {
                    break;
                }
            }

            if(stopRequested)
            {
               break;
            }
        }
    }

    void MessageBusUnixSocketClient::sendRequest(const std::string& requestQueue, const Message& message)
    {
        send(requestQueue, message);
    }

    void MessageBusUnixSocketClient::sendReply(const std::string& replyQueue, const Message& message)
    {
        send(replyQueue, message);
    }

    void MessageBusUnixSocketClient::receive(const std::string& queue, MessageListener messageListener)
    {
        auto iterator = m_subscriptions.find (queue);
        if (iterator != m_subscriptions.end ())
        {
            throw MessageBusException("Already have queue map to listener");
        }

        m_subscriptions.emplace (queue, messageListener);

        log_trace ("%s - receive from queue '%s'", m_clientName.c_str(), queue.c_str());
    }

    Message MessageBusUnixSocketClient::request(const std::string& requestQueue, Message message, int receiveTimeOut)
    {
        Message msg(message);

        auto iterator = message.metaData().find(Message::COORELATION_ID);
        if( iterator == message.metaData().end() || iterator->second == "" )
        {
            throw MessageBusException("Request must have a correlation id.");
        }
        m_syncUuid = iterator->second;

        //add the server name for the reply
        msg.metaData().emplace(Message::REPLY_TO, m_clientName);

        
        std::unique_lock<std::mutex> lock(m_cv_mtx);

        send(requestQueue, msg);

        if(m_cv.wait_for(lock, std::chrono::seconds(receiveTimeOut)) == std::cv_status::timeout)
        {
            throw MessageBusException("Request timed out.");
        }

        return m_syncResponse;

    }

    void MessageBusUnixSocketClient::send(const std::string& requestQueue, const Message & message)
    {
        Message msg(message);

        //get the COORELATION_ID
        auto iterator = msg.metaData().find(Message::COORELATION_ID);
        if( iterator == msg.metaData().end() || iterator->second == "" )
        {
            throw MessageBusException("Reply must have a correlation id.");
        }

        //get the FROM
        iterator = msg.metaData().find(Message::FROM);
        if( iterator == msg.metaData().end() || iterator->second == "" )
        {
            throw MessageBusException("Request must have a FROM field.");
        }

        //get the TO
        iterator = msg.metaData().find(Message::TO);
        if( iterator == msg.metaData().end() || iterator->second == "" )
        {
            throw MessageBusException("Request must have a TO field.");
        }

        //add the target queue
        msg.metaData().emplace(Message::QUEUE, requestQueue);

        sendMessage(m_mainSocket, msg);
    }

    void MessageBusUnixSocketClient::handleIncomingMessage(const Message & msg)
    {
        try
        {
            log_debug ("%s - received mailbox message from '%s' queue '%s'", m_clientName.c_str() ,msg.metaData().at(Message::FROM).c_str() , msg.metaData().at(Message::QUEUE).c_str() );

            bool syncResponse = false;
            if( m_syncUuid != "" )
            {
                const std::string correlationId = msg.metaData().at(Message::COORELATION_ID);
                
                if( m_syncUuid == correlationId )
                {
                    std::unique_lock<std::mutex> lock(m_cv_mtx);
                    m_syncResponse = msg;
                    m_syncUuid = "";
                    syncResponse = true;
                }
                
            }

            if( syncResponse == false )
            {
                auto iterator = m_subscriptions.find (msg.metaData().at(Message::QUEUE));
                if (iterator != m_subscriptions.end ())
                {
                    try
                    {
                        (iterator->second)(msg);
                    }
                    catch(const std::exception& e)
                    {
                        log_error("Error in listener of queue '%s': '%s'", iterator->first.c_str(), e.what());
                    }
                    catch(...)
                    {
                        log_error("Error in listener of queue '%s': 'unknown error'", iterator->first.c_str());
                    }
                }
                else
                {
                    log_warning("Message skipped");
                }
            }
            else
            {
                m_cv.notify_one();
            }
        }
        catch(const std::exception& e)
        {
            throw MessageBusException("Impossible to handle message");
        }
        
    }

} //namespace messagebus

//  --------------------------------------------------------------------------
//  Self test of this class

// If your selftest reads SCMed fixture data, please keep it in
// src/selftest-ro; if your test creates filesystem objects, please
// do so under src/selftest-rw.
// The following pattern is suggested for C selftest code:
//    char *filename = NULL;
//    filename = zsys_sprintf ("%s/%s", SELFTEST_DIR_RO, "mytemplate.file");
//    assert (filename);
//    ... use the "filename" for I/O ...
//    zstr_free (&filename);
// This way the same "filename" variable can be reused for many subtests.
#define SELFTEST_DIR_RO "src/selftest-ro"
#define SELFTEST_DIR_RW "src/selftest-rw"

void
fty_common_messagebus_unix_socket_client_test (bool verbose)
{
    printf (" * fty_common_messagebus_unix_socket_client: ");

    //  @selftest
    //  Simple create/destroy test

    //  @end
    printf ("OK\n");
}
