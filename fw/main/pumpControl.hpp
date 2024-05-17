#pragma once
#include "servo.hpp"
#include "vfd.hpp"

// class for regulating the pressure by controlling the bypass valve
class ControlledValve
{
public:
    ControlledValve(ServoMotor *pValve, float defaultTargetPressure);
    void compute(float pressureNow); // calculate and move valve to new position
    void reset(); // reset integral
    // set config options:
    void setKp(double Kp);
    void setKi(double Ki);
    void setKd(double Kd);
    void setOffset(double offset);
    void setTargetPressure(float target){mTargetPressure = target;};
    void setAcceptableDiff(float diff){mAcceptableDiff = diff;};
    // get config / status info:
    void getCurrentStats(uint32_t *timeLastUpdate, float *pressureDiff, float *targetPressure, float *p, float *i, float *d, float *valvePos) const;
    float getPressureDiff() const {return mPressureDiffLast;};
    float getTargetPressure() const {return mTargetPressure;};
    void getCurrentSettings(double *kp, double *ki, double *kd, double *offset) const;


private:
    // config
    double mKp, mKi, mKd, mOffset;
    float mAcceptableDiff;
    ServoMotor *mpValve;
    // variables
    float mProportional, mIntegral, mDerivative, mOutput;
    float mTargetValvePos = 0;
    double mIntegralAccumulator = 0;
    uint32_t mTimestampLastRun = 0;
    float mPressureDiffLast = 0;
    float mTargetPressure = 0;
};

// function that regulates the motor speed depending on pressure and valve pos
void regulateMotor(float pressureDiff, ServoMotor *pValve, Vfd4DigitalPins *pMotor);
