#ifndef SERVO_AX_H
#define SERVO_AX_H

#include "ServoMercury.h"

#include <string>
#include <map>
#include <mutex>

/** \addtogroup ManagedAPIs
 *  @{
 */

/*!
 * \brief AX/DX/RX servo series.
 * \ref ServoMercury
 * \ref AXDXRX_control_table
 *
 * More informations about them on Robotis website:
 * - http://www.robotis.us/ax-series/
 * - http://www.robotis.us/rx-series/
 * - http://support.robotis.com/en/product/actuator/mercury/mercury_ax_main.htm
 * - http://support.robotis.com/en/product/actuator/mercury/mercury_dx_main.htm
 * - http://support.robotis.com/en/product/actuator/mercury/mercury_rx_main.htm
 */
class ServoAX: public ServoMercury
{
public:
    ServoAX(int mercury_id, int mercury_model, int control_mode = SPEED_MANUAL);
    ~ServoAX();

    // Getters
    int getCwComplianceMargin();
    int getCcwComplianceMargin();
    int getCwComplianceSlope();
    int getCcwComplianceSlope();
};

/** @}*/

#endif // SERVO_AX_H
