#pragma once
#include "servo.hpp"
#include "vfd.hpp"

// class for regulating the pressure by controlling the bypass valve
class ControlledValve
{
public:
    ControlledValve(ServoMotor *pValve);
    void compute(float pressureDiff); // calculate and move valve to new position
    void setKp(double Kp);
    void setKi(double Ki);
    void setKd(double Kd);
    void reset(); // reset integral
    void getCurrentStats(uint32_t *timeLastUpdate, float *p, float *i, float *d, float *valvePos) const;
    void getCurrentSettings(double *kp, double *ki, double *kd) const;


private:
    // config
    double mKp, mKi, mKd;
    ServoMotor *mpValve;
    // variables
    float mProportional, mIntegral, mDerivative, mOutput;
    float mTargetValvePos = 0;
    double mIntegralAccumulator = 0;
    uint32_t mTimestampLastRun = 0;
};

// function that regulates the motor speed depending on pressure and valve pos
void regulateMotor(float pressureDiff, ServoMotor *pValve, Vfd4DigitalPins *pMotor);
