extern "C"
{
#include "driver/gpio.h"
#include "driver/adc.h"
}

#include "global.hpp"

// create global pressure sensor on gpio 36
//FIXME: calibrate ADC and pressure sensor!
AnalogPressureSensor pressureSensor(ADC1_CHANNEL_0, 0.1, 2.5, 0, 30);

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
ControlledValve valveControl(&servo);
