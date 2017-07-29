#include "mercury/Mercury.h"
#include "minitraces.h"

// C++ standard libraries
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>

/* ************************************************************************** */

// Enable latency timer
//#define LATENCY_TIMER

// Enable packet debugger
//#define PACKET_DEBUGGER

/* ************************************************************************** */

/*!
 * \brief The different instructions available with version 1 of the Mercury protocol.
 */
enum MercuryProtocolV1 {
    INST_PING           = 1,
    INST_READ           = 2,
    INST_WRITE          = 3,
    INST_REG_WRITE      = 4,
    INST_ACTION         = 5,
    INST_FACTORY_RESET  = 6,
    INST_SYNC_WRITE     = 131  // 0x83
};

/*!
 * \brief Addresses of the various fields forming a packet.
 * http://support.robotis.com/en/product/mercury/mercury_communication.htm
 */
enum {
    PKT1_HEADER0        = 0,            //!< "0xFF". The 2 bytes header indicate the beginning of a packet.
    PKT1_HEADER1        = 1,            //!< "0xFF"
    PKT1_ID             = 2,            //!< It is the ID of Mercury device which will receive the Instruction Packet. Range is [0;254].
    PKT1_LENGTH         = 3,            //!< Length of the packet after this field (number of parameters + 2).
    PKT1_INSTRUCTION    = 4,
    PKT1_ERRBIT         = 4,
    PKT1_PARAMETER      = 5
};

/* ************************************************************************** */

Mercury::Mercury()
{
    //
}

Mercury::~Mercury()
{
    serialTerminate();
}

int Mercury::serialInitialize(std::string &devicePath, const int baud)
{
    int status = 0;

    if (serial != nullptr)
    {
        serialTerminate();
    }

    // Instanciate a different serial subclass, depending on the current OS
#if defined(FEATURE_QTSERIAL)
    //serial = new SerialPortQt(devicePath, baud, serialDevice, servoSerie);
#else
#if defined(__linux__) || defined(__gnu_linux)
    serial = new SerialPortLinux(devicePath, baud, serialDevice, servoSerie);
#elif defined(_WIN32) || defined(_WIN64)
    serial = new SerialPortWindows(devicePath, baud, serialDevice, servoSerie);
#elif defined(__APPLE__) || defined(__MACH__)
    serial = new SerialPortMacOS(devicePath, baud, serialDevice, servoSerie);
#else
    #error "No compatible operating system detected!"
#endif
#endif

    // Initialize the serial link
    if (serial != nullptr)
    {
        status = serial->openLink();

        if (status > 0)
        {
            TRACE_INFO(DXL, "> Serial interface successfully opened on '%s' @ %i bps", devicePath.c_str(), baud);
        }
    }
    else
    {
        TRACE_ERROR(DXL, "> Failed to open serial interface on '%s' @ %i bps. Exiting...", devicePath.c_str(), baud);
    }

    return status;
}

void Mercury::serialTerminate()
{
    if (serial != nullptr)
    {
        // Close serial link
        serial->closeLink();
        delete serial;
        serial = nullptr;

        // Clear incoming packet?
        rxPacketSize = 0;
        memset(rxPacket, 0, sizeof(rxPacket));
    }
}

std::string Mercury::serialGetCurrentDevice()
{
    std::string serialName;

    if (serial != nullptr)
    {
        serialName = serial->getDevicePath();
    }
    else
    {
        serialName = "unknown";
    }

    return serialName;
}

std::vector <std::string> Mercury::serialGetAvailableDevices()
{
    std::vector <std::string> devices;

    if (serial == nullptr)
    {
        TRACE_ERROR(DXL, "Serial interface is not initialized!");
    }
    else
    {
        devices = serial->scanSerialPorts();
    }

    return devices;
}

void Mercury::serialSetLatency(int latency)
{
    serial->setLatency(latency);
}

void Mercury::setAckPolicy(int ack)
{
    if (ackPolicy >= ACK_NO_REPLY && ack <= ACK_REPLY_ALL)
    {
        ackPolicy = ack;
    }
    else
    {
        TRACE_ERROR(DXL, "Invalid ack policy: '%i', not in [0;2] range.", ack);
    }
}

void Mercury::mercury_tx_packet()
{
    if (serial == nullptr)
    {
        TRACE_ERROR(DXL, "Serial interface is not initialized!");
        return;
    }

    if (commLock == 1)
    {
        return;
    }
    commLock = 1;

    // Make sure serial link is "clean"
    if (commStatus == COMM_RXTIMEOUT || commStatus == COMM_RXCORRUPT)
    {
        serial->flush();
    }

    // Make sure the packet is properly formed
    if (mercury_validate_packet() == 0)
    {
        return;
    }

    // Generate a checksum and write in into the packet
    mcy_checksum_packet();

    // Send packet
    unsigned char txPacketSize = mercury_get_txpacket_size();
    unsigned char txPacketSizeSent = 0;

    if (serial != nullptr)
    {
        txPacketSizeSent = serial->tx(txPacket, txPacketSize);
    }
    else
    {
        TRACE_ERROR(DXL, "Serial interface has been destroyed!");
        return;
    }

    // Check if we send the whole packet
    if (txPacketSize != txPacketSizeSent)
    {
        commStatus = COMM_TXFAIL;
        commLock = 0;
        return;
    }

    // Set a timeout for the response packet
    if (protocolVersion == PROTOCOL_MCY)
    {
        // 6 is the min size of a v1 status packet
        if (txPacket[PKT1_INSTRUCTION] == INST_READ)
        {
            serial->setTimeOut(6 + txPacket[PKT1_PARAMETER+1]);
        }
        else
        {
            serial->setTimeOut(6);
        }
    }

    commStatus = COMM_TXSUCCESS;
}

void Mercury::mercury_rx_packet()
{
    if (serial == nullptr)
    {
        TRACE_ERROR(DXL, "Serial interface is not initialized!");
        return;
    }

    // No lock mean no packet has just been sent, so why wait for an answer (?)
    if (commLock == 0)
    {
        return;
    }

    // Packet sent to a broadcast address? No need to wait for a status packet.
    if (protocolVersion == PROTOCOL_MCY && txPacket[PKT1_ID] == BROADCAST_ID)
    {
        commStatus = COMM_RXSUCCESS;
        commLock = 0;
        return;
    }

    // Minimum status packet size estimation
    if (commStatus == COMM_TXSUCCESS)
    {
        // Min size with protocol v2 is 11, for v1 is 6
        (protocolVersion == PROTOCOL_DXLv2) ? rxPacketSize = 11 : rxPacketSize = 6;
        rxPacketSizeReceived = 0;
    }

    int nRead = 0;
    if (serial != nullptr)
    {
        // Receive packet
        nRead = serial->rx((unsigned char*)&rxPacket[rxPacketSizeReceived], rxPacketSize - rxPacketSizeReceived);
        rxPacketSizeReceived += nRead;

        // Check if we received the whole packet
        if (rxPacketSizeReceived < rxPacketSize)
        {
            if (serial->checkTimeOut() == 1)
            {
                if (rxPacketSizeReceived == 0)
                {
                    commStatus = COMM_RXTIMEOUT;
                }
                else
                {
                    commStatus = COMM_RXCORRUPT;
                }

                commLock = 0;
                return;
            }
        }
    }
    else
    {
        TRACE_ERROR(DXL, "Serial interface has been destroyed!");
        return;
    }

    // Find packet header
    unsigned char i, j;
    if (protocolVersion == PROTOCOL_DXLv2)
    {
        for (i = 0; i < (rxPacketSizeReceived - 1); i++)
        {
            if (rxPacket[i] == 0xFF &&
                rxPacket[i+1] == 0xFF &&
                rxPacket[i+2] == 0xFD &&
                rxPacket[i+3] == 0x00)
            {
                break;
            }
            else if (i == (rxPacketSizeReceived - 2) &&
                     rxPacket[rxPacketSizeReceived-1] == 0xFF)
            {
                break;
            }
        }

        if (i > 0)
        {
            for (j = 0; j < (rxPacketSizeReceived-i); j++)
            {
                rxPacket[j] = rxPacket[j + i];
            }

            rxPacketSizeReceived -= i;
        }
    }
    else
    {
        for (i = 0; i < (rxPacketSizeReceived - 1); i++)
        {
            if (rxPacket[i] == 0xFF &&
                rxPacket[i+1] == 0xFF)
            {
                break;
            }
            else if (i == (rxPacketSizeReceived - 2) &&
                     rxPacket[rxPacketSizeReceived-1] == 0xFF)
            {
                break;
            }
        }

        if (i > 0)
        {
            for (j = 0; j < (rxPacketSizeReceived-i); j++)
            {
                rxPacket[j] = rxPacket[j + i];
            }

            rxPacketSizeReceived -= i;
        }
    }

    // Incomplete packet?
    if (rxPacketSizeReceived < rxPacketSize)
    {
        commStatus = COMM_RXWAITING;
        return;
    }

    // Check ID pairing
    if ((protocolVersion == PROTOCOL_MCY) && (txPacket[PKT1_ID] != rxPacket[PKT1_ID]))
    {
        commStatus = COMM_RXCORRUPT;
        commLock = 0;
        return;
    }

    // Rx packet size
    rxPacketSize = mercury_get_rxpacket_size();

    if (rxPacketSizeReceived < rxPacketSize)
    {
        nRead = serial->rx(&rxPacket[rxPacketSizeReceived], rxPacketSize - rxPacketSizeReceived);
        rxPacketSizeReceived += nRead;

        if (rxPacketSizeReceived < rxPacketSize)
        {
            commStatus = COMM_RXWAITING;
            return;
        }
    }

    // Generate a checksum of the incoming packet
    if (protocolVersion == PROTOCOL_MCY)
    {
        unsigned char checksum = mcy_checksum_packet(rxPacket, mercury_get_rxpacket_length_field());

        // Compare it with the internal packet checksum
        if (rxPacket[rxPacketSize - 1] != checksum)
        {
            commStatus = COMM_RXCORRUPT;
            commLock = 0;
            return;
        }
    }

    commStatus = COMM_RXSUCCESS;
    commLock = 0;
}

void Mercury::mercury_txrx_packet(int ack)
{
#ifdef LATENCY_TIMER
    // Latency timer for a complete transaction (instruction sent and status received)
    std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
    start = std::chrono::high_resolution_clock::now();
#endif

    mercury_tx_packet();

    if (commStatus != COMM_TXSUCCESS)
    {
        TRACE_ERROR(DXL, "Unable to send TX packet on serial link: '%s'", serialGetCurrentDevice().c_str());
        return;
    }

    // Depending on 'ackPolicy' value and current instruction, we wait for an answer to the packet we just sent
    if (ack == ACK_DEFAULT)
    {
        ack = ackPolicy;
    }

    if (ack != ACK_NO_REPLY)
    {
        int cmd = 0;
        if (protocolVersion == PROTOCOL_MCY)
        {
            cmd = txPacket[PKT1_INSTRUCTION];
        }

        if ((ack == ACK_REPLY_ALL) ||
            (ack == ACK_REPLY_READ && cmd == INST_READ))
        {
            do {
                mercury_rx_packet();
            }
            while (commStatus == COMM_RXWAITING);
        }
        else
        {
            commStatus = COMM_RXSUCCESS;
            commLock = 0;
        }
    }
    else
    {
        commStatus = COMM_RXSUCCESS;
        commLock = 0;
    }

#ifdef PACKET_DEBUGGER
    printTxPacket();
    printRxPacket();
#endif

#ifdef LATENCY_TIMER
    end = std::chrono::high_resolution_clock::now();
    int loopd = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    TRACE_1(DXL, "TX > RX loop: %iµs", loopd);
#endif
}

// Low level API
////////////////////////////////////////////////////////////////////////////////

void Mercury::mercury_set_txpacket_header()
{
    txPacket[PKT1_HEADER0] = 0xFF;
    txPacket[PKT1_HEADER1] = 0xFF;
}

void Mercury::mercury_set_txpacket_id(int id)
{
    if (protocolVersion == PROTOCOL_MCY)
    {
        txPacket[PKT1_ID] = get_lowbyte(id);
    }
}

void Mercury::mercury_set_txpacket_length_field(int length)
{
    if (protocolVersion == PROTOCOL_MCY)
    {
        txPacket[PKT1_LENGTH] = get_lowbyte(length);
    }
}

void Mercury::mercury_set_txpacket_instruction(int instruction)
{
    if (protocolVersion == PROTOCOL_MCY)
    {
        txPacket[PKT1_INSTRUCTION] = get_lowbyte(instruction);
    }
}

void Mercury::mercury_set_txpacket_parameter(int index, int value)
{
    if (protocolVersion == PROTOCOL_MCY)
    {
        txPacket[PKT1_PARAMETER+index] = get_lowbyte(value);
    }
}

void Mercury::mcy_checksum_packet()
{
    if (protocolVersion == PROTOCOL_MCY)
    {
        // Generate checksum
        unsigned char checksum = mcy_checksum_packet(txPacket, mercury_get_txpacket_length_field());

        // Write checksum into the last byte of the packet
        txPacket[mercury_get_txpacket_size() - 1] = checksum;
    }
}

unsigned char Mercury::mcy_checksum_packet(unsigned char *packetData, const int packetLengthField)
{
    unsigned char checksum = 0;

    for (unsigned char i = 0; i < (packetLengthField + 1); i++) // 'length field + 1': whut
    {
        checksum += packetData[i+2];
    }
    checksum = ~checksum;

    return checksum;
}

int Mercury::mercury_get_txpacket_length_field()
{
    int size = -1;

    if (protocolVersion == PROTOCOL_MCY)
    {
        size = static_cast<int>(txPacket[PKT1_LENGTH]);
    }

    return size;
}

int Mercury::mercury_get_txpacket_size()
{
    int size = -1;

    if (protocolVersion == PROTOCOL_MCY)
    {
        // There is 4 bytes before the length field
        size = mercury_get_txpacket_length_field() + 4;
    }

    return size;
}

//int Mercury::mercury_validate_packet()
//{
//    int retcode = -1;

//    if (protocolVersion == PROTOCOL_MCY)
//    {
//        retcode = mercury_validate_packet();
//    }

//    return retcode;
//}

int Mercury::mercury_validate_packet()
{
    int retcode = 1;

    // Check if packet size is valid
    if (mercury_get_txpacket_size() > MAX_PACKET_LENGTH_v1)
    {
        commStatus = COMM_TXERROR;
        commLock = 0;
        retcode = 0;
    }

    // Check if packet instruction is valid
    if (txPacket[PKT1_INSTRUCTION] != INST_PING &&
        txPacket[PKT1_INSTRUCTION] != INST_READ &&
        txPacket[PKT1_INSTRUCTION] != INST_WRITE &&
        txPacket[PKT1_INSTRUCTION] != INST_REG_WRITE &&
        txPacket[PKT1_INSTRUCTION] != INST_ACTION)
    {
        commStatus = COMM_TXERROR;
        commLock = 0;
        retcode = 0;
    }

    // Write sync header
    mercury_set_txpacket_header();

    return retcode;
}

int Mercury::mercury_get_rxpacket_error()
{
    int status = 0;

    if (protocolVersion == PROTOCOL_MCY)
    {
        status = (rxPacket[PKT1_ERRBIT] & 0xFD);
    }

    return status;
}

int Mercury::mercury_get_rxpacket_size()
{
    int size = -1;

    if (protocolVersion == PROTOCOL_MCY)
    {
        // There is 4 bytes before the length field
        size = mercury_get_rxpacket_length_field() + 4;
    }

    return size;
}

int Mercury::mercury_get_rxpacket_length_field()
{
    int size = -1;

    if (protocolVersion == PROTOCOL_MCY)
    {
        size = static_cast<int>(rxPacket[PKT1_LENGTH]);
    }

    return size;
}

int Mercury::mercury_get_rxpacket_parameter(int index)
{
    int value = -1;

    if (protocolVersion == PROTOCOL_MCY)
    {
        value = static_cast<int>(rxPacket[PKT1_PARAMETER + index]);
    }

    return value;
}

int Mercury::mercury_get_last_packet_id()
{
    int id = 0;

    // We want to use the ID of the last status packet received through the serial link
    if (protocolVersion == PROTOCOL_MCY)
    {
        id = (rxPacket[PKT1_ID]);
    }

    // In case no status packet has been received (ex: RX timeout) we try to use the ID from the last packet sent
    if (id == 0)
    {
        if (protocolVersion == PROTOCOL_MCY)
        {
            id = (txPacket[PKT1_ID]);
        }
    }

    return id;
}

int Mercury::mercury_get_com_status()
{
    return commStatus;
}

int Mercury::mercury_get_com_error()
{
    if (commStatus < 0)
    {
        return commStatus;
    }

    return 0;
}

int Mercury::mercury_get_com_error_count()
{
    if (commStatus < 0)
    {
        return 1;
    }

    return 0;
}

int Mercury::mercury_print_error()
{
    int id = mercury_get_last_packet_id(); // Get device id which produce the error
    int error = 0;

    switch (commStatus)
    {
    case COMM_TXSUCCESS:
    case COMM_RXSUCCESS:
        // Get error bitfield
        error = mercury_get_rxpacket_error();

        if (protocolVersion == PROTOCOL_MCY)
        {
            if (error & ERRBIT1_VOLTAGE)
                TRACE_ERROR(MCY, "[#%i] Protocol Error: Input voltage error!", id);
            if (error & ERRBIT1_ANGLE_LIMIT)
                TRACE_ERROR(MCY, "[#%i] Protocol Error: Angle limit error!", id);
            if (error & ERRBIT1_OVERHEAT)
                TRACE_ERROR(MCY, "[#%i] Protocol Error: Overheat error!", id);
            if (error & ERRBIT1_RANGE)
                TRACE_ERROR(MCY, "[#%i] Protocol Error: Out of range value error!", id);
            if (error & ERRBIT1_CHECKSUM)
                TRACE_ERROR(MCY, "[#%i] Protocol Error: Checksum error!", id);
            if (error & ERRBIT1_OVERLOAD)
                TRACE_ERROR(MCY, "[#%i] Protocol Error: Overload error!", id);
            if (error & ERRBIT1_INSTRUCTION)
                TRACE_ERROR(MCY, "[#%i] Protocol Error: Instruction code error!", id);
        }
        break;

    case COMM_UNKNOWN:
        TRACE_ERROR(MCY, "[#%i] COMM_UNKNOWN: Unknown communication error!", id);
        break;

    case COMM_TXFAIL:
        TRACE_ERROR(MCY, "[#%i] COMM_TXFAIL: Failed transmit instruction packet!", id);
        break;

    case COMM_TXERROR:
        TRACE_ERROR(MCY, "[#%i] COMM_TXERROR: Incorrect instruction packet!", id);
        break;

    case COMM_RXFAIL:
        TRACE_ERROR(MCY, "[#%i] COMM_RXFAIL: Failed get status packet from device!", id);
        break;

    case COMM_RXWAITING:
        TRACE_ERROR(MCY, "[#%i] COMM_RXWAITING: Now recieving status packet!", id);
        break;

    case COMM_RXTIMEOUT:
        TRACE_ERROR(MCY, "[#%i] COMM_RXTIMEOUT: Timeout reached while waiting for a status packet!", id);
        break;

    case COMM_RXCORRUPT:
        TRACE_ERROR(MCY, "[#%i] COMM_RXCORRUPT: Status packet is corrupted!", id);
        break;

    default:
        TRACE_ERROR(MCY, "[#%i] commStatus has an unknown error code: '%i'", id, commStatus);
        break;
    }

    return error;
}

void Mercury::printRxPacket()
{
    printf("Packet recv [ ");
    if (protocolVersion == PROTOCOL_MCY)
    {
        printf("0x%.2X 0x%.2X ", rxPacket[0], rxPacket[1]);
        printf("0x%.2X ", rxPacket[2]);
        printf("{0x%.2X} ", rxPacket[3]);
        printf("0x%.2X ", rxPacket[4]);

        // Packet header is 5 bytes (counting length field)
        for (int i = 5; i < (rxPacketSize - 1); i++)
        {
            printf("0x%.2X ", rxPacket[i]);
        }
        printf("{0x%.2X} ", rxPacket[rxPacketSize-1]);
    }
    printf("]\n");
}

void Mercury::printTxPacket()
{
    int packetSize = mercury_get_txpacket_size();

    printf("Packet sent [ ");
    if (protocolVersion == PROTOCOL_MCY)
    {
        printf("0x%.2X 0x%.2X ", txPacket[0], txPacket[1]);
        printf("0x%.2X ", txPacket[2]);
        printf("{0x%.2X} ", txPacket[3]);
        printf("(0x%.2X) ", txPacket[4]);
        for (int i = 5; i < (packetSize - 1); i++)
        {
            printf("0x%.2X ", txPacket[i]);
        }
        printf("{0x%.2X} ", txPacket[packetSize-1]);
    }
    printf("]\n");
}

bool Mercury::mercury_ping(const int id, PingResponse *status, const int ack)
{
    bool retcode = false;

    while(commLock);

    if (protocolVersion == PROTOCOL_MCY)
    {
        txPacket[PKT1_ID] = get_lowbyte(id);
        txPacket[PKT1_INSTRUCTION] = INST_PING;
        txPacket[PKT1_LENGTH] = 2;
    }

    mercury_txrx_packet(ack);

    if (commStatus == COMM_RXSUCCESS)
    {
        retcode = true;

        if (status != nullptr)
        {
            if (protocolVersion == PROTOCOL_MCY)
            {
                // Emulate ping response from protocol v2
                status->model_number = mercury_read_word(id, 0, ack);
                status->firmware_version = mercury_read_byte(id, 2, ack);
            }
        }
    }

    return retcode;
}

void Mercury::mercury_reset(const int id, int setting, const int ack)
{
    while(commLock);

    if (protocolVersion == PROTOCOL_MCY)
    {
        txPacket[PKT1_ID] = get_lowbyte(id);
        txPacket[PKT1_INSTRUCTION] = INST_FACTORY_RESET;
        txPacket[PKT1_LENGTH] = 2;
    }

    mercury_txrx_packet(ack);
}

void Mercury::mercury_reboot(const int id, const int ack)
{
    while(commLock);

    if (protocolVersion == PROTOCOL_MCY)
    {
        commStatus = COMM_TXFAIL;
        TRACE_ERROR(DXL, "'Reboot' instruction not available with protocol v1!");
    }
}

void Mercury::mercury_action(const int id, const int ack)
{
    while(commLock);

    if (protocolVersion == PROTOCOL_MCY)
    {
        txPacket[PKT1_ID] = get_lowbyte(id);
        txPacket[PKT1_INSTRUCTION] = INST_ACTION;
        txPacket[PKT1_LENGTH] = 2;
    }

    mercury_txrx_packet(ack);
}

int Mercury::mercury_read_byte(const int id, const int address, const int ack)
{
    int value = -1;

    if (id == 254)
    {
        TRACE_ERROR(DXL, "Cannot send 'Read' instruction to broadcast address!");
    }
    else if (ack == ACK_NO_REPLY)
    {
        TRACE_ERROR(DXL, "Cannot send 'Read' instruction if ACK_NO_REPLY is set!");
    }
    else
    {
        while(commLock);

        if (protocolVersion == PROTOCOL_MCY)
        {
            txPacket[PKT1_ID] = get_lowbyte(id);
            txPacket[PKT1_INSTRUCTION] = INST_READ;
            txPacket[PKT1_PARAMETER] = get_lowbyte(address);
            txPacket[PKT1_PARAMETER+1] = 1;
            txPacket[PKT1_LENGTH] = 4;
        }

        mercury_txrx_packet(ack);

        if ((ack == ACK_DEFAULT && ackPolicy > ACK_NO_REPLY) ||
            (ack > ACK_NO_REPLY))
        {
            if (commStatus == COMM_RXSUCCESS)
            {
                if (protocolVersion == PROTOCOL_MCY)
                {
                    value = static_cast<int>(rxPacket[PKT1_PARAMETER]);
                }
            }
            else
            {
                value = commStatus;
            }
        }
    }

    return value;
}

void Mercury::mercury_write_byte(const int id, const int address, const int value, const int ack)
{
    while(commLock);

    if (protocolVersion == PROTOCOL_MCY)
    {
        txPacket[PKT1_ID] = get_lowbyte(id);
        txPacket[PKT1_INSTRUCTION] = INST_WRITE;
        txPacket[PKT1_PARAMETER] = get_lowbyte(address);
        txPacket[PKT1_PARAMETER+1] = get_lowbyte(value);
        txPacket[PKT1_LENGTH] = 4;
    }

    mercury_txrx_packet(ack);
}

int Mercury::mercury_read_word(const int id, const int address, const int ack)
{
    int value = -1;

    if (id == 254)
    {
        TRACE_ERROR(DXL, "Error! Cannot send 'Read' instruction to broadcast address!");
    }
    else if (ack == ACK_NO_REPLY)
    {
        TRACE_ERROR(DXL, "Error! Cannot send 'Read' instruction if ACK_NO_REPLY is set!");
    }
    else
    {
        while(commLock);

        if (protocolVersion == PROTOCOL_MCY)
        {
            txPacket[PKT1_ID] = get_lowbyte(id);
            txPacket[PKT1_INSTRUCTION] = INST_READ;
            txPacket[PKT1_PARAMETER] = get_lowbyte(address);
            txPacket[PKT1_PARAMETER+1] = 2;
            txPacket[PKT1_LENGTH] = 4;
        }

        mercury_txrx_packet(ack);

        if ((ack == ACK_DEFAULT && ackPolicy > ACK_NO_REPLY) ||
            (ack > ACK_NO_REPLY))
        {
            if (commStatus == COMM_RXSUCCESS)
            {
                if (protocolVersion == PROTOCOL_MCY)
                {
                    value = make_short_word(rxPacket[PKT1_PARAMETER], rxPacket[PKT1_PARAMETER+1]);
                }
            }
            else
            {
                value = commStatus;
            }
        }
    }

    return value;
}

void Mercury::mercury_write_word(const int id, const int address, const int value, const int ack)
{
    while(commLock);

    if (protocolVersion == PROTOCOL_MCY)
    {
        txPacket[PKT1_ID] = get_lowbyte(id);
        txPacket[PKT1_INSTRUCTION] = INST_WRITE;
        txPacket[PKT1_PARAMETER] = get_lowbyte(address);
        txPacket[PKT1_PARAMETER+1] = get_lowbyte(value);
        txPacket[PKT1_PARAMETER+2] = get_highbyte(value);
        txPacket[PKT1_LENGTH] = 5;
    }

    mercury_txrx_packet(ack);
}
