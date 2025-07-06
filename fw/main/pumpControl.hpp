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
    void getCurrentStats(uint32_t *timeLastUpdate, float *pressureNowSmoothed, float *pressureDiff, float *targetPressure, float *p, float *i, float *d, float *valvePos) const;
    float getPressureDiff() const {return mPressureDiffLast;};
    float getTargetPressure() const {return mTargetPressure;};
    void getCurrentSettings(double *kp, double *ki, double *kd, double *offset, float *acceptableDiff) const;


private:
    // config
    double mKp, mKi, mKd, mOffset;
    float mAcceptableDiff;
    ServoMotor *mpValve;
    // variables
    float mProportional, mIntegral, mDerivative, mOutput_percentClosed;
    float mFilteredPressure = 0.0;
    float mTargetValvePercentOpen = 0;
    double mIntegralAccumulator = 0;
    uint32_t mTimestampLastRun = 0;
    float mPressureDiffLast = 0;
    float mTargetPressure = 0;

    // === Valve fault detection recovery ===
    void checkValveResponseAndRecoverIfStuck(float pressureNow);
    static constexpr uint32_t VALVE_RECOVERY_CHECK_TIMEOUT_MS = 750;     // wait before detecting failure
    static constexpr uint32_t VALVE_RECOVERY_RETRY_INTERVAL_MS = 3000;    // retry interval if still stuck
    static constexpr float VALVE_RECOVERY_PRESSURE_THRESHOLD = 2.0f;      // how much too high pressure must be
    // internal tracking variables
    uint32_t mTimestampLastValveFullyOpenCommand = 0;
    uint32_t mTimestampLastValveRecoveryAttempt = 0;

};

// function that regulates the motor speed depending on pressure and valve pos
void regulateMotor(float pressureDiff, ServoMotor *pValve, Vfd4DigitalPins *pMotor);
