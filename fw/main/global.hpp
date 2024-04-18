#pragma once

#include "servo.hpp"
#include "pumpControl.hpp"

//TODO drop global variables again (optimize mqtt task)

// global objects defined in main.c
extern ServoMotor servo;

// create controlled valve object that regulates the valve position
extern ControlledValve valveControl;