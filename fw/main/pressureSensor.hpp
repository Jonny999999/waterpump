#pragma once

class AnalogPressureSensor
{
public:
    AnalogPressureSensor(int adcChannel, float minVoltage, float maxVoltage, float minPressure, float maxPressure);
    // initialize using lookuptable (pass 2d array {ADC, Bar} and count of entries) instead of conversion parameters
    AnalogPressureSensor(int adcChannel, float const lookupTableAdcBar[][2], int lookupCount);
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
    //methods
    void init();
    float adcToBar(int adcValue);
    //variables
    float mCurrentPressureBar = 0;
    int mCurrentAdcValue;
    const float (*mLookupTableAdcBar)[2]; //ptr to 2d lookup table
    int mLookupCount = 0;
    bool mUsingLookupTable = false;
};