
extern "C" {
#include <driver/adc.h>
#include "esp_log.h"
}
#include "pressureSensor.hpp"
#include "common.hpp"

// tag for logging
static const char *TAG = "pressure";



// constructor (parameters for conversion)
AnalogPressureSensor::AnalogPressureSensor(int adcChannel, float minVoltage, float maxVoltage, float minPressure, float maxPressure, uint8_t rollingAverageLength)
    : mAdcChannel(adcChannel), mMinPressure(minPressure), mMaxPressure(maxPressure), mMinVoltage(minVoltage), mMaxVoltage(maxVoltage), mRollingAverageLength(rollingAverageLength)
{
    // create array for rolling average, 0 initialized
    mAdcReadings = new uint32_t[mRollingAverageLength]();
    // initialize ADC
    init();
}

// constructor (lookup table for conversion)
AnalogPressureSensor::AnalogPressureSensor(int adcChannel, float const lookupTableAdcBar[][2], int lookupCount, uint8_t rollingAverageLength)
    : mAdcChannel(adcChannel), mMinPressure(0), mMaxPressure(0), mMinVoltage(0), mMaxVoltage(0), mRollingAverageLength(rollingAverageLength)
    {
    // copy pointer to lookup table
    mLookupTableAdcBar = lookupTableAdcBar;
    mLookupCount = lookupCount;
    mUsingLookupTable = true; // disable conversion using now undefined parameters
    // create array for rolling average, 0 initialized
    mAdcReadings = new uint32_t[mRollingAverageLength]();
    // initialize ADC
    init();
}


// deconstructor
AnalogPressureSensor::~AnalogPressureSensor(){
    // delete array created for rolling average
    delete[] mAdcReadings;
}


// initialize ADC
void AnalogPressureSensor::init()
{
    adc1_config_width(ADC_WIDTH_BIT_12);                                                  // Max resolution 4096
    adc1_config_channel_atten(static_cast<adc1_channel_t>(mAdcChannel), ADC_ATTEN_DB_11); // Max voltage
}



// convert ADC value to pressure in bars
float AnalogPressureSensor::adcToBar(int adcValue)
{
    float pressure;
    if (mUsingLookupTable)
    {
        pressure = scaleUsingLookupTable<float>(mLookupTableAdcBar, mLookupCount, adcValue);
        ESP_LOGI(TAG, "converted using lookupTable: adc=%d --> pressure=%f", adcValue, pressure);
    }
    else
    {
        // Convert ADC value to voltage 0 to 3.3V
        float voltage = static_cast<float>(adcValue) / 4095.0 * 3.3;

        // Convert voltage range (e.g. 0.25V to 2.25V) to pressure range (e.g. 0 to 30 bar)
        pressure = (voltage - mMinVoltage) * (mMaxPressure / (mMaxVoltage - mMinVoltage));
        ESP_LOGI(TAG, "converted using params: adc=%d, voltage=%f, pressure=%f", adcValue, voltage, pressure);
    }

    return pressure;
}



// read and store current value
#define ADC_READ_SAMPLE_COUNT 199
void AnalogPressureSensor::read()
{
    // Read adc (multisampling)
    uint32_t sum = 0;
    for (int i=0; i<ADC_READ_SAMPLE_COUNT; i++)
    {
        sum += adc1_get_raw(static_cast<adc1_channel_t>(mAdcChannel));
    }
    mCurrentAdcValue = sum/ADC_READ_SAMPLE_COUNT;

    // add new reading to the rolling average buffer
    mAdcReadings[mBufferIndex] = mCurrentAdcValue;
    mBufferIndex = (mBufferIndex + 1) % mRollingAverageLength; // set next index to be changed

    // logging
    ESP_LOGD(TAG, "Adding adc value %d to buffer", mCurrentAdcValue);
    if (esp_log_level_get(TAG) == ESP_LOG_DEBUG) {
        printRollingAverageBuffer();
    }

    // calculate rolling average
    sum = 0;
    for (int i = 0; i < mRollingAverageLength; i++) {
        sum += mAdcReadings[i];
    }
    float averageAdc = (float)sum / mRollingAverageLength;

    // Convert and store pressure
    mCurrentPressureBar = adcToBar(averageAdc);
}



// get currently stored bar
float AnalogPressureSensor::getBar()
{
    return mCurrentPressureBar;
}



// read and return current bar
float AnalogPressureSensor::readBar()
{
    read();
    return getBar();
}



// method for printing the rolling average buffer
void AnalogPressureSensor::printRollingAverageBuffer() {
    char buffer[8*mRollingAverageLength + 1];
    int offset = snprintf(buffer, sizeof(buffer), "Buffer: ");
    for (int i = 0; i < mRollingAverageLength; i++) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%.3f ", adcToBar(mAdcReadings[i]));
    }
    ESP_LOGD(TAG, "Rolling Average Buffer: %s", buffer);
}