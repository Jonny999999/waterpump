
#include "global.hpp"

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
