#ifndef CONTROL_TABLES_MERCURY_H
#define CONTROL_TABLES_MERCURY_H
/* ************************************************************************** */

#include "../Utils.h"

/* ************************************************************************** */

/** \addtogroup ControlTables
 *  @{
 */

/*!
 * \brief V1 control table.
 *
 * V1 series are using the exact same control table.
 */
const int V1_control_table[33][8] =
{
    // Instruction // size // RW Access // ROM addr // RAM addr // initial value // min value // max value

    // Control table // EEPROM area
    { REG_MODEL_NUMBER         , 2, READ_ONLY,   0, -1,   -2,   -1,   -1 },
    { REG_FIRMWARE_VERSION     , 1, READ_ONLY,   2, -1,   -2,   -1,   -1 },
    { REG_ID                   , 1, READ_WRITE,  3, -1,    1,    0,  253 },
    { REG_BAUD_RATE            , 1, READ_WRITE,  4, -1,    1,    1,  254 },
    { REG_RETURN_DELAY_TIME    , 1, READ_WRITE,  5, -1,  250,    0,  254 },
    { REG_MIN_POSITION         , 2, READ_WRITE,  6, -1,    0,    0, 1023 },
    { REG_MAX_POSITION         , 2, READ_WRITE,  8, -1, 1023,    0, 1023 },
    { REG_TEMPERATURE_LIMIT    , 1, READ_WRITE, 11, -1,   65,    0,  150 },
    { REG_VOLTAGE_LOWEST_LIMIT , 1, READ_WRITE, 12, -1,   90,   50,  250 },
    { REG_VOLTAGE_HIGHEST_LIMIT, 1, READ_WRITE, 13, -1,  120,   50,  250 },
    { REG_MAX_TORQUE           , 2, READ_WRITE, 14, -1, 1023,    0, 1023 },
    { REG_STATUS_RETURN_LEVEL  , 1, READ_WRITE, 16, -1,    2,    0,    2 },
    { REG_ALARM_LED            , 1, READ_WRITE, 17, -1,   36,    0,  127 },
    { REG_ALARM_SHUTDOWN       , 1, READ_WRITE, 18, -1,   36,    0,  127 },
    // Control table // RAM area
    { REG_TORQUE_ENABLE        , 1, READ_WRITE, -1, 24,    0,    0,    1 },
    { REG_LED                  , 1, READ_WRITE, -1, 25,    0,    0,    1 },
    { REG_CW_COMPLIANCE_MARGIN , 1, READ_WRITE, -1, 26,    0,    0,   255},
    { REG_CCW_COMPLIANCE_MARGIN, 1, READ_WRITE, -1, 27,    0,    0,   255},
    { REG_CW_COMPLIANCE_SLOPE  , 1, READ_WRITE, -1, 28,    0,    2,   128},
    { REG_CCW_COMPLIANCE_SLOPE , 1, READ_WRITE, -1, 29,    0,    2,   128},
    { REG_GOAL_POSITION        , 2, READ_WRITE, -1, 30,   -1,    0, 1023 },
    { REG_GOAL_SPEED           , 2, READ_WRITE, -1, 32,   -1,    0, 1023 },
    { REG_TORQUE_LIMIT         , 2, READ_WRITE, -1, 34,   -1,    0, 1023 },
    { REG_CURRENT_POSITION     , 2, READ_ONLY,  -1, 36,   -1,   -1,   -1 },
    { REG_CURRENT_SPEED        , 2, READ_ONLY,  -1, 38,   -1,   -1,   -1 },
    { REG_CURRENT_LOAD         , 2, READ_ONLY,  -1, 40,   -1,   -1,   -1 },
    { REG_CURRENT_VOLTAGE      , 1, READ_ONLY,  -1, 42,   -1,   -1,   -1 },
    { REG_CURRENT_TEMPERATURE  , 1, READ_ONLY,  -1, 43,   -1,   -1,   -1 },
    { REG_REGISTERED           , 1, READ_ONLY,  -1, 44,    0,   -1,   -1 },
    { REG_MOVING               , 1, READ_ONLY,  -1, 46,    0,   -1,   -1 },
    { REG_LOCK                 , 1, READ_WRITE, -1, 47,    0,    0,    1 },
    { REG_PUNCH                , 2, READ_WRITE, -1, 48,   32,    0, 1023 },
    { 999, 999, 999, 999, 999, 999, 999, 999 },
};

/** @}*/

/* ************************************************************************** */
#endif // CONTROL_TABLES_MERCURY_H
