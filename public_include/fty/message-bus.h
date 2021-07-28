/*  =========================================================================
    message-bus.h - Common message bus wrapper

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

#pragma once
#include "fty/message.h"
#include <functional>
#include <memory>

// =========================================================================================================================================

namespace fty {

/// Common message bus temporary wrapper
class MessageBus
{
public:
    /// Creates message bus
    [[nodiscard]] static Expected<MessageBus> create(const std::string& actorName, const std::string& endpoint = Endpoint) noexcept;

public:
    ~MessageBus();
    MessageBus(const MessageBus&) = delete;
    MessageBus(MessageBus&&) noexcept;

    /// Sends message to the queue and wait to receive response
    /// @param queue the queue to use
    /// @param msg the message to send
    /// @return Response message or error
    [[nodiscard]] Expected<Message> send(const std::string& queue, const Message& msg) noexcept;

    /// Publishes message to a topic
    /// @param queue the queue to use
    /// @param msg the message object to send
    /// @return Success or error
    [[nodiscard]] Expected<void> publish(const std::string& queue, const Message& msg) noexcept;

    /// Sends a reply to a queue
    /// @param queue the queue to use
    /// @param req request message on which you send response
    /// @param answ response message
    /// @return Success or error
    [[nodiscard]] Expected<void> reply(const std::string& queue, const Message& req, const Message& answ) noexcept;

    /// Receives message from queue
    /// @param queue the queue where receive message
    /// @return recieved message or error
    [[nodiscard]] Expected<Message> receive(const std::string& queue) noexcept;

    /// Subscribes to a queue
    /// @example
    ///     bus.subsribe("queue", &MyCls::listener, this);
    /// @param queue the queue to subscribe
    /// @param fnc the member function to subscribe
    /// @param cls class instance
    /// @return Success or error
    template <typename Func, typename Cls>
    [[nodiscard]] Expected<void> subscribe(const std::string& queue, Func&& fnc, Cls* cls) noexcept
    {
        return subscribe(queue, [f = std::move(fnc), c = cls](const messagebus::Message& msg) -> void {
            std::invoke(f, *c, Message(msg));
        });
    }

private:
    static constexpr const char* Endpoint = "ipc://@/malamute";

    MessageBus();
    Expected<void> subscribe(const std::string& queue, std::function<void(const messagebus::Message&)>&& func) noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace fty
