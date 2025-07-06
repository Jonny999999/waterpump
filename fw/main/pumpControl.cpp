extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
}
#include <math.h>

#include "servo.hpp"
#include "vfd.hpp"
#include "pumpControl.hpp"
#include "common.hpp"
#include "log_utils.hpp"

#define getMs() (esp_timer_get_time() / 1000)

// TODO adjust parameters
// TODO smaller steps
// significantly reduced values while testing 2024.05.13
// slightly increased again 2025.06.21 P:4->5 I: 0.01->0.02
#define Kp 4     // proportional gain
#define Ki 0.006  // integral gain
#define Kd 0     // derivative gain
#define ACCEPTABLE_DIFF 0.15 // skip compute when difference is smaller than this (less oscillation / valve wear)
//TODO variable offset depending on target pressure?
#define OFFSET (100 - 30) // 0 fully open, 100 fully closed - idle valve position (expected working point)
#define INTEGRAL_LIMIT_OFFSET 0  // increase/decrease max possible integral value (valve percent)
// examples (integral value only):
// 0: valve can reach exactly 0 to 100%
// <0: valve can not reach limits (e.g. -20 -> 20% to 80%)
// >0: valve can overshoot - danger of windup (e.g. 20 -> -20% to 120%)
#define MIN_VALVE_MOVE_PERCENT 2 // only update valve position when change is more than that value (prevent continous very small movement, reduce valve wear)
#define MAX_DT_MS 5000 // prevent bugged action with large time delta at first after several seconds

#define PRESSURE_SMOOTH_ALPHA 0.10 // 0-1  1: almost no smoothing, 0.2: new reading contributes 20% --- note: strongly depends on the rate called
//=================================
//== ControlledValve Constructor ==
//=================================
ControlledValve::ControlledValve(ServoMotor *pValve, float defaultTargetPressure)
{
    // copy pointer and config values
    // TODO get values from nvs instead of default?
    mpValve = pValve;
    mKp = Kp;
    mKi = Ki;
    mKd = Kd;
    mOffset = OFFSET;
    mAcceptableDiff = ACCEPTABLE_DIFF;

    mTargetPressure = defaultTargetPressure;
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
void ControlledValve::getCurrentStats(uint32_t *timestampLastUpdate, float *pressureNowSmoothed, float *pressureDiff, float * targetPressure, float *p, float *i, float *d, float *valvePos) const
{
    // TODO mutex
    *timestampLastUpdate = mTimestampLastRun;
    *pressureDiff = mPressureDiffLast;
    *pressureNowSmoothed = mFilteredPressure;
    *targetPressure = mTargetPressure;
    *p = mProportional;
    *i = mIntegral;
    *d = mDerivative;
    *valvePos = mTargetValvePercentOpen;
}
void ControlledValve::getCurrentSettings(double *kp, double *ki, double *kd, double *offset, float *acceptableDiff) const
{
    *kp = mKp;
    *ki = mKi;
    *kd = mKd;
    *offset = mOffset;
    *acceptableDiff = mAcceptableDiff;
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
void ControlledValve::compute(float pressureNow)
{
    // variables
    uint32_t dt, timeNow; // in milliseconds
    double dp;

    float pressureNowRaw = pressureNow;
    mFilteredPressure = PRESSURE_SMOOTH_ALPHA * pressureNow + (1.0f - PRESSURE_SMOOTH_ALPHA) * mFilteredPressure;
    float pressureNowSmoothed = mFilteredPressure;

    // calculate pressure difference
    //float pressureDiff = mTargetPressure - pressureNow;
    float pressureDiff = mTargetPressure - pressureNowSmoothed;
    // calculate time passed since last run
    timeNow = getMs();
    dt = timeNow - mTimestampLastRun;
    mTimestampLastRun = timeNow;
    // calculate pressure change since last run
    dp = pressureDiff - mPressureDiffLast;
    mPressureDiffLast = pressureDiff;
    // ignore run with very large dt (probably first run of method since startup) to prevent huge integral step
    if (dt > MAX_DT_MS)
    {
        dt = 0;
        ESP_LOGE("regulateValve", "dt too large (%ldms), ignoring this run", dt);
        return;
    }
    // ignore run when target pressure is already reached / in acceptable range
    else if (fabs(pressureDiff) < mAcceptableDiff){
        ESP_LOGI("regulateValve", "pressure diff %.2f in acceptable range %.2f -> ignoring this run", pressureDiff, ACCEPTABLE_DIFF);
        return;
    }

    // integrate
    mIntegralAccumulator += pressureDiff * dt;
    // limit to allowed range (prevents windup)
    // calculate integral limits depending on offset TODO: do this on offset change only
    double maxIntegral = ((100-mOffset) + INTEGRAL_LIMIT_OFFSET) / mKi;
    double minIntegral = -(mOffset + INTEGRAL_LIMIT_OFFSET) / mKi;
    mIntegralAccumulator = limitToRange(mIntegralAccumulator, minIntegral, maxIntegral);

    // --- integral term ---
    mIntegral = mKi * mIntegralAccumulator;
    // --- proportional term ---
    mProportional = mKp * pressureDiff;
    // --- derivative term ---
    mDerivative = mKd * dp / dt;
    // calculate total output
    // note: mOutput_percentClosed positive = pressure too low -> valve has to close -> reduce pos
    mOutput_percentClosed = mOffset + mProportional + mIntegral + mDerivative;

    // define new valve position
    float oldPos = mpValve->getPercent();
    // invert valve pos (mOutput_percentClosed: percentage valve is CLOSED, servo: percentage valve is OPEN)
    // TODO invert servo direction
    mTargetValvePercentOpen = 100 - mOutput_percentClosed;
    // Ensure newPos is within the valid range [0, 100]
    mTargetValvePercentOpen = limitToRange<float>(mTargetValvePercentOpen, 0, 100);

    // move valve to new pos
    // only move if threshold exceeded to reduce osciallation and unnecessary hardware wear
    if (fabs(oldPos - mTargetValvePercentOpen) > MIN_VALVE_MOVE_PERCENT)
    {
        mpValve->setPercentage(mTargetValvePercentOpen);
        ESP_LOGI("regulateValve", "moving valve by %.2f%% degrees to %.2f%%", oldPos - mTargetValvePercentOpen, mTargetValvePercentOpen);
    }

    // detect fault condition where servo is not opening properly (hangup due to EMI?)  
    // -> power cycle the servo
    checkValveResponseAndRecoverIfStuck(pressureNow);

    // debug log
    ESP_LOGD("regulateValve", "diff=%.2fbar, dt=%dms, P=%.2f%%, I=%.2f%%, valvePrev=%.2f%%, valveTarget=%.2f%% ",
             pressureDiff, (int)dt, mProportional, mIntegral, oldPos, mTargetValvePercentOpen);

}




//=============================================================
//==== ControlledValve checkValveResponseAndRecoverIfStuck ====
//=============================================================
// detect fault condition where servo is not opening properly (hangup due to EMI?)  
// -> power cycle the servo
void ControlledValve::checkValveResponseAndRecoverIfStuck(float pressureNow)
{
    // called only if servo is supposed to reduce pressure and target if valve fully open
    if (mTargetValvePercentOpen >= 100)
    {
        // on first detection of "fully open"
        if (mTimestampLastValveFullyOpenCommand == 0)
        {
            mTimestampLastValveFullyOpenCommand = getMs();
        }

        uint32_t timeSinceOpen = getMs() - mTimestampLastValveFullyOpenCommand;
        float pressureStillHigh = pressureNow - mTargetPressure;

        // detect stuck condition
        if (timeSinceOpen > VALVE_RECOVERY_CHECK_TIMEOUT_MS &&
            pressureStillHigh > VALVE_RECOVERY_PRESSURE_THRESHOLD)
        {
            // prevent excessive resets
            if (getMs() - mTimestampLastValveRecoveryAttempt > VALVE_RECOVERY_RETRY_INTERVAL_MS)
            {
                LOGE_TERM_AND_MQTT("regulateValve", "Servo stuck? Valve fully open for %ldms but pressure still high (%.2f > %.2f) -> resetting servo power.",
                         timeSinceOpen, pressureNow, mTargetPressure);

                mTimestampLastValveRecoveryAttempt = getMs();
                mpValve->disable();
                vTaskDelay(pdMS_TO_TICKS(300)); // short power cut
                mpValve->enable();
                mpValve->setPercentage(mTargetValvePercentOpen); // resend current position
            }
        }
    }
    else
    {
        // reset tracking if valve is not fully open anymore
        mTimestampLastValveFullyOpenCommand = 0;
    }
}








//=======================
//==== regulateMotor ====
//=======================
#define MAX_SPEED_LEVEL 3
#define MIN_SPEED_LEVEL 1
// TODO: adjust thresholds:
#define VALVE_PERCENT_TOO_SLOW 3       // speed up when valve below that position and pressure too low
#define VALVE_PERCENT_TOO_FAST 55      // slow down when valve above that position and pressure too high
#define CHANGE_SPEED_WAIT_TIMEOUT 4000 // ms threshold speed change conditions have to be met before change is made
#define PRESSURE_TOLERANCE 2 //bar
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
    // pressure too low but valve already almost completely closed => speed up?
    if (pressureDiff > -PRESSURE_TOLERANCE && pValve->getPercent() < VALVE_PERCENT_TOO_SLOW && currentSpeedLevel < MAX_SPEED_LEVEL)
    {
        integralSpeedChangePlanned += 1 * dt; // TODO: use actual integral? (e.g. multiply with valve difference above threshold)
        ESP_LOGD("regulateMotor", "motor seems too slow, incrementing time by %ld to %d", dt, integralSpeedChangePlanned);
    }
    // pressure too high but valve already wide open => slow down?
    //else if (pressureDiff < PRESSURE_TOLERANCE && pValve->getPercent() > VALVE_PERCENT_TOO_FAST && currentSpeedLevel > MIN_SPEED_LEVEL)
    else if (pValve->getPercent() > VALVE_PERCENT_TOO_FAST && currentSpeedLevel > MIN_SPEED_LEVEL)
    {
        integralSpeedChangePlanned -= 1 * dt;
        ESP_LOGD("regulateMotor", "motor seems too fast, decrementing time by %ld to %d", dt, integralSpeedChangePlanned);
    }
    else
    {
        // reset tracked time when in valid working range
        integralSpeedChangePlanned = 0;
    }

    // speed change is due
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