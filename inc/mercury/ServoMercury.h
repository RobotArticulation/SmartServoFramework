#ifndef SERVO_MERCURY_H
#define SERVO_MERCURY_H

#include "../Servo.h"

#include <string>
#include <map>
#include <mutex>

/** \addtogroup ManagedAPIs
 *  @{
 */

/*!
 * \brief The Mercury servo class.
 * \ref ServoV1
 */
class ServoMercury: public Servo
{
    int speedMode;  //!< Control the servo with manual or 'automatic' speed mode, using '::SpeedMode_e' enum.

public:
    ServoMercury(const int control_table[][8], int mercury_id, int mercury_model, int speed_mode = SPEED_MANUAL);
    virtual ~ServoMercury() = 0;

    // Device
    void status();
    std::string getModelString();
    void getModelInfos(int &servo_serie, int &servo_model);

    // Helpers
    int getSpeedMode();
    void setSpeedMode(int speed_mode);
    void waitMovementCompletion(int timeout_ms = 5000);

    // Getters
    int getBaudRate();
    int getReturnDelay();

    double getHighestLimitTemp();
    double getLowestLimitVolt();
    double getHighestLimitVolt();

    int getMaxTorque();

    int getGoalPosition();
    int getMovingSpeed();

    int getTorqueLimit();
    int getCurrentPosition();
    int getCurrentSpeed();
    int getCurrentLoad();
    double getCurrentVoltage();
    double getCurrentTemperature();

    int getRegistered();
    int getMoving();
    int getLock();
    int getPunch();

    // Setters
    void setId(int id);
    void setCWLimit(int limit);
    void setCCWLimit(int limit);
    void setGoalPosition(int pos);
    void setGoalPosition(int pos, int time_budget_ms);
    void moveGoalPosition(int move);
    void setMovingSpeed(int speed);
    void setMaxTorque(int torque);

    void setLed(int led);
    void setTorqueEnabled(int torque);
};

/** @}*/

#endif // SERVO_MERCURY_H
