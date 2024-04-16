
#include <driver/adc.h>
#include "esp_log.h"
#include "pressureSensor.hpp"

// tag for logging
static const char *TAG = "pressure";



// constructor
AnalogPressureSensor::AnalogPressureSensor(int adcChannel, float minVoltage, float maxVoltage, float minPressure, float maxPressure)
    : mAdcChannel(adcChannel), mMinPressure(minPressure), mMaxPressure(maxPressure), mMinVoltage(minVoltage), mMaxVoltage(maxVoltage)
{
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
    // Convert ADC value to voltage 0 to 3.3V
    float voltage = static_cast<float>(adcValue) / 4095.0 * 3.3;

    // Convert voltage range (e.g. 0.25V to 2.25V) to pressure range (e.g. 0 to 30 bar)
    float pressure = (voltage - 0.25) * (30.0 / (2.25 - 0.25));

    ESP_LOGI(TAG, "adc=%d, voltage=%f, pressure=%f", adcValue, voltage, pressure);
    return pressure;
}



// read and store current value
void AnalogPressureSensor::read()
{
    // Read adc
    mCurrentAdcValue = adc1_get_raw(static_cast<adc1_channel_t>(mAdcChannel));
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
