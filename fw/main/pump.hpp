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
#define OFFSET 10       // idle valve position (expected working point)
#define MAX_INTEGRAL 60 // maximum valve percentage integral term can apply (prevent windup)
#define MIN_VALVE_MOVE_ANGLE 2
void regulateValve(float pressureDiff, ServoMotor *pValve)
{
    // variables
    float proportional, integral, output;
    uint32_t dt, timeNow; //in milliseconds
    static float integralStorage = 0, prevDiff;
    static uint32_t timestampLastRun = 0;

    // calculate time passed since last run
    timeNow = getMs();
    dt = timeNow - timestampLastRun;
    timestampLastRun = timeNow;
    // FIXME: dt will be huge at first run after entering this mode, thus integral will be max

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
    output = OFFSET + proportional + integral;

    // define new valve position
    float oldPos = pValve->getPercent();
    float newPos = oldPos + output;

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
    // only if threshold exceeded to prevent potentional osciallation and unnecessary hardware wear
    if (fabs(oldPos - newPos) > MIN_VALVE_MOVE_ANGLE)
    {
        pValve->setPercentage(newPos);
        ESP_LOGI("regulateValve", "moving valve by %.2f%% degrees to %.2f%%", oldPos - newPos, newPos);
    }

    // store pressure of last run
    prevDiff = pressureDiff;
    // debug log
    ESP_LOGD("regulateValve", "diff=%.2fbar, dt=%dms, P=%.2f%%, I=%.2f%%, valvePrev=%.2f%%, valveTarget=%.2f%% ",
             pressureDiff, (int)dt, proportional, integral, oldPos, newPos);
}




//=======================
//==== regulateMotor ====
//=======================
// TODO: simplify this e.g. by integrating the time?
#define MAX_SPEED_LEVEL 3
#define MIN_SPEED_LEVEL 1
#define VALVE_PERCENT_TOO_SLOW 5    // speed up when valve below that position and pressure too low
#define VALVE_PERCENT_TOO_FAST 80   // slow down when valve above that position and pressure too high
#define CHANGE_SPEED_WAIT_TIMEOUT 3000 //time waited after speed change starts to seem appropriate
enum motorSpeedState {MOTOR_NORMAL = 0, MOTOR_TOO_FAST, MOTOR_TOO_SLOW};
void regulateMotor(float pressureDiff, ServoMotor *pValve, Vfd4DigitalPins *pMotor)
{
    static enum motorSpeedState state = MOTOR_NORMAL; // store state to track timeout
    static uint32_t timestampStateChanged = 0;
    int newSpeedLevel = pMotor->getSpeedLevel(); // get current speed level

    // statemachine to achieve certain wait time between speed changes to ensure its not just a spike
    switch (state)
    {
    case MOTOR_NORMAL: // check if speed change could be necessary
        // pressure too low but valve already almost completely closed => need to speed up
        if (pressureDiff < 0 && pValve->getPercent() < VALVE_PERCENT_TOO_SLOW && newSpeedLevel < MAX_SPEED_LEVEL)
        {
            state = MOTOR_TOO_SLOW;
            timestampStateChanged = getMs();
        }
        // pressure too high but valve already wide open => need to slow down
        else if (pressureDiff > 0 && pValve->getPercent() > VALVE_PERCENT_TOO_FAST && newSpeedLevel > MIN_SPEED_LEVEL) // TODO adjust valve threshold
        {
            state = MOTOR_TOO_FAST;
            timestampStateChanged = getMs();
        }
        break;
    case MOTOR_TOO_SLOW: // wait for speed up
        if (pValve->getPercent() > VALVE_PERCENT_TOO_SLOW)
        { // valve opened again (no need to speed up anymore)
            state = MOTOR_NORMAL;
        }
        else if ((getMs() - timestampStateChanged > CHANGE_SPEED_WAIT_TIMEOUT)) // wait time passed -> speeding up
        {
            pMotor->setSpeedLevel(++newSpeedLevel);
            ESP_LOGW("regulateMotor", "valve below %d%% for too long -> increasing motor speed to %d", VALVE_PERCENT_TOO_SLOW, newSpeedLevel); // todo also log variables
        }
        break;
    case MOTOR_TOO_FAST: // wait for slow down
        if (pValve->getPercent() < VALVE_PERCENT_TOO_FAST)
        { // valve closed again (no need to slow down anymore)
            state = MOTOR_NORMAL;
        }
        else if ((getMs() - timestampStateChanged > CHANGE_SPEED_WAIT_TIMEOUT)) // wait time passed -> slowing down
        {
            pMotor->setSpeedLevel(--newSpeedLevel);
            ESP_LOGW("regulateMotor", "valve above %d%% for too long -> decreasing motor speed to %d", VALVE_PERCENT_TOO_FAST, newSpeedLevel); // todo also log variables
        }
        break;
    }
}