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

/*
@header
    fty_common_messagebus_dto -
@discuss
@end
*/

#include "fty_common_messagebus_classes.h"
#include "fty-common-messagebus-dto.h"

namespace messagebus 
{
    // SrrQueryDto
    void operator<<(messagebus::UserData &data, const SrrQueryDto &object)
    {
        data.push_back(object.action);
        data.push_back(object.data);
    }

    void operator>>(messagebus::UserData &inputData, SrrQueryDto &object)
    {
        auto action = inputData[0];
        auto data = inputData[1];
        object = SrrQueryDto(action, data);
    }

    // SrrFeaturesListDto

    void operator<<(messagebus::UserData &data, const SrrFeaturesListDto &object)
    {
        data = object.featuresList;
    }

    void operator>> (messagebus::UserData &inputData, SrrFeaturesListDto &object) 
    {
        object.featuresList = inputData;
    }

    // SrrResponseDto

    void operator<<(messagebus::UserData &data, const SrrResponseDto &object)
    {
        data.push_back(object.name);
        data.push_back(object.status);
        data.push_back(object.error);
    }

    void operator>>(messagebus::UserData &inputData, SrrResponseDto &object)
    {
        auto name = inputData[0];
        auto status = inputData[1];
        auto error = inputData[2];
        object = SrrResponseDto(name, status, error);
    }

    //void operator<< (srr::UserData &data, SrrResponseDtoList &object) {
    //    data.push_back(object.status);
    //    data.push_back(object.responseList);
    //}
    //void operator>> (srr::UserData &inputData, SrrResponseDtoList &object) {
    //    auto status = inputData[0];
    //    auto responseList = inputData[1];
    //    object = SrrResponseDtoList(status, responseList);
    //}

    // ConfigQueryDto

    void operator<<(messagebus::UserData &data, const ConfigQueryDto &object)
    {
        data.push_back(object.action);
        data.push_back(object.featureName);
    }

    void operator>>(messagebus::UserData &inputData, ConfigQueryDto &object)
    {
        auto action = inputData[0];
        auto featureName = inputData[1];
        object = ConfigQueryDto(action, featureName);
    }

    // ConfigResponseDto

    void operator<<(messagebus::UserData &data, const ConfigResponseDto &object)
    {
        data.push_back(object.status);
        data.push_back(object.data);
    }

    void operator>>(messagebus::UserData &inputData, ConfigResponseDto &object)
    {
        auto status = inputData[0];
        auto data = inputData[1];
        object = ConfigResponseDto(status, data);
    }
}