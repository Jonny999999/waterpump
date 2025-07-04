#pragma once

#include "driver/mcpwm_prelude.h"


// configuration struct for a servo motor
typedef struct servoConfig_t
{
    int gpioPwmSignal;
    int gpioEnablePower;    // pin controlled by enable() and disable(), no effect if feature disabled below
    bool powerEnableRequired; // gpioEnablePower has to be turned on with .enable() method
    int ratedAngle;
    int minAllowedAngle;
    int maxAllowedAngle;
    bool invertDirection;
    float backlashCompensationDeg = 0.0; // Additional degrees rotated when direction changes (default 0 = disabled)
} servoConfig_t;


// class for controlling a servo motor via PWM signal
class ServoMotor
{
public:
    ServoMotor(servoConfig_t config);
    void enable(); //turn configured power pin on (if required)
    void disable(); //turn configured power pin off, and prevent further movement
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
    float mBacklashOffset = 0;
    float mPreviousAngle = 0.0; // To track last target, for backlash compensation
    bool isEnabled = false;
};