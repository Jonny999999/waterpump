extern "C" {
#include "esp_log.h"
}

#include "mode.hpp"
#include "global.hpp"

//tag for logging
static const char * TAG = "control";

//==========================
//====== control task ======
//==========================
//task that handles user input (buttons, poti) 
//controls what state the pump is in (user input)
//other tasks act depending on current mode
//Parameter: pointer to SystemModeController Object
void task_control(void *pvParameters)
{
    SystemModeController *control = (SystemModeController *)pvParameters;
    ESP_LOGW(TAG, "starting handle loop");
    while (1)
    {
        control->handle();
        vTaskDelay(150 / portTICK_PERIOD_MS);
    }
}

//===================
//=== constructor ===
//===================
SystemModeController::SystemModeController(controlConfig_t config)
    : mButtonMode(config.gpioModeButton, false, true), // pullup=false, inverted=true (swith to 3v3, pulldown soldered)
      mButtonSet(config.gpioSetButton, false, true)
{
    ESP_LOGW(TAG, "Initializing SystemModeController");
    // copy config and objects, set defaults
    mConfig = config;
    mMode = mConfig.defaultMode;
    //TODO init gpios
}


// initialize used ADC (poti)
void SystemModeController::initAdc(){
    ESP_LOGW(TAG, "initializing adc");
    // configure adc for poti
    adc1_config_width(ADC_WIDTH_BIT_12);                  //=> max resolution 4096
    adc1_config_channel_atten(mConfig.adcChannelPoti, ADC_ATTEN_DB_11); //max voltage
}


//=================
//===== hande =====
//=================
void SystemModeController::handle(){
    // initialize adc for poti once at first run
    static bool adcInitialized = false;
    if (!adcInitialized) {
        initAdc();
        adcInitialized = true;
    }

    ESP_LOGV(TAG, "handling controlled-pump...");

    // handle buttons
    mButtonMode.handle();
    mButtonSet.handle();

    // handle change of on-state
    if (mButtonMode.risingEdge){
        if (mMode == IDLE) { // currently IDLE
            mMode = REGULATE_PRESSURE;
            ESP_LOGW(TAG, "mode-button edge: Turning pump ON  (state REGULATE_PRESSURE)");
        } else { // not in idle already
            mMode = IDLE;
            ESP_LOGW(TAG, "mode-button edge: Turning pump OFF (state IDLE)");
            // mpVfd->turnOff();
        }
    }

    // handle poti when set button is pressed
#define MAX_PRESSURE 8
    if (mButtonSet.state)
    {
        //  read poti   TODO: multisampling
        int potiRaw = adc1_get_raw(mConfig.adcChannelPoti);
        float potiPercent = (float)potiRaw / 4095 * 100;
        // define target pressure
        float pressureTarget = potiPercent * MAX_PRESSURE / 100;
        valveControl.setTargetPressure(pressureTarget);
        ESP_LOGW(TAG, "set-button pressed: Updating target pressure to %.3f bar (poti)", pressureTarget);
    }

// handle current mode
//    switch (mMode){
//        case IDLE:
//            break;
//
//        case SPEED:
//            //if (buttonSet.fallingEdge)
//            //{
//            //    mpVfd->setSpeedLevel(getSpeedLevelFromPoti());
//            //}
//        break;
//
//        case REGULATE_PRESSURE:
//        break;
//
//        case REGULATE_PRESSURE_VALVE_ONLY:
//        break;
//    }
}
