/*  =========================================================================
    fty_common_messagebus_mqtt - class description

    Copyright (C) 2014 - 2021 Eaton

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

#ifndef FTY_COMMON_MESSAGEBUS_MQTT_CALL_BACK
#define FTY_COMMON_MESSAGEBUS_MQTT_CALL_BACK

#include "fty_common_messagebus_interface.h"

#include <mqtt/async_client.h>
#include <mqtt/message.h>

#include <string>
#include <thread>

namespace messagebus
{

  using subScriptionListener = std::map<std::string, MessageListener>;

  class callback : public virtual mqtt::callback
  {
  public:
    //callback() = default;
    callback();
    ~callback();
    void connection_lost(const std::string& cause) override;
    void onConnected(const std::string& cause);
    bool onConnectionUpdated(const mqtt::connect_data& connData);

    void onRequestArrived(mqtt::const_message_ptr msg, MessageListener messageListener);
    void onReqRepMsgArrived(mqtt::const_message_ptr msg);

    auto getSubscriptions() -> messagebus::subScriptionListener;
    void setSubscriptions(const std::string& queue, MessageListener messageListener);

  private:
    messagebus::subScriptionListener m_subscriptions;

    std::condition_variable_any m_cv;
    // TODO replace by a real thread pool
    std::vector<std::thread> m_threadPool;
  };
} // namespace messagebus

#endif // ifndef FTY_COMMON_MESSAGEBUS_MQTT_CALL_BACK
