extern "C"
{
#include "driver/gpio.h"
#include "driver/adc.h"
}
#include "global.hpp"
#include "config.h"


//================
//==== config ====
//================
#define DEFAULT_TARGET_PRESSURE 4
#define ADC_POTI ADC1_CHANNEL_6 //gpio34

//lookup table for 5V 30bar pressure sensor (measured 2024.05.01)
const float lookupPSensor[][2] = {
    // ADC, bar
    {139, 0},
    {188, 0.5},
    {208, 1},
    {220, 1.25},
    {240, 1.5}, // very linear after this point
    {260, 1.75},
    {279, 2},
    {295, 2.25},
    {310, 2.5},
    {330, 2.75},
    {349, 3},
    {383, 3.5},
    {420, 4},
    {451, 4.5},
    {483, 5},
    {525, 5.5},
    {559, 6},
    {590, 6.5},
    {624, 7},
    {656, 7.5},
    {693, 8},
    {723, 8.5},

    // x1=349  y1=3
    // x2=723 y2=8.5
    // p = (5.5/374)*adc - 2.1323529

    {4095, 58} //extrapolated to max adc value
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

// create global flow sensor object
FlowSensor flowSensor(GPIO_NUM_26, 12);

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

// create global display objects, one for each segment
// note: reference to hardware device is passed later in main() after 5V is enabled using .init()
handledDisplay displayTop(0);
handledDisplay displayMid(8);
handledDisplay displayBot(16);