#pragma once
#include <vector>

class AnalogPressureSensor
{
public:
    AnalogPressureSensor(int adcChannel, float minVoltage, float maxVoltage, float minPressure, float maxPressure, uint8_t rollingAverageLength);
    // initialize using lookuptable (pass 2d array {ADC, Bar} and count of entries) instead of conversion parameters
    AnalogPressureSensor(int adcChannel, float const lookupTableAdcBar[][2], int lookupCount, uint8_t rollingAverageLength);
    void read(); // read and store current value
    float getBar(); // get currently stored bar
    float readBar(); // read and return current bar

private:
    //config
    const int mAdcChannel;
    const float mMinPressure;
    const float mMaxPressure;
    const float mMinVoltage;
    const float mMaxVoltage;
    const uint8_t mRollingAverageLength = 1;
    //methods
    void init();
    float adcToBar(int adcValue);
    void printRollingAverageBuffer();
    // variables
    float mCurrentPressureBar = 0;
    int mCurrentAdcValue;
    const float (*mLookupTableAdcBar)[2]; // ptr to 2d lookup table
    int mLookupCount = 0;
    bool mUsingLookupTable = false;
    std::vector<int> mAdcReadings;
    };