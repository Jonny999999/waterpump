#pragma once

#include "driver/mcpwm_prelude.h"


// configuration struct for a servo motor
typedef struct servoConfig_t
{
    int gpioPwmSignal;
    int ratedAngle;
    int minAllowedAngle;
    int maxAllowedAngle;
    bool invertDirection;
} servoConfig_t;


// class for controlling a servo motor via PWM signal
class ServoMotor
{
public:
    ServoMotor(servoConfig_t config);
    void setAngle(float newAngle); // move to absolute (e.g. 0-180) angle, note: will limit to allowed range
    void setPercentage(float newPosition); // move to percentage within configured allowed range: 0=minAllowed, 100=maxAllowed
    void runTestDrive();
    float getAngle() const {return mCurrentAngle;};
    float getPercent() const;

private:
    // methods
    void init();
    uint32_t angleToCompareValue(float angle) const;
    float absAngleToRelPercent(float angle) const;
    float relPercentToAbsAngle(float percent) const;
    // variables
    servoConfig_t mConfig;
    mcpwm_cmpr_handle_t mComparator = NULL;
    float mCurrentAngle = 0;
};