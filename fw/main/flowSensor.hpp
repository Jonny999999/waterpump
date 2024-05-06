#pragma once
extern "C"
{
#include "driver/gpio.h"
}

class FlowSensor
{
public:
    FlowSensor(gpio_num_t gpioPulse, float pulsesPerLiter);
    void init();  // initialize gpio and isr - has to be run manual after running `gpio_install_isr_service(0)` at startuo
    void read();  // update and calculate stored volume and flow rate using pulses during time since last run
    void reset(); // reset count / volume to zero

    float getVolume_liter() const { return mVolume_liter; };
    double getFlowRate_literPerSecond() const { return mFlow_literPerSecond; };
    uint32_t getPulseCount() const { return mPulseCount; };

    void countPulse(); // increment pulse count - only used by ISR

private:
    // storage
    double mFlow_literPerSecond = 0;
    float mVolume_liter = 0;
    volatile uint32_t mPulseCount = 0;
    volatile uint64_t mTimestampLastPulseUs = 0; 
    // config
    float mConfigPulsesPerLiter = 1;
    gpio_num_t mGpioPulse;
    // variables
    uint32_t mTimestampLastRead = 0;
    uint32_t mPulseCountLastRead = 0;
    bool mIsInitialized = false;
};