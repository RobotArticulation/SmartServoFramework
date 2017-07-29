/*!
 * This file is part of SmartServoFramework.
 * Copyright (c) 2014, INRIA, All rights reserved.
 *
 * SmartServoFramework is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this software. If not, see <http://www.gnu.org/licenses/lgpl-3.0.txt>.
 *
 * \file DynamixelTools.cpp
 * \date 11/03/2014
 * \author Emeric Grange <emeric.grange@gmail.com>
 */

#include "dynamixel/DynamixelTools.h"
#include "minitraces.h"

std::string mcy_get_model_name(const int model_number)
{
    std::string name;

    switch (model_number & 0x0000FFFF)
    {
    case 0x000C:
        name = "AX-12A";
        break;
    case 0x012C:
        name = "AX-12W";
        break;
    case 0x0012:
        name = "AX-18A";
        break;

    case 0x0071:
        name = "DX-113";
        break;
    case 0x0074:
        name = "DX-116";
        break;
    case 0x0075:
        name = "DX-117";
        break;

    case 0x000A:
        name = "RX-10";
        break;
    case 0x0018:
        name = "RX-24F";
        break;
    case 0x001C:
        name = "RX-28";
        break;
    case 0x0040:
        name = "RX-64";
        break;

    case 0x006A:
        name = "EX-106";
        break;
    case 0x006B:
        name = "EX-106+";
        break;

    case 0x0168:
        name = "MX-12W";
        break;
    case 0x001D:
        name = "MX-28";
        break;
    case 0x0136:
        name = "MX-64";
        break;
    case 0x0140:
        name = "MX-106";
        break;

    case 0x015E:
        name = "XL-320";
        break;

    case 0x01020:
        name = "XM430-W350";
        break;
    case 0x01030:
        name = "XM430-W210";
        break;
    case 0x01040:
        name = "XH430-V350";
        break;
    case 0x01050:
        name = "XH430-V210";
        break;
    case 0x01000:
        name = "XH430-W350";
        break;
    case 0x01010:
        name = "XH430-W210";
        break;

    case 0x0013:
        name = "AX-S1";
        break;
    case 0x014A:
        name = "IR Sensor Array";
        break;

    default:
        name = "Unknown";
        break;
    }

    return name;
}

void mcy_get_model_infos(const int model_number, int &servo_serie, int &servo_model)
{
    switch (model_number & 0x0000FFFF)
    {
    case 0x017:
        servo_serie = SERVO_ARCADIA;
        servo_model = SERVO_ARCADIA_01;
        break;
    default:
        servo_serie = SERVO_UNKNOWN;
        servo_model = SERVO_UNKNOWN;
        break;
    }
}

int mcy_get_servo_model(const int model_number)
{
    int servo_serie, servo_model;
    mcy_get_model_infos(model_number, servo_serie, servo_model);

    return servo_model;
}

int mcy_get_baudrate(const int baudnum, const int servo_serie)
{
    int baudRate = 1000000;

    if (servo_serie == 0)
    {
        TRACE_ERROR(TOOLS, "Unknown servo serie, using default baudrate of: '%i' bps", baudRate);
    }
    else if (servo_serie >= SERVO_ARCADIA)
    {
        if (baudnum > 0 && baudnum < 255)
        {
            baudRate = static_cast<int>(2000000.0 / static_cast<double>(baudnum + 1));
        }
        else
        {
            TRACE_ERROR(TOOLS, "Invalid baudnum '%i' for AX serie, using default baudrate of: '%i' bps", baudnum, baudRate);
        }
    }
    else
    {
        TRACE_ERROR(TOOLS, "Unsupported Dynamixel servo serie, using default baudrate of: '%i' bps", baudRate);
    }

    // Force minimum baudrate to 2400 bps if needed
    if (baudRate < 2400)
    {
        baudRate = 2400;
        TRACE_ERROR(TOOLS, "Baudrate value '%i' is too low for Dynamixel devices, using minimum baudrate of: '2400' bps", baudRate);
    }

    return baudRate;
}
