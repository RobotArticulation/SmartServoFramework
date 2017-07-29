#ifndef MERCURY_CONTROLLER_H
#define MERCURY_CONTROLLER_H

#include "../ControllerAPI.h"
#include "Mercury.h"

#include "ServoMercury.h"
#include "ServoAX.h"
#include "ServoEX.h"
#include "ServoMX.h"
#include "ServoXL.h"
#include "ServoX.h"

#include <vector>

/** \addtogroup ManagedAPIs
 *  @{
 */

/*!
 * \brief The MercuryController class, part of the ManagedAPI
 *
 * A controller can only be attached to ONE serial link at a time.

 * A "controller" provide a high level API to handle several servos at the same time.
 * A client must instanciate servos objects and register them to a controller.
 * Each servo object is synchronized with its hardware counterpart by the run()
 * method, running in its own backgound thread.
 *
 */
class MercuryController: public Mercury, public ControllerAPI
{
    //! Compute some internal settings (ackPolicy, maxId, protocolVersion) depending on current servo serie and serial device.
    void updateInternalSettings();

    //! Read/write synchronization loop, running inside its own background thread
    void run();

public:
    /*!
     * \brief MercuryController constructor.
     * \param servoSerie: The servo serie to use with this controller. Only used to choose the right communication protocol.
     * \param ctrlFrequency: This is the synchronization frequency between the controller and the servos devices. Range is [1;120], default is 30.
     */
    MercuryController(int ctrlFrequency = 30, int servoSerie = SERVO_MX);

    /*!
     * \brief MercuryController destructor. Stop the controller's thread and close the serial connection.
     */
    ~MercuryController();

    /*!
     * \brief Change communication protocol version for this controller instance.
     * \param protocol: The Mercury communication protocol to use. Can be v1 or v2.
     */
    void changeProtocolVersion(int protocol);

    /*!
     * \brief Connect the controller to a serial port, if the connection is successfull start a synchronization thread.
     * \param devicePath: The serial port device node.
     * \param baud: The serial port speed, can be a baud rate or a 'baudnum'.
     * \param serialDevice: If known, the serial adapter model used by this link.
     * \return 1 if the connection is successfull, 0 otherwise.
     */
    int connect(std::string &devicePath, const int baud, const int serialDevice = SERIAL_UNKNOWN);

    /*!
     * \brief Stop the controller's thread, clean umessage queue, and close the serial connection.
     */
    void disconnect();

    /*!
     * \brief Scan a serial link for Mercury devices.
     * \param start: First ID to be scanned.
     * \param stop: Last ID to be scanned.
     *
     * Note: Be aware that calling this function will reset the current servoList.
     *
     * This scanning function will ping every Mercury ID (from 'start' to 'stop',
     * default [0;253]) on a serial link, and use the status response to detect
     * the presence of a device.
     * When a device is being scanned, its LED is briefly switched on.
     * Every servo found will be automatically registered to this controller.
     *
     * The current value 'protocolVersion' will be used. You can change it with
     * the setProtocolVersion() function before calling autodetect().
     */
    void autodetect_internal(int start = 0, int stop = 253);

    // Wrappers
    std::string serialGetCurrentDevice_wrapper();
    std::vector <std::string> serialGetAvailableDevices_wrapper();
    void serialSetLatency_wrapper(int latency);
};

/** @}*/

#endif // MERCURY_CONTROLLER_H