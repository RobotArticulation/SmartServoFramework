#ifndef MERCURY_H
#define MERCURY_H

#include "../SerialPortQt.h"
#include "../SerialPortLinux.h"
#include "../SerialPortWindows.h"
#include "../SerialPortMacOS.h"

#include "../Utils.h"
#include "../ControlTables.h"
#include "MercuryTools.h"

#include <string>
#include <vector>

/*!
 * \brief The Mercury communication protocols implementation
 * \todo Rename to MercuryProtocol
 * \todo Handle "sync" and "bulk" read/write operations.
 *
 * This class provide the low level API to handle communication with servos.
 * It can generate instruction packets and send them over a serial link. This class
 * will be used by both "SimpleAPIs" and "Controllers".
 */
class Mercury
{
private:
    SerialPort *serial = nullptr;   //!< The serial port instance we are going to use.

    unsigned char txPacket[MAX_PACKET_LENGTH_v1] = {0};  //!< TX "instruction" packet buffer
    unsigned char rxPacket[MAX_PACKET_LENGTH_v1] = {0};  //!< RX "status" packet buffer
    int rxPacketSize = 0;           //!< Size of the incoming packet
    int rxPacketSizeReceived = 0;   //!< Byte(s) of the incoming packet received from the serial link

    /*!
     * The software lock used to lock the serial interface, to avoid concurent
     * reads/writes that would lead to multiplexing and packet corruptions.
     * We need one lock per Mercury (or MercuryController) instance because
     * we want to keep the ability to use multiple serial interface simultaneously
     * (ex: /dev/tty0 and /dev/ttyUSB0).
     */
    int commLock = 0;
    int commStatus = COMM_RXSUCCESS;//!< Last communication status

    // Serial communication methods, using one of the SerialPort[Linux/Mac/Windows] implementations.
    void mercury_tx_packet();
    void mercury_rx_packet();
    void mercury_txrx_packet(int ack);

protected:
    Mercury();
    virtual ~Mercury() = 0;

    int serialDevice = SERIAL_UNKNOWN;      //!< Serial device in use (if known) using '::SerialDevices_e' enum. Can affect link speed and latency.
    int servoSerie = SERVO_MX;              //!< Servo serie using '::ServoDevices_e' enum. Used internally to setup some parameters like maxID, ackPolicy and protocolVersion.

    int protocolVersion = PROTOCOL_MCY;   //!< Version of the communication protocol in use.
    int maxId = 252;                        //!< Store in the maximum value for servo IDs.
    int ackPolicy = ACK_REPLY_ALL;          //!< Set the status/ack packet return policy using '::AckPolicy_e' (0: No return; 1: Return for READ commands; 2: Return for all commands).

    // Handle serial link
    ////////////////////////////////////////////////////////////////////////////

    /*!
     * \brief Open a serial link with the given parameters.
     * \param devicePath: The path to the serial device node.
     * \param baud: The baudrate or Mercury 'baudnum'.
     * \return 1 if success, 0 if locked, -1 otherwise.
     */
    int serialInitialize(std::string &devicePath, const int baud);

    /*!
     * \brief Make sure the serial link is properly closed.
     */
    void serialTerminate();

    // Low level API
    ////////////////////////////////////////////////////////////////////////////

    // TX packet building
    void mercury_set_txpacket_header();
    void mercury_set_txpacket_id(int id);
    void mercury_set_txpacket_length_field(int length);
    void mercury_set_txpacket_instruction(int instruction);
    void mercury_set_txpacket_parameter(int index, int value);

    void mcy_checksum_packet();    //!< Generate and write a checksum of tx packet payload
    unsigned char mcy_checksum_packet(unsigned char *packetData, const int packetLengthField);

    // TX packet analysis
    int mercury_get_txpacket_size();
    int mercury_get_txpacket_length_field();
    int mercury_validate_packet();

    // RX packet analysis
    int mercury_get_rxpacket_error();
    int mercury_get_rxpacket_size();
    int mercury_get_rxpacket_length_field();
    int mercury_get_rxpacket_parameter(int index);

    // Debug methods
    int mercury_get_last_packet_id();
    int mercury_get_com_status();       //!< Get communication status (commStatus) of the latest TX/RX instruction
    int mercury_get_com_error();        //!< Get communication error (if commStatus is an error) of the latest TX/RX instruction
    int mercury_get_com_error_count();  //!< 1 if commStatus is an error, 0 otherwise
    int mercury_print_error();          //!< Print the last communication error
    void printRxPacket();           //!< Print the RX buffer (last packet received)
    void printTxPacket();           //!< Print the TX buffer (last packet sent)

    // Instructions
    bool mercury_ping(const int id, PingResponse *status = nullptr, const int ack = ACK_DEFAULT);

    /*!
     * \brief Reset servo control table.
     * \param id: The servo to reset to factory default settings.
     * \param setting: If protocol v2 is used, you can control what to erase using the 'ResetOptions' enum.
     * \param ack: Ack policy in effect.
     *
     * \todo emulate "RESET_ALL_EXCEPT_ID" and "RESET_ALL_EXCEPT_ID_BAUDRATE" settings when using protocol v1?
     * Please note that when using protocol v1, the servo ID will be changed to 1.
     */
    void mercury_reset(const int id, int setting, const int ack = ACK_DEFAULT);
    void mercury_reboot(const int id, const int ack = ACK_DEFAULT);
    void mercury_action(const int id, const int ack = ACK_DEFAULT);

    // DOCME // Read/write register instructions
    int mercury_read_byte(const int id, const int address, const int ack = ACK_DEFAULT);
    void mercury_write_byte(const int id, const int address, const int value, const int ack = ACK_DEFAULT);
    int mercury_read_word(const int id, const int address, const int ack = ACK_DEFAULT);
    void mercury_write_word(const int id, const int address, const int value, const int ack = ACK_DEFAULT);
public:
    /*!
     * \brief Get the name of the serial device associated with this Mercury instance.
     * \return The path to the serial device node (ex: "/dev/ttyUSB0").
     */
    std::string serialGetCurrentDevice();

    /*!
     * \brief Get the available serial devices.
     * \return A list of path to all the serial device nodes available (ex: "/dev/ttyUSB0").
     */
    std::vector <std::string> serialGetAvailableDevices();

    /*!
     * \brief serialSetLatency
     * \param latency: Latency value in milliseconds.
     */
    void serialSetLatency(int latency);

    /*!
     * \brief setAckPolicy
     * \param ack: Ack policy value, using '::AckPolicy_e' enum.
     */
    void setAckPolicy(int ack);
};

#endif // MERCURY_H
