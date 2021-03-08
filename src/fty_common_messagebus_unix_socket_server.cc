/*  =========================================================================
    fty_common_messagebus_unix_socket_server - class description

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
    fty_common_messagebus_unix_socket_server -
@discuss
@end
*/

#include "fty_common_messagebus_classes.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace messagebus
{
    //local constants
    static const std::string BEGIN_METADATA = "___BEGIN__METADATA___";
    static const std::string END_METADATA   = "___END__METADATA___";

    MessageBusUnixSocketServer::MessageBusUnixSocketServer(const std::string& socketPath, const std::string& serverName, size_t maxClient)
        :   m_socketPath(socketPath), m_serverName(serverName), m_maxClient(maxClient), m_mainSocket(-1)
    {
        m_syncUuid = "";

        m_pipe[0] = -1;
        m_pipe[1] = -1;

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

        if (pipe(m_pipe) < 0)
        {
            throw MessageBusException("Impossible to create the pipe: " + std::string(strerror(errno)));
        }

    }

    MessageBusUnixSocketServer::~MessageBusUnixSocketServer()
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

        // Unlink the socket
        unlink(m_socketPath.c_str());

    }

    void MessageBusUnixSocketServer::connect()
    {
        int ret = 0;
        /*
        * In case the program exited inadvertently on the last run,
        * remove the socket.
        */
        unlink(m_socketPath.c_str());

        //bind
        ret = bind(m_mainSocket, (const struct sockaddr *) &m_addr, sizeof(struct sockaddr_un));
    
        if (ret == -1)
        {
            throw MessageBusException("Impossible to bind the Unix socket "+m_socketPath+": " + std::string(strerror(errno)));
        }
        
        //change the right of the socket
        ret = chmod(m_socketPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO );
        
        if(ret == -1)
        {
            throw MessageBusException("Impossible to change the rights of the Unix socket "+m_socketPath+": " + std::string(strerror(errno)));
        }
        
        //Prepare for accepting connections
        ret = listen(m_mainSocket, m_maxClient);
        if (ret == -1)
        {
            throw MessageBusException("Impossible to listen on the Unix socket "+m_socketPath+": " + std::string(strerror(errno)));
        }

        //launch the listener
        m_listenerThread = std::thread(&MessageBusUnixSocketServer::listenerServer, this);

    }

    void MessageBusUnixSocketServer::listenerServer()
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

                //printf("Activity on file descriptor '%i'\n",socket);


                if (socket == m_mainSocket)
                {
                    //printf("activity on main socket\n");

                    // A client is asking a new connection
                    socklen_t addrlen;
                    struct sockaddr_storage clientaddr;
                    int newSocket;

                    // Handle a new connection
                    addrlen = sizeof(clientaddr);
                    memset(&clientaddr, 0, sizeof(clientaddr));

                    newSocket = accept(m_mainSocket, (struct sockaddr *)&clientaddr, &addrlen);

                    //printf("New socket %i\n", newSocket);

                    if (newSocket != -1)
                    {
                        //Store the name of the client
                        try
                        {
                            std::string clientName = recvFrames(newSocket)[0];
                            
                            if(m_connections.find(clientName) != m_connections.end())
                            {
                                sendFrames(newSocket, {"KO"});
                                throw MessageBusException("already connected");
                            }
                            
                            sendFrames(newSocket, {"OK"});

                            m_connections[clientName] = newSocket;

                            //save the socket
                            FD_SET(newSocket, &socketsSet);

                            if (newSocket > lastSocket)
                            {
                                // Keep track of the maximum
                                lastSocket = newSocket;
                            }

                            log_debug("%s - Connected to client '%s' through socket %i", m_serverName.c_str(), clientName.c_str(), newSocket);
                        }
                        catch(...)
                        {
                            
                            //close the connection in case of error
                            close(newSocket);

                            continue;
                        }

                    }
                    else
                    {
                        //PRINT_DEBUG("Server accept() error");
                    }

            
                    
                }
                else if(socket == m_pipe[0]) //request to stop which stop the 
                {
                    //printf("activity on main the pipe\n");
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

                    //printf("activity socket %i\n", socket);
                    bool socketNeedToBeClose = false;

                    //check if we received data or if the socket is closed by client
                    int numberOfBytes = 0;
                    ioctl(socket, FIONREAD, &numberOfBytes);

                    if(numberOfBytes == 0)
                    {
                        //socket is closed
                        socketNeedToBeClose = true;
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

                            //log_debug("=== New connection on server from %s with PID %i, with UID %i and GID %i\n",pws->pw_name,cred.pid, cred.uid, cred.gid);
                            
                            //Get frames
                            Message msg = recvMessage(socket);  

                            msg.metaData().emplace(Message::REMOTE_USER, sender);                      
                            
                            //Execute the request
                            handleIncomingMessage(msg);
                        }
                        catch(const std::exception & e)
                        {

                            log_debug("%s - Error: '%s'", m_serverName.c_str(), e.what());
                            //close the connection in case of error
                            socketNeedToBeClose = true;
                            
                        } 
                    }

                    if(socketNeedToBeClose)
                    {
                        close(socket);

                        auto it = m_connections.begin();
                        while(it != m_connections.end())
                        {
                            if(it->second == socket)
                            {
                                log_debug ("%s - Connection closed with client '%s'", m_serverName.c_str(), it->first.c_str());

                                m_connections.erase(it);
                                break;
                            }
                            // Go to next entry in map
                            it++;
                        }

                        // Remove from reference set
                        FD_CLR(socket, &socketsSet);

                        if (socket == lastSocket)
                        {
                            lastSocket--;
                        }
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

        //End of the handler.Close the sockets except the server one.
        for (int socket = lastSocket; socket >= 0; socket--)
        {
          m_connections.clear();

          fd_set tmpSockets = socketsSet;

          if (!FD_ISSET(socket, &tmpSockets))
          {
            // This socket is not in the set
            continue;
          }

          //Don't close the server socket or pipe
          if((socket != m_mainSocket) && (socket != m_pipe[0]))
          {
            close(socket);

            // Remove from reference set
            FD_CLR(socket, &socketsSet);

            if (socket == lastSocket)
            {
              lastSocket--;
            }
          }

        }
    }

    void MessageBusUnixSocketServer::sendRequest(const std::string& requestQueue, const Message& message)
    {
        send(requestQueue, message);
    }

    void MessageBusUnixSocketServer::sendReply(const std::string& replyQueue, const Message& message)
    {
        send(replyQueue, message);
    }

    void MessageBusUnixSocketServer::receive(const std::string& queue, MessageListener messageListener)
    {
        auto iterator = m_subscriptions.find (queue);
        if (iterator != m_subscriptions.end ())
        {
            throw MessageBusException("Already have queue map to listener");
        }

        m_subscriptions.emplace (queue, messageListener);

        log_trace ("%s - receive from queue '%s'", m_serverName.c_str(), queue.c_str());
    }

    Message MessageBusUnixSocketServer::request(const std::string& requestQueue, Message message, int receiveTimeOut)
    {
        Message msg(message);

        auto iterator = message.metaData().find(Message::COORELATION_ID);
        if( iterator == message.metaData().end() || iterator->second == "" )
        {
            throw MessageBusException("Request must have a correlation id.");
        }
        m_syncUuid = iterator->second;

        //add the server name for the reply
        msg.metaData().emplace(Message::REPLY_TO, m_serverName);
        
        std::unique_lock<std::mutex> lock(m_cv_mtx);

        send(requestQueue, msg);

        if(m_cv.wait_for(lock, std::chrono::seconds(receiveTimeOut)) == std::cv_status::timeout)
        {
            throw MessageBusException("Request timed out.");
        }

        return m_syncResponse;

    }

    void MessageBusUnixSocketServer::send(const std::string& requestQueue, const Message & message)
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

        auto it = m_connections.find(iterator->second);
        if(it == m_connections.end())
        {
            throw MessageBusException("No client <"+ iterator->second +" connected.");
        }

        sendMessage(it->second, msg);
    }

    void MessageBusUnixSocketServer::handleIncomingMessage(const Message & msg)
    {
        try
        {
            log_debug ("%s - received mailbox message from '%s' queue '%s'", m_serverName.c_str() ,msg.metaData().at(Message::FROM).c_str() , msg.metaData().at(Message::QUEUE).c_str() );

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
//    filename = zsys_sprintf ("%s/%s", SELFTEST_DIR_RO, "mytemplate.file");l
//    assert (filename);
//    ... use the "filename" for I/O ...
//    zstr_free (&filename);
// This way the same "filename" variable can be reused for many subtests.
#define SELFTEST_DIR_RO "src/selftest-ro"
#define SELFTEST_DIR_RW "src/selftest-rw"

#include "fty_common_messagebus_unix_socket_client.h"
#include <memory>

using namespace messagebus;
    
std::unique_ptr<MessageBus> g_server;
std::unique_ptr<MessageBus> g_client;

void server_callback(const Message & request)
{
    Message response;
    response.metaData()[Message::TO] = request.metaData().at(Message::REPLY_TO);
    response.metaData()[Message::COORELATION_ID] = request.metaData().at(Message::COORELATION_ID);
    response.metaData()[Message::FROM] = "my_server";
    
    response.userData().emplace_back("I got your message <"+request.userData().front()+">");
    
    g_server->sendReply("COMMUNICATION_QUEUE", response);

}

void
fty_common_messagebus_unix_socket_server_test (bool verbose)
{
    printf (" * fty_common_messagebus_unix_socket_server: ");

    //  @selftest
    try
    {
        g_server.reset(new MessageBusUnixSocketServer("./test.socket", "my_server"));
        g_client.reset(new MessageBusUnixSocketClient("./test.socket", "my_client"));

        g_server->connect();
        g_client->connect();

        g_server->receive("COMMUNICATION_QUEUE", server_callback);
                
        Message myRequest;
        myRequest.metaData()[Message::TO] = "my_server";
        myRequest.metaData()[Message::FROM] = "my_client";
        myRequest.metaData()[Message::COORELATION_ID] = "1111011";
        
        myRequest.userData().emplace_back("test");
        
        Message myResponse = g_client->request("COMMUNICATION_QUEUE", myRequest , 1);

        if(myResponse.userData().front() != "I got your message <test>")
        {
            throw std::runtime_error("Test failed!");
        }
    }
    catch(const std::exception& e)
    {
        printf("Error %s\n", e.what());
        assert(false);
    }

    //  @end
    printf ("OK\n");
}
