extern "C"
{
#include "esp_log.h"
#include "esp_timer.h"
}
#include <math.h>

#include "servo.hpp"
#include "vfd.hpp"
#include "pumpControl.hpp"

#define getMs() (esp_timer_get_time() / 1000)

// TODO adjust parameters
// TODO smaller steps
#define Kp 10     // proportional gain
#define Ki 0.0001 // integral gain
#define Kd 0
#define OFFSET (100 - 20) // 0 fully open, 100 fully closed - idle valve position (expected working point)
#define MAX_INTEGRAL 60   // maximum valve percentage integral term can apply (prevent windup)
#define MIN_VALVE_MOVE_ANGLE 2
#define MAX_DT_MS 5000 // prevent bugged action with large time delta at first after several seconds

//=================================
//== ControlledValve Constructor ==
//=================================
ControlledValve::ControlledValve(ServoMotor *pValve)
{
    // copy pointer and config values
    mpValve = pValve;
    mKp = Kp;
    mKi = Ki;
    mKd = Kd;
}

//=====================
//====== reset() ======
//=====================
void ControlledValve::reset(){
    ESP_LOGW("regulateValve", "reset - set integral from %.3f to 0", mIntegralAccumulator);
    mIntegralAccumulator = 0;
}


//=======================
//==== get variables ====
//=======================
void ControlledValve::getCurrentStats(uint32_t *timestampLastUpdate, float *p, float *i, float *d, float *valvePos) const
{
    // TODO mutex
    *p = mProportional;
    *i = mIntegral;
    *d = mDerivative;
    *valvePos = mTargetValvePos;
    *timestampLastUpdate = mTimestampLastRun;
}
void ControlledValve::getCurrentSettings(float *kp, float *ki, float *kd) const
{
    *kp = mKp;
    *ki = mKi;
    *kd = mKd;
}

//===================================
//==== ControlledValve compute() ====
//===================================
// calculate and move valve to new position
void ControlledValve::compute(float pressureDiff)
{
    // variables
    uint32_t dt, timeNow; // in milliseconds
    double dp;
    static float pressureDiffLast = 0;

    // calculate time passed since last run
    timeNow = getMs();
    dt = timeNow - mTimestampLastRun;
    mTimestampLastRun = timeNow;
    // ignore run with very large dt (probably first run of method since startup)
    if (dt > MAX_DT_MS)
    {
        dt = 0;
        ESP_LOGE("regulateValve", "dt too large (%ldms), ignoring this run", dt);
        return;
    }
    // calculate pressure change since last run
    dp = pressureDiff - pressureDiffLast;
    pressureDiffLast = pressureDiff;

    // integrate
    mIntegralAccumulator += pressureDiff * dt;
    // limit to max (prevents windup)
    if (mIntegralAccumulator * Ki > MAX_INTEGRAL)
    {
        mIntegralAccumulator = MAX_INTEGRAL / Ki;
    }
    else if (mIntegralAccumulator * Ki < -MAX_INTEGRAL)
    {
        mIntegralAccumulator = -MAX_INTEGRAL / Ki;
    }

    // --- integral term ---
    mIntegral = Ki * mIntegralAccumulator;
    // --- proportional term ---
    mProportional = Kp * pressureDiff;
    // --- derivative term ---
    mDerivative = Kd * dp / dt;
    // calculate total output
    // note: output positive = pressure too low -> valve has to close -> reduce pos
    mOutput = OFFSET + mProportional + mIntegral + mDerivative;

    // define new valve position
    float oldPos = mpValve->getPercent();
    // invert valve pos (output: percentage valve is CLOSED, servo: percentage valve is OPEN)
    // TODO invert servo direction
    mTargetValvePos = 100 - mOutput;
    // Ensure newPos is within the valid range [0, 100]
    if (mTargetValvePos < 0)
    {
        mTargetValvePos = 0;
    }
    else if (mTargetValvePos > 100)
    {
        mTargetValvePos = 100;
    }

    // move valve to new pos
    // only move if threshold exceeded to reduce osciallation and unnecessary hardware wear
    if (fabs(oldPos - mTargetValvePos) > MIN_VALVE_MOVE_ANGLE)
    {
        mpValve->setPercentage(mTargetValvePos);
        ESP_LOGI("regulateValve", "moving valve by %.2f%% degrees to %.2f%%", oldPos - mTargetValvePos, mTargetValvePos);
    }

    // debug log
    ESP_LOGD("regulateValve", "diff=%.2fbar, dt=%dms, P=%.2f%%, I=%.2f%%, valvePrev=%.2f%%, valveTarget=%.2f%% ",
             pressureDiff, (int)dt, mProportional, mIntegral, oldPos, mTargetValvePos);
}




//=======================
//==== regulateMotor ====
//=======================
#define MAX_SPEED_LEVEL 3
#define MIN_SPEED_LEVEL 1
// TODO: adjust thresholds:
#define VALVE_PERCENT_TOO_SLOW 5       // speed up when valve below that position and pressure too low
#define VALVE_PERCENT_TOO_FAST 80      // slow down when valve above that position and pressure too high
#define CHANGE_SPEED_WAIT_TIMEOUT 3000 // ms threshold speed change conditions have to be met before change is made
void regulateMotor(float pressureDiff, ServoMotor *pValve, Vfd4DigitalPins *pMotor)
{
    static int integralSpeedChangePlanned = 0;
    static uint32_t timestampLastRunMs = 0;

    // calculate passed time since last run
    uint32_t timestampNow = getMs();
    uint32_t dt = timestampNow - timestampLastRunMs;
    timestampLastRunMs = timestampNow;
    // get current motor speed level
    int currentSpeedLevel = pMotor->getSpeedLevel();

    // evaluate if speed is too low/high
    // pressure too low but valve already almost compeltely closed => speed up?
    if (pressureDiff > 0 && pValve->getPercent() < 5 && currentSpeedLevel < MAX_SPEED_LEVEL)
    {
        integralSpeedChangePlanned += 1 * dt;
        ESP_LOGD("regulateMotor", "motor seems too slow, incrementing time by %ld to %d", dt, integralSpeedChangePlanned);
    }
    // pressure too high but valve already wide open => slow down?
    else if (pressureDiff < 0.5 && pValve->getPercent() > 80 && currentSpeedLevel > MIN_SPEED_LEVEL)
    {
        integralSpeedChangePlanned -= 1 * dt;
        ESP_LOGD("regulateMotor", "motor seems too fast, decrementing time by %ld to %d", dt, integralSpeedChangePlanned);
    }
    else
    {
        // reset tracked time when in valid working range
        integralSpeedChangePlanned = 0;
    }

    if (integralSpeedChangePlanned > CHANGE_SPEED_WAIT_TIMEOUT)
    {
        pMotor->setSpeedLevel(++currentSpeedLevel);
        ESP_LOGW("regulateMotor", "valve below %d%% for too long -> increasing motor speed to %d", VALVE_PERCENT_TOO_SLOW, currentSpeedLevel); // todo also log variables
        integralSpeedChangePlanned = 0;
    }
    else if (integralSpeedChangePlanned < -CHANGE_SPEED_WAIT_TIMEOUT)
    {
        pMotor->setSpeedLevel(--currentSpeedLevel);
        ESP_LOGW("regulateMotor", "valve above %d%% for too long -> decreasing motor speed to %d", VALVE_PERCENT_TOO_FAST, currentSpeedLevel); // todo also log variables
        integralSpeedChangePlanned = 0;
    }
}