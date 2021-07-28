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

#include "fty/message-bus.h"
#include <fty_common_messagebus_exception.h>
#include <fty_common_messagebus_interface.h>
#include <fty_common_messagebus_message.h>
#include <fty_log.h>
#include <mutex>

namespace fty {

// =========================================================================================================================================

class MessageBus::Impl
{
public:
    Expected<void> init(const std::string& actorName, const std::string& endpoint)
    {
        try {
            std::lock_guard<std::mutex> lock(m_mutex);

            m_bus = std::unique_ptr<messagebus::MessageBus>(messagebus::MlmMessageBus(endpoint, actorName));
            m_bus->connect();
            m_actorName = actorName;

            return {};
        } catch (const std::exception& ex) {
            return unexpected(ex.what());
        } catch (...) {
            return unexpected("Unspecified error");
        }
    }

public:
    std::unique_ptr<messagebus::MessageBus> m_bus;
    std::mutex                              m_mutex;
    std::string                             m_actorName;
};

// =========================================================================================================================================

MessageBus::MessageBus()
    : m_impl(new Impl)
{
}

MessageBus::MessageBus(MessageBus&&) noexcept = default;

MessageBus::~MessageBus()
{
}

Expected<MessageBus> MessageBus::create(const std::string& actorName, const std::string& endpoint) noexcept
{
    MessageBus bus;
    if (auto ret = bus.m_impl->init(actorName, endpoint); !ret) {
        return unexpected(ret.error());
    }
    return Expected<MessageBus>(std::move(bus));
}

Expected<Message> MessageBus::send(const std::string& queue, const Message& msg) noexcept
{
    try {
        std::lock_guard<std::mutex> lock(m_impl->m_mutex);
        if (msg.meta.correlationId.empty()) {
            msg.meta.correlationId = messagebus::generateUuid();
        }
        msg.meta.from = m_impl->m_actorName;

        Message m(m_impl->m_bus->request(queue, msg.toMessageBus(), 10000));
        if (m.meta.status == Message::Status::Error) {
            return unexpected(m.userData[0]);
        }
        return Expected<Message>(m);
    } catch (const messagebus::MessageBusException& ex) {
        return unexpected(ex.what());
    } catch (const std::exception& ex) {
        return unexpected(ex.what());
    } catch (...) {
        return unexpected("Unspecified error");
    }
}

Expected<void> MessageBus::publish(const std::string& queue, const Message& msg) noexcept
{
    try {
        std::lock_guard<std::mutex> lock(m_impl->m_mutex);
        msg.meta.from = m_impl->m_actorName;

        m_impl->m_bus->publish(queue, msg.toMessageBus());

        return {};
    } catch (const messagebus::MessageBusException& ex) {
        return unexpected(ex.what());
    } catch (const std::exception& ex) {
        return unexpected(ex.what());
    } catch (...) {
        return unexpected("Unspecified error");
    }
}

Expected<void> MessageBus::reply(const std::string& queue, const Message& req, const Message& answ) noexcept
{
    try {
        std::lock_guard<std::mutex> lock(m_impl->m_mutex);

        answ.meta.correlationId = req.meta.correlationId;
        answ.meta.to            = req.meta.replyTo;
        answ.meta.from          = m_impl->m_actorName;

        m_impl->m_bus->sendReply(queue, answ.toMessageBus());
        return {};
    } catch (const messagebus::MessageBusException& ex) {
        return unexpected(ex.what());
    } catch (const std::exception& ex) {
        return unexpected(ex.what());
    } catch (...) {
        return unexpected("Unspecified error");
    }
}

Expected<Message> MessageBus::receive(const std::string& queue) noexcept
{
    try {
        std::lock_guard<std::mutex> lock(m_impl->m_mutex);

        Message ret;
        m_impl->m_bus->receive(queue, [&ret](const messagebus::Message& msg) {
            ret = Message(msg);
        });

        return Expected<Message>(ret);
    } catch (const messagebus::MessageBusException& ex) {
        return unexpected(ex.what());
    } catch (const std::exception& ex) {
        return unexpected(ex.what());
    } catch (...) {
        return unexpected("Unspecified error");
    }
}

Expected<void> MessageBus::subscribe(const std::string& queue, std::function<void(const messagebus::Message&)>&& func) noexcept
{
    try {
        std::lock_guard<std::mutex> lock(m_impl->m_mutex);
        m_impl->m_bus->subscribe(queue, func);
        return {};
    } catch (const messagebus::MessageBusException& ex) {
        return unexpected(ex.what());
    } catch (const std::exception& ex) {
        return unexpected(ex.what());
    } catch (...) {
        return unexpected("Unspecified error");
    }
}

} // namespace fty
