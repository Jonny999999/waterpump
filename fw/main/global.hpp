#pragma once

#include "servo.hpp"
#include "pumpControl.hpp"
#include "pressureSensor.hpp"
#include "mode.hpp"

//TODO drop global variables again (optimize mqtt task)

// create global control object (handles Button/Poti input and system mode)
extern SystemModeController control;

// create global pressure sensor object
extern AnalogPressureSensor pressureSensor;

// global objects defined in main.c
extern ServoMotor servo;

// create controlled valve object that regulates the valve position
extern ControlledValve valveControl;