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
 * \file HerkuleXController.cpp
 * \date 25/08/2014
 * \author Emeric Grange <emeric.grange@inria.fr>
 */

#include "HerkuleXController.h"

// C++ standard libraries
#include <iostream>
#include <chrono>
#include <thread>

// Enable latency timer
//#define LATENCY_TIMER

HerkuleXController::HerkuleXController(int freq, int servoSerie):
    ControllerAPI(freq)
{
    if (servoSerie != SERVO_UNKNOWN)
    {
        std::cout << std::endl;

        if (servoSerie >= SERVO_HERKULEX)
        {
            ackPolicy = 1;
            maxId = 253;

            protocolVersion = 1;
            std::cout << "- Using HerkuleX communication protocol" << std::endl;
        }
        else if (servoSerie >= SERVO_DYNAMIXEL)
        {
            ackPolicy = 2;
            maxId = 252;

            if (servoSerie >= SERVO_XL)
            {
                protocolVersion = 2;
            }
            else // SERVO AX to MX
            {
                protocolVersion = 1;

                if (serialDevice == SERIAL_USB2AX)
                {
                    // The USB2AX device uses the ID 253 for itself
                    maxId = 252;
                }
                else
                {
                    maxId = 253;
                }
            }

            if (protocolVersion == 2)
            {
                std::cout << "- Using Dynamixel communication protocol version 2" << std::endl;
            }
            else
            {
                std::cout << "- Using Dynamixel communication protocol version 1" << std::endl;
            }
        }
    }
    else
    {
        std::cerr << "Warning: Unknown servo serie!" << std::endl;
    }

    startThread();
}

HerkuleXController::~HerkuleXController()
{
    stopThread();
    serialTerminate();
}

int HerkuleXController::serialInitialize_wrapper(std::string &deviceName, const int baud, const int serialDevice)
{
    return serialInitialize(deviceName, baud, serialDevice);
}

void HerkuleXController::serialTerminate_wrapper()
{
    serialTerminate();
}

std::string HerkuleXController::serialGetCurrentDevice_wrapper()
{
    return serialGetCurrentDevice();
}

std::vector <std::string> HerkuleXController::serialGetAvailableDevices_wrapper()
{
    return serialGetAvailableDevices();
}

void HerkuleXController::serialSetLatency_wrapper(int latency)
{
    setLatency(latency);
}

void HerkuleXController::autodetect_internal(int start, int stop)
{
    controllerState = state_scanning;

    // Prepare to scan
    unregisterServos_internal();

    // Check start/stop boundaries
    if (start < 0 || start > (maxId - 1))
        start = 0;

    if (stop < 1 || stop > maxId || stop < start)
        stop = maxId;

    // Bring RX packet timeout down to scan way faster
    setLatency(8);

    std::cout << "HKX ctrl_device_autodetect(port: '" << serialGetCurrentDevice() << "' | tid: '" << std::this_thread::get_id() << "')" << std::endl;

    std::cout << "> THREADED Scanning for HKX devices on '" << serialGetCurrentDevice() << "', Range is [" << start << "," << stop << "[" << std::endl;

    for (int id = start; id <= stop; id++)
    {
        PingResponse pingstats;

        // If the ping gets a response, then we have found a servo
        if (hkx_ping(id, &pingstats) == true)
        {
            //setLed(id, 1, LED_GREEN);

            int serie, model;
            hkx_get_model_infos(pingstats.model_number, serie, model);
            ServoHerkuleX *servo = NULL;

            std::cout << std::endl << "[#" << id << "] " << hkx_get_model_name(pingstats.model_number) << " servo found! ";

            // Instanciate the device found
            switch (serie)
            {
            case SERVO_DRS:
                servo = new ServoDRS(id, pingstats.model_number);
                break;

            default:
                break;
            }

            if (servo != NULL)
            {
                servoListLock.lock();

                // Add the servo to the controller
                servoList.push_back(servo);

                // Mark it for an "initial read" and synchronization
                updateList.push_back(servo->getId());
                syncList.push_back(servo->getId());

                servoListLock.unlock();
            }

            //setLed(id, 0);
        }
        else
        {
            std::cout << ".";
        }
    }

    std::cout << std::endl;

    // Restore RX packet timeout
    setLatency(LATENCY_TIME_DEFAULT);

    controllerState = state_scanned;
}

void HerkuleXController::run()
{
    std::cout << "HerkuleXController::run(port: '" << serialGetCurrentDevice() << "' | tid: '" << std::this_thread::get_id() << "')" << std::endl;

    std::chrono::time_point<std::chrono::system_clock> start, end;

    while (syncloopRunning)
    {
        // Loop timer
        start = std::chrono::system_clock::now();

        // MESSAGE PARSING
        ////////////////////////////////////////////////////////////////////////

        unsigned ignoreMsg = 0;

        m_mutex.lock();
        while (m_queue.empty() == false && m_queue.size() != ignoreMsg)
        {
            miniMessages m = m_queue.front();
            m_mutex.unlock();

            switch (m.msg)
            {
            case ctrl_device_autodetect:
                autodetect_internal(m.p1, m.p2);
                break;

            case ctrl_device_register:
                // handle registering servo by "id"?
                registerServo_internal((Servo *)(m.p));
                break;
            case ctrl_device_unregister:
                unregisterServo_internal((Servo *)(m.p));
                break;
            case ctrl_device_unregister_all:
                unregisterServos_internal();
                break;

            case ctrl_device_delayed_add:
                if (delayedAddServos_internal(m.delay, m.p1, m.p2) == 1)
                {
                    ignoreMsg++;
                    sendMessage(&m);
                }
                break;
/*
            case ctrl_pause:
            case ctrl_resume:
                pauseThread_internal();
                break;
            case ctrl_stop:
                stopThread_internal();
                break;
*/
            default:
                std::cerr << "[HKX] Unknown message type: '" << m.msg << "'" << std::endl;
                break;
            }

            m_mutex.lock();
            m_queue.pop_front();
        }
        m_mutex.unlock();

        // ACTION LOOP
        ////////////////////////////////////////////////////////////////////////

        servoListLock.lock();
        for (auto s: servoList)
        {
            int actionProgrammed, rebootProgrammed, refreshProgrammed, resetProgrammed;
            s->getActions(actionProgrammed, rebootProgrammed, refreshProgrammed, resetProgrammed);

            if (refreshProgrammed == 1)
            {
                // Every servo register value will be updated
                updateList.push_back(s->getId());
                std::cout << "Refresh servo #" << s->getId() << " registers"<< std::endl;
            }

            if (rebootProgrammed == 1)
            {
                // Remove servo from sync/update lists; Need to be added again after reboot!
                for (std::vector <int>::iterator it = updateList.begin(); it != updateList.end();)
                {
                    if (*it == s->getId())
                    { updateList.erase(it); }
                    else
                    { it++; }
                }
                for (std::vector <int>::iterator it = syncList.begin(); it != syncList.end();)
                {
                    if (*it == s->getId())
                    { syncList.erase(it); }
                    else
                    { it++; }
                }

                // Reboot
                hkx_reboot(s->getId());
                std::cout << "Rebooting servo #" << s->getId() << "..." << std::endl;

                miniMessages m {ctrl_device_delayed_add, std::chrono::system_clock::now() + std::chrono::seconds(2), NULL, s->getId(), 1};
                sendMessage(&m);
            }

            if (resetProgrammed > 0)
            {
                // Remove servo from sync/update lists; Need to be added again after reset!
                for (std::vector <int>::iterator it = updateList.begin(); it != updateList.end();)
                {
                    if (*it == s->getId())
                    { updateList.erase(it); }
                    else
                    { it++; }
                }
                for (std::vector <int>::iterator it = syncList.begin(); it != syncList.end();)
                {
                    if (*it == s->getId())
                    { syncList.erase(it); }
                    else
                    { it++; }
                }

                // Reset
                hkx_reset(s->getId(), resetProgrammed);
                std::cout << "Resetting servo #" << s->getId() << " (setting: " << resetProgrammed << ")..." << std::endl;

                miniMessages m {ctrl_device_delayed_add, std::chrono::system_clock::now() + std::chrono::seconds(2), NULL, s->getId(), 1};
                sendMessage(&m);
            }
        }
        servoListLock.unlock();

        // INITIAL READ LOOP
        ////////////////////////////////////////////////////////////////////////

        servoListLock.lock();
        if (updateList.empty() == false)
        {
            std::vector <int>::iterator itr;
            for (itr = updateList.begin(); itr != updateList.end();)
            {
                controllerState = state_reading;
                int id = (*itr);

                for (auto s: servoList)
                {
                    if (s->getId() == id)
                    {
                        for (int ctid = 1; ctid < s->getRegisterCount(); ctid++)
                        {
                            struct RegisterInfos reg;
                            int reg_name = getRegisterName(s->getControlTable(), ctid);
                            getRegisterInfos(s->getControlTable(), reg_name, reg);

                            //std::cout << "Reading value for reg [" << ctid << "] name: '" << regname << "' addr: '" << regaddr << "' size: '" << regsize << "'" << std::endl;

                            int reg_type = REGISTER_AUTO;
                            if (reg.reg_addr_rom >= 0 && reg.reg_addr_ram >= 0)
                                reg_type = REGISTER_BOTH;
                            else if (reg.reg_addr_rom >= 0)
                                reg_type = REGISTER_ROM;
                            else if (reg.reg_addr_ram >= 0)
                                reg_type = REGISTER_RAM;

                            if (reg.reg_size == 1)
                            {
                                if (reg_type == REGISTER_BOTH)
                                {
                                    s->updateValue(reg_name, hkx_read_byte(id, reg.reg_addr_rom, REGISTER_ROM), REGISTER_ROM);
                                    s->updateValue(reg_name, hkx_read_byte(id, reg.reg_addr_ram, REGISTER_RAM), REGISTER_RAM);
                                }
                                else if (reg_type == REGISTER_ROM)
                                {
                                    s->updateValue(reg_name, hkx_read_byte(id, reg.reg_addr_rom, REGISTER_ROM), REGISTER_ROM);
                                }
                                else if (reg_type == REGISTER_RAM)
                                {
                                    s->updateValue(reg_name, hkx_read_byte(id, reg.reg_addr_ram, REGISTER_RAM), REGISTER_RAM);
                                }
                            }
                            else //if (reg.reg_size == 2)
                            {
                                if (reg_type == REGISTER_BOTH)
                                {
                                    s->updateValue(reg_name, hkx_read_word(id, reg.reg_addr_rom, REGISTER_ROM), REGISTER_ROM);
                                    s->updateValue(reg_name, hkx_read_word(id, reg.reg_addr_ram, REGISTER_RAM), REGISTER_RAM);
                                }
                                else if (reg_type == REGISTER_ROM)
                                {
                                    s->updateValue(reg_name, hkx_read_word(id, reg.reg_addr_rom, REGISTER_ROM), REGISTER_ROM);
                                }
                                else if (reg_type == REGISTER_RAM)
                                {
                                    s->updateValue(reg_name, hkx_read_word(id, reg.reg_addr_ram, REGISTER_RAM), REGISTER_RAM);
                                }
                            }

                            s->setError(hkx_get_rxpacket_error());
                            s->setStatus(hkx_get_rxpacket_status_detail());
                            errors += hkx_get_com_error();
                            hkx_print_error();
                        }

                        // Once all registers are read, remove the servo from the "updateList"
                        itr = updateList.erase(itr);
                    }
                }
            }
            controllerState = state_ready;
        }
        servoListLock.unlock();

        // SYNCHRONIZATION LOOP
        ////////////////////////////////////////////////////////////////////////

        int cumulid = 0;

        servoListLock.lock();
        for (auto id: syncList)
        {
            cumulid++;
            cumulid %= syncloopFrequency;

            for (auto s: servoList)
            {
                servoListLock.unlock();

                if (s->getId() == id)
                {
                    // Write
                    for (int ctid = 0; ctid < s->getRegisterCount(); ctid++)
                    {
                        int regname = getRegisterName(s->getControlTable(), ctid);
                        int regsize = getRegisterSize(s->getControlTable(), regname);

                        {
                            if (s->getValueCommit(regname, REGISTER_ROM) == 1)
                            {
                                int regaddr = getRegisterAddr(s->getControlTable(), regname, REGISTER_ROM);

                                //std::cout << "Writing ROM value '" << s->getValue(regname) << "' for reg [" << ctid << "] name: '" << getRegisterNameTxt(regname)
                                //          << "' addr: '" << regaddr << "' size: '" << regsize << "'" << std::endl;

                                if (regsize == 1)
                                {
                                    hkx_write_byte(id, regaddr, s->getValue(regname, REGISTER_ROM), REGISTER_ROM);
                                }
                                else //if (regsize == 2)
                                {
                                    hkx_write_word(id, regaddr, s->getValue(regname, REGISTER_ROM), REGISTER_ROM);
                                }

                                s->setError(hkx_get_rxpacket_error());
                                s->setStatus(hkx_get_rxpacket_status_detail());
                                s->commitValue(regname, 0, REGISTER_ROM);
                                errors += hkx_get_com_error();
                                hkx_print_error();
                            }

                            if (s->getValueCommit(regname, REGISTER_RAM) == 1)
                            {
                                int regaddr = getRegisterAddr(s->getControlTable(), regname, REGISTER_RAM);

                                //std::cout << "Writing RAM value '" << s->getValue(regname) << "' for reg [" << ctid << "] name: '" << getRegisterNameTxt(regname)
                                //          << "' addr: '" << regaddr << "' size: '" << regsize << "'" << std::endl;

                                if (regsize == 1)
                                {
                                    hkx_write_byte(id, regaddr, s->getValue(regname, REGISTER_RAM), REGISTER_RAM);
                                }
                                else //if (regsize == 2)
                                {
                                    hkx_write_word(id, regaddr, s->getValue(regname, REGISTER_RAM), REGISTER_RAM);
                                }

                                s->setError(hkx_get_rxpacket_error());
                                s->setStatus(hkx_get_rxpacket_status_detail());
                                s->commitValue(regname, 0, REGISTER_RAM);
                                errors += hkx_get_com_error();
                                hkx_print_error();
                            }
                        }
                    }

                    // 1Hz "low priority" loop
                    if ((syncloopCounter - cumulid) == 0)
                    {
                        // Read voltage
                        s->updateValue(REG_CURRENT_VOLTAGE, hkx_read_byte(id, s->gaddr(REG_CURRENT_VOLTAGE)));
                        s->setError(hkx_get_rxpacket_error());
                        s->setStatus(hkx_get_rxpacket_status_detail());
                        errors += hkx_get_com_error();
                        hkx_print_error();

                        // Read temp
                        s->updateValue(REG_CURRENT_TEMPERATURE, hkx_read_byte(id, s->gaddr(REG_CURRENT_TEMPERATURE)));
                        s->setError(hkx_get_rxpacket_error());
                        s->setStatus(hkx_get_rxpacket_status_detail());
                        errors += hkx_get_com_error();
                        hkx_print_error();
                    }

                    // x/4 Hz "feedback" loop
                    if ((syncloopCounter - cumulid) % 4 == 0)
                    {
                        s->updateValue(REG_STATUS_ERROR, hkx_read_byte(id, s->gaddr(REG_STATUS_ERROR)));
                        s->setError(hkx_get_rxpacket_error());
                        s->setStatus(hkx_get_rxpacket_status_detail());
                        errors += hkx_get_com_error();
                        hkx_print_error();

                        s->updateValue(REG_STATUS_DETAIL, hkx_read_byte(id, s->gaddr(REG_STATUS_DETAIL)));
                        s->setError(hkx_get_rxpacket_error());
                        s->setStatus(hkx_get_rxpacket_status_detail());
                        errors += hkx_get_com_error();
                        hkx_print_error();
/*
                        s->updateCurrentSpeed(hkx_read_word(id, s->gaddr(SERVO_CURRENT_SPEED)));
                        s->setError(hkx_get_rxpacket_error());
                        s->setStatus(hkx_get_rxpacket_status_detail());
                        errors += hkx_get_com_error();
                        hkx_print_error();

                        s->updateCurrentLoad(hkx_read_word(id, s->gaddr(SERVO_CURRENT_LOAD)));
                        s->setError(hkx_get_rxpacket_error());
                        s->setStatus(hkx_get_rxpacket_status_detail());
                        errors += hkx_get_com_error();
                        hkx_print_error();
*/
                    }

                    // x Hz "full speed" loop
                    {
                        // Get "current" values from servo, and write them into obj
                        int cpos = hkx_read_word(id, s->gaddr(REG_ABSOLUTE_POSITION));
                        s->updateValue(REG_ABSOLUTE_POSITION, cpos);
                        s->setError(hkx_get_rxpacket_error());
                        s->setStatus(hkx_get_rxpacket_status_detail());
                        errors += hkx_get_com_error();
                        hkx_print_error();

                        if (((ServoDRS*)s)->getGoalPositionCommited() == 1)
                        {
                            int gpos = s->getGoalPosition();

                            hkx_i_jog(id, 0, gpos);
                            if (hkx_print_error() == 0)
                            {
                                ((ServoDRS*)s)->commitGoalPosition();
                            }
                        }

                        s->updateValue(REG_ABSOLUTE_GOAL_POSITION, hkx_read_word(id, s->gaddr(REG_ABSOLUTE_GOAL_POSITION)));
                        s->setError(hkx_get_rxpacket_error());
                        s->setStatus(hkx_get_rxpacket_status_detail());
                        errors += hkx_get_com_error();
                        hkx_print_error();
                    }
                }

                servoListLock.lock();
            }
        }

        // Make sure we unlock servoList
        servoListLock.unlock();

        // Loop control
        syncloopCounter++;
        syncloopCounter %= syncloopFrequency;

        // Loop timer
        end = std::chrono::system_clock::now();
        double loopd = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
        double waitd = (syncloopDuration * 1000.0) - loopd;

#ifdef LATENCY_TIMER
        std::cout << "Sync loop duration: " << (loopd / 1000.0) << "ms of the " << syncloopDuration << "ms budget." << std::endl;
#endif

        if (waitd > 0.0)
        {
            std::chrono::microseconds waittime(static_cast<int>(waitd));
            std::this_thread::sleep_for(waittime);
        }
    }
}