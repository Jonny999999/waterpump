extern "C" {
#include "esp_log.h"
}

#include "mode.hpp"

//tag for logging
static const char * TAG = "control";

//==========================
//====== control task ======
//==========================
//task that controls what state the pump is in (user input)
//other tasks act depending on current mode
//Parameter: pointer to SystemModeController Object
void task_control( void * pvParameters ){
    SystemModeController * pump = (SystemModeController *)pvParameters;
    ESP_LOGW(TAG, "starting handle loop");
    pump->handle();
}


//===================
//=== constructor ===
//===================
SystemModeController::SystemModeController(controlConfig_t config){
    ESP_LOGW(TAG, "Initializing SystemModeController");
    // copy config and objects, set defaults
    mConfig = config;
    mMode = IDLE;
    //TODO init gpios
}



//=================
//===== hande =====
//=================
void SystemModeController::handle(){
    ESP_LOGV(TAG, "handling controlled-pump...");
    /* GPIO objects not available yet
    // handle buttons
    buttonSet.handle();
    buttonToggleOn.handle();

    // handle change of on-state
    if (buttonToggleOn.risingEdge){
        if (mMode == IDLE) {
            mMode = mConfig.defaultMode; //TODO select mode / switch last mode
            ESP_LOGW(TAG, "toggle-button: Turning pump ON");
            mpVfd->turnOn(mConfig.defaultSpeedLevel);
        } else {
            mMode = IDLE;
            ESP_LOGW(TAG, "toggle-button: Turning pump OFF (IDLE)");
            mpVfd->turnOff();
        }
}
*/


// handle current mode
    switch (mMode){
        case IDLE:
            break;

        case SPEED:
            //if (buttonSet.fallingEdge)
            //{
            //    mpVfd->setSpeedLevel(getSpeedLevelFromPoti());
            //}
        break;

        case PRESSURE_SPEED:
        break;

        case PRESSURE_RETURN:
        break;

        case PRESSURE_SPEED_AND_RETURN:
        break;
    }
}
