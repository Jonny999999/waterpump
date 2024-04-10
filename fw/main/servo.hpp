#pragma once

#include "driver/mcpwm_prelude.h"

typedef struct servoConfig_t
{
    int gpioPwmSignal;
    int minAngle;
    int maxAngle;
    bool invertDirection;
} servoConfig_t;

class ServoMotor
{
public:
    ServoMotor(servoConfig_t config);
    void setAngle(float newAngle);
    void runTestDrive();
    float getAngle() const {return mCurrentAngle;};

private:
    void init();
    uint32_t angleToCompareValue(float angle);
    servoConfig_t mConfig;
    mcpwm_cmpr_handle_t comparator = NULL;
    float mCurrentAngle = 0;
};