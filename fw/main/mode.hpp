#pragma once
extern "C" {
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/adc.h"
}
#include "vfd.hpp"
#include "gpio_evaluateSwitch.hpp"

typedef enum controlMode_t {IDLE, SPEED, REGULATE_PRESSURE, REGULATE_PRESSURE_VALVE_ONLY } controlMode_t;
extern const char *controlMode_str[4];


typedef struct controlConfig_t {
    controlMode_t defaultMode;
    gpio_num_t gpioSetButton;
    gpio_num_t gpioModeButton;
    gpio_num_t gpioStatusLed;
    adc1_channel_t adcChannelPoti;
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
    controlMode_t getMode() const {return mMode;};

private:
    void initAdc();
    controlConfig_t mConfig;
    controlMode_t mMode;
    float mSpeedTarget = 0;
    gpio_evaluatedSwitch mButtonMode;
    gpio_evaluatedSwitch mButtonSet;
};