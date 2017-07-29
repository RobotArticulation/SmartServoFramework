#ifndef MERCURY_TOOLS_H
#define MERCURY_TOOLS_H

#include "../Utils.h"
#include <string>

/*!
 * \brief Max packet size with Mercury communication protocol v1.
 * The '150' bytes size limit seems to be arbitrary, and the real limit might
 * be depending on the RX buffer size of particular servo models.
 */
#define MAX_PACKET_LENGTH_v1    (150)

/*!
 * \brief The different errors available through the error bitfield for Mercury protocol v1.
 */
enum {
    ERRBIT1_VOLTAGE      = 0x01,
    ERRBIT1_ANGLE_LIMIT  = 0x02,
    ERRBIT1_OVERHEAT     = 0x04,
    ERRBIT1_RANGE        = 0x08,
    ERRBIT1_CHECKSUM     = 0x10,
    ERRBIT1_OVERLOAD     = 0x20,
    ERRBIT1_INSTRUCTION  = 0x40
};

/*!
 * \brief Get a Mercury model name from a model number.
 * \param model: The model number of the Mercury servo or sensor.
 * \return A string containing the textual name for the device.
 *
 * This function does not handle PRO serie yet.
 */
std::string mcy_get_model_name(const int model);

/*!
 * \brief Get a Mercury serie (V1, ...) from a model number.
 * \param[in] model_number: The model number of the Mercury servo or sensor.
 * \param[out] servo_serie: A servo serie from ServoDevices_e enum.
 * \param[out] servo_model: A servo model from ServoDevices_e enum.
 *
 * This function does not handle PRO serie yet.
 */
void mcy_get_model_infos(const int model_number, int &servo_serie, int &servo_model);

/*!
 * \brief Return a Mercury model (V1, ...) from a model number.
 * \param model_number: The model number of a Mercury servo.
 * \return A servo model from ServoDevices_e enum.
 */
int mcy_get_servo_model(const int model_number);

/*!
 * \brief Convert a Mercury "baudnum" to a baudrate in bps.
 * \param baudnum: Mercury "baudnum", from 0 to 254.
 * \param servo_serie: The servo serie, used to compute adequate baudrate.
 * \return A valid baudrate, from 2400 to 1000000 bps.
 *
 * The baudrate is usually computed from baudnum using the following formula:
 * Speed(baudrate) = 2000000/(baudnum+1).
 * Valid baudnum values are in range 1 to 254, which gives us baudrate values
 * of 1MB/s to 7,84kB/s.
 */
int mcy_get_baudrate(const int baudnum, const int servo_serie = SERVO_AX);

#endif // MERCURY_TOOLS_H
