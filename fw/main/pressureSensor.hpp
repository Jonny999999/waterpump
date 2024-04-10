
class AnalogPressureSensor
{
public:
    AnalogPressureSensor(int adcChannel, float minVoltage, float maxVoltage, float minPressure, float maxPressure);
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
};