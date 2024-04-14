#include "servo.hpp"
#include "vfd.hpp"
#include "esp_log.h"
#include <math.h>



//=======================
//==== regulateValve ====
//=======================
// TODO adjust parameters
// TODO smaller steps
#define Kp 2            // proportional gain
#define Ki 0.5          // integral gain
#define MAX_INTEGRAL 40 // maximum integral term to prevent windup
#define MIN_MOVE_ANGLE 2
void regulateValve(float pressureDiff, ServoMotor *pValve)
{
    float proportional, output;
    static float integral, prevDiff;

    // --- proportional term ---
    proportional = Kp * pressureDiff;

    // --- integral term ---
    integral += Ki * pressureDiff;
    // limit to max (prevents windup)
    if (integral > MAX_INTEGRAL)
    {
        integral = MAX_INTEGRAL;
    }
    else if (integral < -MAX_INTEGRAL)
    {
        integral = -MAX_INTEGRAL;
    }

    // calculate total output
    output = proportional + integral;

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
    if (fabs(oldPos - newPos) > MIN_MOVE_ANGLE)
    {
        pValve->setPercentage(newPos);
        ESP_LOGI("regulateValve", "moving valve by %.2f%% degrees to %.2f%%", oldPos - newPos, newPos);
    }

    // store pressure of last run
    prevDiff = pressureDiff;
    // debug log
    ESP_LOGD("regulateValve", "diff=%.2fbar, prevDiff=%.2fbar, P=%.2f%%, I=%.2f%%, valvePrev=%.2f%%, valveTarget=%.2f%% ",
             pressureDiff, prevDiff, proportional, integral, oldPos, newPos);
}

//=======================
//==== regulateMotor ====
//=======================
#define MAX_SPEED_LEVEL 3
#define MIN_SPEED_LEVEL 1
//TODO add min duration valve is above threshold
void regulateMotor(float pressureDiff, ServoMotor *pValve, Vfd4DigitalPins *pMotor)
{
    int newSpeedLevel = pMotor->getSpeedLevel();

    // pressure too low but valve already almost compeltely closed => speed up
    if (pressureDiff < 0 && pValve->getPercent() < 5 && newSpeedLevel < MAX_SPEED_LEVEL)
    {
        pMotor->setSpeedLevel(++newSpeedLevel);
        ESP_LOGW("regulateMotor", "increasing motor speed to %d", newSpeedLevel); // todo also log variables
    }
    // pressure too high but valve already wide open => slow down
    else if (pressureDiff > 0 && pValve->getPercent() > 80 && newSpeedLevel > MIN_SPEED_LEVEL) // TODO adjust valve threshold
    {
        pMotor->setSpeedLevel(--newSpeedLevel);
        ESP_LOGW("regulateMotor", "decreasing motor speed to %d", newSpeedLevel); // todo also log variables
    }
}