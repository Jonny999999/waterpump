#include "servo.hpp"
#include "vfd.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include <math.h>


#define getMs() (esp_timer_get_time()/1000)

//=======================
//==== regulateValve ====
//=======================
// TODO adjust parameters
// TODO smaller steps
#define Kp 10           // proportional gain
#define Ki 0.0001       // integral gain
#define OFFSET (100-20) // 0 fully open, 100 fully closed - idle valve position (expected working point)
#define MAX_INTEGRAL 60 // maximum valve percentage integral term can apply (prevent windup)
#define MIN_VALVE_MOVE_ANGLE 2
#define MAX_DT_MS 5000 // prevent bugged action with large time delta at first after several seconds
void regulateValve(float pressureDiff, ServoMotor *pValve)
{
    // variables
    float proportional, integral, output;
    uint32_t dt, timeNow; //in milliseconds
    static float integralStorage = 0;
    static uint32_t timestampLastRun = 0;

    // calculate time passed since last run
    timeNow = getMs();
    dt = timeNow - timestampLastRun;
    timestampLastRun = timeNow;
    // ignore run with very large dt (probably first run of function)
    if (dt > MAX_DT_MS){
        dt = 0; 
        ESP_LOGE("regulateValve", "dt too large (%ldms), ignoring this run", dt);
        return;
    }

    // integrate
    integralStorage += pressureDiff * dt;
    // limit to max (prevents windup)
    if (integralStorage*Ki > MAX_INTEGRAL)
    {
        integralStorage = MAX_INTEGRAL/Ki;
    }
    else if (integralStorage*Ki < -MAX_INTEGRAL)
    {
        integralStorage = -MAX_INTEGRAL/Ki;
    }

    // --- integral term ---
    integral = Ki * integralStorage;
    // --- proportional term ---
    proportional = Kp * pressureDiff;
    // calculate total output
    // note: output positive = pressure too low -> valve has to close -> reduce pos
    output = OFFSET + proportional + integral;

    // define new valve position
    float oldPos = pValve->getPercent();
    // invert valve pos (output: percentage valve is CLOSED, servo: percentage valve is OPEN)
    // TODO invert servo direction
    float newPos = 100 - output;

    // Ensure newPos is within the valid range [0, 100]
    if (newPos < 0)
    {
        newPos = 0;
    }
    else if (newPos > 100)
    {
        newPos = 100;
    }

    // move valve to new pos
    // only move if threshold exceeded to reduce osciallation and unnecessary hardware wear
    if (fabs(oldPos - newPos) > MIN_VALVE_MOVE_ANGLE)
    {
        pValve->setPercentage(newPos);
        ESP_LOGI("regulateValve", "moving valve by %.2f%% degrees to %.2f%%", oldPos - newPos, newPos);
    }

    // debug log
    ESP_LOGD("regulateValve", "diff=%.2fbar, dt=%dms, P=%.2f%%, I=%.2f%%, valvePrev=%.2f%%, valveTarget=%.2f%% ",
             pressureDiff, (int)dt, proportional, integral, oldPos, newPos);
}




//=======================
//==== regulateMotor ====
//=======================
#define MAX_SPEED_LEVEL 3
#define MIN_SPEED_LEVEL 1
// TODO: adjust thresholds:
#define VALVE_PERCENT_TOO_SLOW 5    // speed up when valve below that position and pressure too low
#define VALVE_PERCENT_TOO_FAST 80   // slow down when valve above that position and pressure too high
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
        integralSpeedChangePlanned += 1*dt;
        ESP_LOGD("regulateMotor", "motor seems too slow, incrementing time by %ld to %d", dt, integralSpeedChangePlanned);
    }
    // pressure too high but valve already wide open => slow down?
    else if (pressureDiff < 0.5 && pValve->getPercent() > 80 && currentSpeedLevel > MIN_SPEED_LEVEL)
    {
        integralSpeedChangePlanned -= 1*dt;
        ESP_LOGD("regulateMotor", "motor seems too fast, decrementing time by %ld to %d", dt, integralSpeedChangePlanned);
    } else {
        // reset tracked time when in valid working range
        integralSpeedChangePlanned = 0;
    }

    if (integralSpeedChangePlanned > CHANGE_SPEED_WAIT_TIMEOUT){
            pMotor->setSpeedLevel(++currentSpeedLevel);
            ESP_LOGW("regulateMotor", "valve below %d%% for too long -> increasing motor speed to %d", VALVE_PERCENT_TOO_SLOW, currentSpeedLevel); // todo also log variables
            integralSpeedChangePlanned = 0;

    } else if (integralSpeedChangePlanned < -CHANGE_SPEED_WAIT_TIMEOUT){
            pMotor->setSpeedLevel(--currentSpeedLevel);
            ESP_LOGW("regulateMotor", "valve above %d%% for too long -> decreasing motor speed to %d", VALVE_PERCENT_TOO_FAST, currentSpeedLevel); // todo also log variables
            integralSpeedChangePlanned = 0;
    }
}