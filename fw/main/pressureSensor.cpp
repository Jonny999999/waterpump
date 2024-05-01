
extern "C" {
#include <driver/adc.h>
#include "esp_log.h"
}
#include "pressureSensor.hpp"
#include "common.hpp"

// tag for logging
static const char *TAG = "pressure";



// constructor (parameters for conversion)
AnalogPressureSensor::AnalogPressureSensor(int adcChannel, float minVoltage, float maxVoltage, float minPressure, float maxPressure)
    : mAdcChannel(adcChannel), mMinPressure(minPressure), mMaxPressure(maxPressure), mMinVoltage(minVoltage), mMaxVoltage(maxVoltage)
{
    // initialize ADC
    init();
}

// constructor (lookup table for conversion)
AnalogPressureSensor::AnalogPressureSensor(int adcChannel, float const lookupTableAdcBar[][2], int lookupCount)
    : mAdcChannel(adcChannel), mMinPressure(0), mMaxPressure(0), mMinVoltage(0), mMaxVoltage(0)
    {
    // copy pointer to lookup table
    mLookupTableAdcBar = lookupTableAdcBar;
    mLookupCount = lookupCount;
    mUsingLookupTable = true; // disable conversion using now undefined parameters
    // initialize ADC
    init();
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
        pressure = scaleUsingLookupTable(mLookupTableAdcBar, mLookupCount, adcValue);
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
#define ADC_READ_SAMPLE_COUNT 200
void AnalogPressureSensor::read()
{
    // Read adc (multisampling)
    uint32_t sum = 0;
    for (int i=0; i<ADC_READ_SAMPLE_COUNT; i++)
    {
        sum += adc1_get_raw(static_cast<adc1_channel_t>(mAdcChannel));
    }
    mCurrentAdcValue = sum/ADC_READ_SAMPLE_COUNT;

    // Convert and store pressure
    mCurrentPressureBar = adcToBar(mCurrentAdcValue);
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
