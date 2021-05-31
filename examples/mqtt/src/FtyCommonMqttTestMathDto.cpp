/*  =========================================================================
    FtyCommonMqttTestMathDto - class description

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

/*
@header
    FtyCommonMqttTestMathDto -
@discuss
@end
*/

#include "FtyCommonMqttTestMathDto.h"
#include "fty_common_messagebus_message.h"

#include <ostream>

void operator<<(messagebus::UserData& data, const MathOperation& object)
{
  data.push_back(object.operation);
  data.push_back(object.param_1);
  data.push_back(object.param_2);
}

void operator>>(messagebus::UserData& data, MathOperation& object)
{
  auto operation = data.front();
  data.pop_front();
  auto param_1 = data.front();
  data.pop_front();
  auto param_2 = data.front();
  data.pop_front();
  object = MathOperation(operation, param_1, param_2);
}

std::ostream& operator<<(std::ostream& os, const MathOperation& mathOperation)
{
  os << "MathOperation(";
  os << "operation=" << mathOperation.operation;
  os << ", param_1=" << mathOperation.param_1;
  os << ", param_2=" << mathOperation.param_2;
  os << ")";
  return os;
}

void operator<<(messagebus::UserData& data, const MathResult& object)
{
  data.push_back(object.status);
  data.push_back(object.result);
}

void operator>>(messagebus::UserData& data, MathResult& object)
{
  auto status = data.front();
  data.pop_front();
  auto result = data.front();
  data.pop_front();
  object = MathResult(status, result);
}

std::ostream& operator<<(std::ostream& os, const MathResult& mathResult)
{
  os << "MathResult(";
  os << "status=" << mathResult.status;
  os << ", result=" << mathResult.result;
  os << ")";
  return os;
}
