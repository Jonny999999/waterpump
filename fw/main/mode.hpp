#pragma once
extern "C" {
#include "driver/gpio.h"
#include "esp_log.h"
}
#include "vfd.hpp"

typedef enum controlMode_t {IDLE, SPEED, PRESSURE_SPEED, PRESSURE_RETURN, PRESSURE_SPEED_AND_RETURN } controlMode_t;


typedef struct controlConfig_t {
    controlMode_t defaultMode;
    gpio_num_t gpioSetButton;
    gpio_num_t gpioStatusLed;
} controlConfig_t;


//==========================
//====== control task ======
//==========================
//task that controls what state the pump is in (user input)
//other tasks act depending on current mode
//Parameter: pointer to SystemModeController Object
void task_control( void * SystemModeController );


//==========================
//===== control class ======
//==========================
class SystemModeController
{
public:
    SystemModeController(controlConfig_t config);
    void handle();
    void changeMode(controlMode_t newMode);

private:
    controlConfig_t mConfig;
    controlMode_t mMode;
    float mSpeedTarget = 0;
};