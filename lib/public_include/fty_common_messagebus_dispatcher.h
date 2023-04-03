/*  =========================================================================
    fty_common_messagebus_dispatcher - class description

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

#include <functional>
#include <map>

namespace messagebus {

/**
 * \brief Callable dispatcher based on std::map.
 */
template <class KeyType, typename WorkFunctionType, typename MissingFunctionType>
class Dispatcher {
public:
    /// \brief Map of (key -> callable).
    using Map = std::map<KeyType, WorkFunctionType>;

    /**
     * \brief Constructor without default handler.
     * \param map Function map.
     */
    Dispatcher(Map map) : Dispatcher(map, MissingFunctionType()) { }

    /**
     * \brief Constructor with default handler.
     * \param map Function map.
     * \param defaultHandler Default handler callable.
     */
    Dispatcher(Map map, MissingFunctionType defaultHandler) : m_map(map), m_defaultHandler(defaultHandler) { }

    /**
     * \brief Dispatch a callable based on a key.
     * \param key Value to dispatch with.
     * \param args Arguments to pass to the callable.
     * \return Result of callable.
     * \warning Dispatching an unknown key without a default handler will throw an std::bad_function_call.
     */
    template <typename... ArgsType>
    typename WorkFunctionType::result_type operator()(const KeyType& key, ArgsType&&... args) {
        auto it = m_map.find(key);
        if (it != m_map.end()) {
            return it->second(std::forward<ArgsType>(args)...);
        }
        return m_defaultHandler(key, std::forward<ArgsType>(args)...);
    }

private:
    Map m_map;
    MissingFunctionType m_defaultHandler;
};

}
