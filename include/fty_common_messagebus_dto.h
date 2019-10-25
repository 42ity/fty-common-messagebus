/*  =========================================================================
    fty_common_messagebus_dto - class description

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

#ifndef FTY_COMMON_MESSAGEBUS_DTO_H_INCLUDED
#define FTY_COMMON_MESSAGEBUS_DTO_H_INCLUDED

#include <string>
#include <list>

#include "fty_common_messagebus.h"

struct FooBar {
    std::string foo;
    std::string bar;
    FooBar() = default;
    FooBar(const std::string& foo, const std::string& bar) : foo(foo), bar(bar) { }
};

void operator<< (UserData& data, const FooBar& object);
void operator>> (UserData& payload, FooBar& object);

#endif
