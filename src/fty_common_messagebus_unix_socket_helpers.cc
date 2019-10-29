/*  =========================================================================
    fty_common_messagebus_unix_socket_helpers - class description

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
    fty_common_messagebus_unix_socket_helpers -
@discuss
@end
*/

#include "fty_common_messagebus_classes.h"

namespace messagebus
{
    //local constants
    static const std::string BEGIN_METADATA = "___BEGIN__METADATA___";
    static const std::string END_METADATA   = "___END__METADATA___";


    Message recvMessage(int socket)
    {
        std::vector<std::string> payload = recvFrames(socket);

        Message message; 

        if( !payload.empty() )
        {
            std::vector<std::string>::const_iterator frameIterator = payload.begin();

            std::string key = *frameIterator;

            //get metadata
            if( key == BEGIN_METADATA )
            {
                while (++frameIterator != payload.end())
                {
                    key = *frameIterator;

                    if ( key == END_METADATA )
                    {
                        break;
                    }

                    ++frameIterator;
                    const std::string & value = *frameIterator;

                    //add to metaData
                    message.metaData().emplace(key, value);
                    printf("Metadata[%s]=%s\n",key.c_str(),value.c_str());
                }
            }
            else
            {
                message.userData().emplace_back(key);
                printf("Userdata <%s>\n",key.c_str());
            }

            while (++frameIterator != payload.end())
            {
                message.userData().emplace_back(*frameIterator);
                printf("Userdata <%s>\n",(*frameIterator).c_str());
            }
        }

        return message;
    }
    
    void sendMessage(int socket, const Message & msg)
    {
        std::vector<std::string> payload;

        //add meta data
        payload.push_back(BEGIN_METADATA);
        for (const auto& pair : msg.metaData())
        {
            payload.push_back(pair.first);
            payload.push_back(pair.second);
        }
        payload.push_back(END_METADATA);

        //add user data
        for(const auto& item : msg.userData())
        {
            payload.push_back(item);
        }

        sendFrames(socket, payload);
    }

    std::vector<std::string> recvFrames(int socket)
    {
        //format => [ Number of frames ], [ <size of frame 1> <data> ], ... [ <size of frame N> <data> ]

        //get the number of frames
        uint32_t numberOfFrame = 0;

        
        if(read(socket, &numberOfFrame, sizeof(uint32_t)) != sizeof(uint32_t))
        {
            throw MessageBusException("Error while reading number of frame");
        }
        

        //Get frames
        std::vector<std::string> frames;

        for( uint32_t index = 0; index < numberOfFrame; index++)
        {
            //get the size of the frame
            uint32_t frameSize = 0;
            
            
            if(read(socket, &frameSize, sizeof(uint32_t)) != sizeof(uint32_t))
            {
                throw MessageBusException("Error while reading size of frame");
            }
            

            if(frameSize == 0)
            {
                throw MessageBusException("Read error: Empty frame");
            }

            //get the payload of the frame
            std::vector<char> buffer;
            buffer.reserve(frameSize+1);
            buffer[frameSize] = 0;

            if(read(socket, &buffer[0], frameSize) != frameSize)
            {
                throw MessageBusException("Read error while getting payload of frame");
            }
            
            std::string str(&buffer[0]);
            
            frames.push_back(str);
        }
        
        return frames;
    }
    
    void sendFrames(int socket, const std::vector<std::string> & payload)
    {
        //Send number of frame
        uint32_t numberOfFrame = payload.size();
        
        if ( write(socket, &numberOfFrame, sizeof(uint32_t)) != sizeof(uint32_t) )
        {
            throw MessageBusException("Error while writing number of frame on socket "+std::to_string(socket));
        }
        
        for(const std::string & frame : payload)
        {
            uint32_t frameSize = frame.length() + 1;
            
            if ( write(socket, &frameSize, sizeof(uint32_t)) != sizeof(uint32_t) )
            {
                throw MessageBusException("Error while writing size of frame on socket "+std::to_string(socket));
            }
            
            if ( write(socket, frame.data(), frameSize) != frameSize )
            {
                throw MessageBusException("Error while writing payload on socket "+std::to_string(socket));
            }    
        }
    }
}