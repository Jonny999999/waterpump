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
#define Ki 0.005 // integral gain
#define Kd 1000
#define OFFSET (100 - 30) // 0 fully open, 100 fully closed - idle valve position (expected working point)
#define MAX_INTEGRAL_RANGE_MULTIPLIER 1  // scale maximum percentage integral term can apply to valve
// 1 means: with integral value alone valve can reach 0 and 100 
// >1 means: integral can overshoot (danger of windup)
// <0 means: integral can not influence the result that much
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
    mOffset = OFFSET;
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
void ControlledValve::getCurrentStats(uint32_t *timestampLastUpdate, float *pressureDiff, float * targetPressure, float *p, float *i, float *d, float *valvePos) const
{
    // TODO mutex
    *timestampLastUpdate = mTimestampLastRun;
    *pressureDiff = mPressureDiffLast;
    *targetPressure = mTargetPressure;
    *p = mProportional;
    *i = mIntegral;
    *d = mDerivative;
    *valvePos = mTargetValvePos;
}
void ControlledValve::getCurrentSettings(double *kp, double *ki, double *kd, double *offset) const
{
    *kp = mKp;
    *ki = mKi;
    *kd = mKd;
    *offset = mOffset;
}


//=======================
//==== set variables ====
//=======================
void ControlledValve::setKp(double KpNew)
{
    ESP_LOGW("regulateValve", "changing Kp from %lf to %lf", mKp, KpNew);
    mKp = KpNew;
}
void ControlledValve::setKi(double KiNew)
{
    ESP_LOGW("regulateValve", "changing Ki from %lf to %lf", mKi, KiNew);
    mKi = KiNew;
}
void ControlledValve::setKd(double KdNew)
{
    ESP_LOGW("regulateValve", "changing Kp from %lf to %lf", mKd, KdNew);
    mKd = KdNew;
}
void ControlledValve::setOffset(double offsetNew)
{
    ESP_LOGW("regulateValve", "changing Offset from %lf to %lf", mOffset, offsetNew);
    mOffset = offsetNew;
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
    dp = pressureDiff - mPressureDiffLast;
    mPressureDiffLast = pressureDiff;

    // integrate
    mIntegralAccumulator += pressureDiff * dt;
    // limit to max (prevents windup)
    // calculate integral limits depending on offset TODO: do this on offset change only
    float maxIntegral = (100-mOffset) * MAX_INTEGRAL_RANGE_MULTIPLIER;
    float minIntegral = (-mOffset) * MAX_INTEGRAL_RANGE_MULTIPLIER;
    if (mIntegralAccumulator * mKi > maxIntegral)
    {
        mIntegralAccumulator = maxIntegral / mKi;
    }
    else if (mIntegralAccumulator * mKi < minIntegral)
    {
        mIntegralAccumulator = minIntegral / mKi;
    }

    // --- integral term ---
    mIntegral = mKi * mIntegralAccumulator;
    // --- proportional term ---
    mProportional = mKp * pressureDiff;
    // --- derivative term ---
    mDerivative = mKd * dp / dt;
    // calculate total output
    // note: output positive = pressure too low -> valve has to close -> reduce pos
    mOutput = mOffset + mProportional + mIntegral + mDerivative;

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
#define VALVE_PERCENT_TOO_FAST 60      // slow down when valve above that position and pressure too high
#define CHANGE_SPEED_WAIT_TIMEOUT 2500 // ms threshold speed change conditions have to be met before change is made
#define PRESSURE_TOLERANCE 1 //bar
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
    if (pressureDiff > -PRESSURE_TOLERANCE && pValve->getPercent() < VALVE_PERCENT_TOO_SLOW && currentSpeedLevel < MAX_SPEED_LEVEL)
    {
        integralSpeedChangePlanned += 1 * dt;
        ESP_LOGD("regulateMotor", "motor seems too slow, incrementing time by %ld to %d", dt, integralSpeedChangePlanned);
    }
    // pressure too high but valve already wide open => slow down?
    else if (pressureDiff < PRESSURE_TOLERANCE && pValve->getPercent() > VALVE_PERCENT_TOO_FAST && currentSpeedLevel > MIN_SPEED_LEVEL)
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