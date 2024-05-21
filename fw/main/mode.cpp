extern "C" {
#include "esp_log.h"
}

#include "mode.hpp"
#include "global.hpp"
#include "common.hpp"

//tag for logging
static const char * TAG = "control";
//string for logging the control mode
extern const char *controlMode_str[] = {"IDLE", "SPEED", "REGULATE_PRESSURE", "REGULATE_REDUCED", "REGULATE_PRESSURE_VALVE_ONLY"};


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
        vTaskDelay(200 / portTICK_PERIOD_MS);
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


//--------------------
//----- initAdc ------
//--------------------
// initialize used ADC (poti)
void SystemModeController::initAdc(){
    ESP_LOGW(TAG, "initializing adc");
    // configure adc for poti
    adc1_config_width(ADC_WIDTH_BIT_12);                  //=> max resolution 4096
    adc1_config_channel_atten(mConfig.adcChannelPoti, ADC_ATTEN_DB_11); //max voltage
}


//====================
//==== changeMode ====
//====================
void SystemModeController::changeMode(controlMode_t newMode){
    ESP_LOGW(TAG, "changeMode: Switching from mode '%s' to '%s'", controlMode_str[(int)mMode], controlMode_str[(int)newMode]);
    mMode = newMode;

    //reset variables
    mTimestampLastFlow = esp_log_timestamp();
    mTimestampLastNoFlow = esp_log_timestamp();
    mTimestampLastNonZeroPressure = esp_log_timestamp();
}


//=================
//===== hande =====
//=================
// handle mode select, UI and timeouts
// repeatedly called by control task
void SystemModeController::handle()
{
    // initialize adc for poti once at first run
    static bool adcInitialized = false;
    if (!adcInitialized)
    {
        initAdc();
        adcInitialized = true;
    }

    ESP_LOGV(TAG, "handling controlled-pump...");

    // handle buttons
    mButtonMode.handle();
    mButtonSet.handle();

    //-------------------------------
    //------ MODE/START button ------
    //-------------------------------
    // handle change of on-state
    if (mButtonMode.risingEdge)
    {
        if (mMode == IDLE)
        { // currently IDLE
            mMode = REGULATE_PRESSURE;
            ESP_LOGW(TAG, "mode-button edge: Turning pump ON  (state REGULATE_PRESSURE)");
        }
        else
        { // not in idle already
            mMode = IDLE;
            ESP_LOGW(TAG, "mode-button edge: Turning pump OFF (state IDLE)");
            // mpVfd->turnOff();
        }
    }

    //------------------------
    //------ SET button ------
    //------------------------
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

        // show target pressure on display 2
        // TODO handle display priority
        static char buf[15];
        static char formatted[10];
        snprintf(formatted, 10, "%.3f", pressureTarget);
        formatted[5] = '\0'; // limit to 5 characters
        snprintf(buf, 15, "%s bar", formatted);
        // displayMid.showString(buf);
        displayMid.blinkStrings(buf, "        ", 299, 99, true);
    }
    // lock display while set button is pressed (get priority)
    if (mButtonSet.risingEdge)
    {
        // prevent display task from using the display
        displayMid.lock();
    }
    else if (mButtonSet.fallingEdge)
    {
        // stop showing target pressure, give display task access again
        displayMid.unlock();
    }


    //--------------------
    //----- timeouts -----
    //--------------------
    //configure timeouts
    // TODO: Adjust those thresholds during test
#define NO_FLOW_THRESHOLD 0.01 //liter per second considered 0 flow
#define NO_FLOW_TIMEOUT 20000 // pressure is reduced when flow below threshold for more than this time
#define NO_FLOW_REDUCED_PRESSURE 0.5 // pressure in bar when in reduced mode
#define NO_FLOW_REDUCED_TIMEOUT 1000 // time flow has to be above threshold for pressure restore

#define NO_PRESSURE_THRESHOLD 0.08  // when below that pressure for more than TIMEOUT switching to IDLE
#define NO_PRESSURE_TIMEOUT 20000

    // get current pressure
    float pressureNow = pressureSensor.getBar();

    // check for pressure
    if (pressureNow > NO_PRESSURE_THRESHOLD){
        ESP_LOGV(TAG, "pressure detected");
        mTimestampLastNonZeroPressure = esp_log_timestamp();
    }

    // check for water flow
    if (flowSensor.getFlowRate_literPerSecond() > NO_FLOW_THRESHOLD)
    {
        ESP_LOGV(TAG, "flow detected");
        mTimestampLastFlow = esp_log_timestamp();
    }
    else
    {
        ESP_LOGV(TAG, "no flow detected");
        mTimestampLastNoFlow = esp_log_timestamp();
    }

    // --- no-pressure timeout ---
    // when not in IDLE check for no pressure timeout (pump runs dry -> turn off)
    if (mMode != IDLE 
        && esp_log_timestamp() - mTimestampLastNonZeroPressure > NO_PRESSURE_TIMEOUT)
        //TODO false positive at high flowrate possible? also check flow?
        //&& flowSensor.getFlowRate_literPerSecond() < NO_FLOW_THRESHOLD
    {
        ESP_LOGE(TAG, "TIMEOUT - pressure less than %.2f for more than %d s! switching to IDLE", NO_PRESSURE_THRESHOLD, NO_PRESSURE_TIMEOUT / 1000);
        changeMode(IDLE);
        // TODO notify display or add TIMEOUT mode that has to be reset?
    }

    //--------------------------
    //------ statemachine ------
    //--------------------------
    // handle timeouts
    switch (mMode)
    {
    default:
    case IDLE:
        break;

    case REGULATE_PRESSURE: // normal operation
        // check for no flow timeout (reduce pressure)
        if (pressureNow > NO_PRESSURE_THRESHOLD &&
            esp_log_timestamp() - mTimestampLastFlow > NO_FLOW_TIMEOUT)
        {
            // store previous (actual desired) target pressure
            mTargetPressurePrevious = valveControl.getTargetPressure();
            // note: using min() to not increase pressure if already lower than the reduced value
            ESP_LOGW(TAG, "no flow detected for more than %d s -> reducing pressure to %.2f bar", NO_FLOW_TIMEOUT, min(NO_FLOW_REDUCED_PRESSURE, mTargetPressurePrevious));
            valveControl.setTargetPressure(min(NO_FLOW_REDUCED_PRESSURE, mTargetPressurePrevious));
            changeMode(REGULATE_REDUCED);
        }
        break;

    case REGULATE_REDUCED: // pressure reduced due to no flow
        // check if there is flow again for long enough
        if (esp_log_timestamp() - mTimestampLastNoFlow > NO_FLOW_REDUCED_TIMEOUT)
        {
            ESP_LOGW(TAG, "detected continuous flow for more than %d s -> setting pressure to previous value %.2f bar", NO_FLOW_REDUCED_TIMEOUT, mTargetPressurePrevious);
            // set to previous target pressure
            valveControl.setTargetPressure(mTargetPressurePrevious);
            changeMode(REGULATE_PRESSURE);
        }
        // check if target pressure got changed by user
        if (valveControl.getTargetPressure() != min(NO_FLOW_REDUCED_PRESSURE, mTargetPressurePrevious)){
            ESP_LOGW(TAG, "target pressure changed while in reduced mode -> switching to normal operation");
            mTimestampLastFlow = esp_log_timestamp(); // reset no-flow timeout
            changeMode(REGULATE_PRESSURE);
        }
        break;
    }
}