extern "C"
{
#include "driver/gpio.h"
#include "driver/adc.h"
}
#include "global.hpp"


//================
//==== config ====
//================
#define DEFAULT_TARGET_PRESSURE 4
#define ADC_POTI ADC1_CHANNEL_6 //gpio34

const float lookupPSensor[][2] = {
    //FIXME populate this table with actual measurements
    //ADC, bar
    {10, 1},
    {1234, 4.1},
    {1234, 4.1}
    //{, },
};


//======================
//=== global objects ===
//======================
// create global control object (handles Button/Poti input and system mode)
controlConfig_t controlConfig{
    .defaultMode = IDLE,
    .gpioSetButton = GPIO_NUM_33,
    .gpioModeButton = GPIO_NUM_25,
    .gpioStatusLed = GPIO_NUM_10,
    .adcChannelPoti = ADC_POTI};
SystemModeController control(controlConfig);

// create global pressure sensor on gpio 36
//AnalogPressureSensor pressureSensor(ADC1_CHANNEL_0, 0.1, 2.5, 0, 30);
AnalogPressureSensor pressureSensor(ADC1_CHANNEL_0, lookupPSensor, sizeof(lookupPSensor)/sizeof(lookupPSensor[0]));

// create global servo object
servoConfig_t servoConfig{
    .gpioPwmSignal = 27,
    .gpioEnablePower = 13,       // onboard realy
    .powerEnableRequired = true, // require enable() call to turn above pin on
    .ratedAngle = 180,
    // Coupling V1: 17 to 87 (no play)
    // Coupling V2: 11 to 89 deg
    .minAllowedAngle = 17, // valve completely closed
    .maxAllowedAngle = 87, // valve completely open
    .invertDirection = true};
ServoMotor servo(servoConfig);

// create global controlled valve object that regulates the valve position
ControlledValve valveControl(&servo, DEFAULT_TARGET_PRESSURE);
