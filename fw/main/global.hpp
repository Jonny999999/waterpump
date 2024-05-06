#pragma once

#include "servo.hpp"
#include "pumpControl.hpp"
#include "pressureSensor.hpp"
#include "mode.hpp"
#include "display.hpp"
#include "flowSensor.hpp"
#include "vfd.hpp"

//TODO drop global variables again (optimize mqtt task)

// create global control object (handles Button/Poti input and system mode)
extern SystemModeController control;

// create global motor object
extern Vfd4DigitalPins motor;

// create global pressure sensor object
extern AnalogPressureSensor pressureSensor;

// create global flow sensor object
extern FlowSensor flowSensor;

// global objects defined in main.c
extern ServoMotor servo;

// create controlled valve object that regulates the valve position
extern ControlledValve valveControl;

// create global display objects
extern max7219_t three7SegDisplays;
extern handledDisplay displayTop;
extern handledDisplay displayMid;
extern handledDisplay displayBot;