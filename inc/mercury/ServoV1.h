#ifndef SERVO_V1_H
#define SERVO_V1_H

#include "ServoMercury.h"

#include <string>
#include <map>
#include <mutex>

/** \addtogroup ManagedAPIs
 *  @{
 */

/*!
 * \brief V1 servo series.
 * \ref ServoMercury
 * \ref V1_control_table
 */
class ServoV1: public ServoMercury
{
public:
    ServoV1(int mercury_id, int mercury_model, int control_mode = SPEED_MANUAL);
    ~ServoV1();

    // Getters
    int getCwComplianceMargin();
    int getCcwComplianceMargin();
    int getCwComplianceSlope();
    int getCcwComplianceSlope();
};

/** @}*/

#endif // SERVO_V1_H
