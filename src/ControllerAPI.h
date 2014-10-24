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
 * \file ControllerAPI.h
 * \date 25/08/2014
 * \author Emeric Grange <emeric.grange@inria.fr>
 */

#ifndef CONTROLLER_API_H
#define CONTROLLER_API_H

#include "Servo.h"
#include "Utils.h"

#include <vector>
#include <deque>
#include <thread>
#include <mutex>

/*!
 * \brief The controllerState enum
 */
enum controllerState
{
    state_stopped = 0,
    state_started,
    state_scanning,
    state_scanned,
    state_reading,
    state_ready,
    state_paused,
};

/*!
 * \brief The ControllerAPI base class.
 * \note This API is considered UNSTABLE at the moment!
 *
 * A "controller" provide a high level API to handle several servos object at the
 * same time. A program must instanciate servos instances and register them to
 * a controller. Each servo object is synchronized with its hardware counterpart
 * by the run() method, running in its own backgound thread.
 */
class ControllerAPI
{
protected:

    enum controllerMessage
    {
        ctrl_device_autodetect = 0,
        ctrl_device_register,
        ctrl_device_unregister,
        ctrl_device_unregister_all,
        ctrl_device_delayed_add,
    };

    struct miniMessages
    {
        controllerMessage msg;
        std::chrono::time_point <std::chrono::system_clock> delay;
        void *p;
        int p1;
        int p2;
    };

    int controllerState;                //!< The current state of the controller, used by client apps to know.

    bool syncloopRunning;
    int syncloopFrequency;              //!< Frequency of the synchronization loop, in Hz. May not be respected if there is too much traffic on the serial port.
    int syncloopCounter;
    double syncloopDuration;            //!< Maximum duration for the synchronization loop, in milliseconds.

    std::thread syncloopThread;

    std::deque<struct miniMessages> m_queue; //!< Message queue.
    std::mutex m_mutex;                 //!< Lock for the message queue.

    std::vector <Servo *> servoList;    //!< List containing device object managed by this controller.
    std::mutex servoListLock;           //!< Lock for the device list.

    std::vector <int> updateList;       //!< List of device object marked for a "full" register update.
    std::vector <int> syncList;         //!< List of device object to keep in sync.

    //! Store the number of transmission errors (should be reseted every X seconds?)
    int errors;

    //! Read/write synchronization loop, running inside its own background thread
    virtual void run() = 0;

    /*!
     * \brief Internal thread messaging system.
     * \param m: A pointer to a miniMessages structure. Will be copied.
     *
     * Push a message into the back of a deque if the thread is running, otherwise
     * messages will be discarded.
     */
    void sendMessage(miniMessages *m);

    void registerServo_internal(Servo *servo);
    void unregisterServo_internal(Servo *servo);
    void unregisterServos_internal();
    int delayedAddServos_internal(std::chrono::time_point<std::chrono::_V2::system_clock> delay, int id, int update);
    virtual void autodetect_internal(int start = 0, int stop = 253) = 0;
    void pauseThread_internal();

public:
    /*!
     * \brief ControllerAPI constructor.
     * \param freq: This is the synchronization frequency between the controller and the servos devices. Range is [1;120], default is 30.
     */
    ControllerAPI(int freq = 30);

    /*!
     * \brief ControllerAPI destructor. Stop the controller's thread and close the serial connection.
     */
    virtual ~ControllerAPI();

    /*!
     * \brief getState
     * \return The current state of the controller.
     */
    int getState() { return controllerState; }

    /*!
     * \brief Wait until the controller is ready.
     * \return true if the controller is ready, false if timeout has been hit and the controller status is unknown.
     *
     * You need to call this function after an autodetection or registering servos
     * manually, to let the controller some time to process new devices.
     * This is a blocking function that only return after
     */
    bool waitUntilReady();

    /*!
     * \brief Scan a serial link for servo or sensor devices.
     * \param start: First ID to be scanned.
     * \param stop: Last ID to be scanned.
     *
     * Please be aware that this function will stop the controller thread and
     * reset its servoList.
     *
     * This scanning function will ping every device ID (from 'start' to 'stop',
     * default [0;253]) on a serial link, and use the status response to detect
     * the presence of a device.
     * When a device is being scanned, its LED is briefly switched on.
     *
     * Every servo found will be registered to this controller.
     */
    void autodetect(int start = 0, int stop = 253);

    virtual int serialInitialize_wrapper(std::string &deviceName, const int baud, const int serialDevice = 0) = 0;
    virtual void serialTerminate_wrapper() = 0;
    virtual std::string serialGetCurrentDevice_wrapper() = 0;
    virtual std::vector <std::string> serialGetAvailableDevices_wrapper() = 0;
    virtual void serialSetLatency_wrapper(int latency) = 0;

    /*!
     * \brief clearMessageQueue
     */
    void clearMessageQueue();

    /*!
     * \brief Register a servo given in argument.
     * \param servo: A servo instance.
     *
     * Register the servo given in argument. It will be added to the controller
     * and thus added to the "synchronization loop".
     */
    void registerServo(Servo *servo);

    /*!
     * \brief Create and register a servo with the ID given in argument.
     * \param id: A servo id.
     *
     * Register the servo given in argument. It will be added to the controller
     * and thus added to the "synchronization loop".
     */
    void registerServo(int id);

    /*!
     * \brief Unregister a servo given in argument.
     * \param servo: A servo instance.
     */
    void unregisterServo(Servo *servo);

    /*!
     * \brief Unregister a servo with the ID given in argument.
     * \param id: A servo id.
     */
    void unregisterServo(int id);

    /*!
     * \brief Return a servo instance corresponding to the id given in argument.
     * \param id: The ID of the servo we want.
     * \return A servo instance.
     */
    Servo *getServo(const int id);

    /*!
     * \brief Return all servo instances registered to this controller.
     * \return A read only list of servo instances registered to this controller.
     */
    const std::vector <Servo *> getServos();

    /*!
     * \brief Return the number of error logged on the serial link associated with this controller.
     */
    int getErrorCount()
    {
        return errors;
    }

    /*!
     * \brief Reset error count for this controller.
     */
    void clearErrors()
    {
        errors = 0;
    }

    /*!
     * \brief Start synchronization loop thread.
     */
    void startThread();

    /*!
     * \brief Pause/un-pause synchronization loop thread.
     */
    void pauseThread();

    /*!
     * \brief Stop synchronization loop thread. All servos are deleted from its internal lists.
     */
    void stopThread();
};

#endif /* CONTROLLER_API_H */