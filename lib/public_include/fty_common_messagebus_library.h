/*  =========================================================================
    fty-common-messagebus - generated layer of public API

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

//  Set up environment for the application

//  External dependencies
#include <czmq.h>
#include <malamute.h>
#include <fty_log.h>

//  Public classes, each with its own header file
#include "fty_common_messagebus_exception.h"
#include "fty_common_messagebus_message.h"
#include "fty_common_messagebus_dto.h"
#include "fty_common_messagebus_interface.h"
#include "fty_common_messagebus_dispatcher.h"
#include "fty_common_messagebus_pool_worker.h"

