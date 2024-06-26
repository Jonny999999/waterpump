#include "display.hpp"

#include "config.h"
#include "global.hpp"

//=== variables ===
static const char *TAG = "display"; //tag for logging

//==========================
//====== display task ======
//==========================
// task that handles the actual display output
// shows pressure, valve, flow, volume, target pressure etc... depending on modes
#define DISPLAY_RE_INIT_INTERVAL 15000 // initialize display every x ms (workaround crashes because of noise)
void task_display(void *pvParameters)
{
    ESP_LOGW(TAG, "starting display task...");

    char buf[15];
    char buf1[15];
    char formatted[10];
    uint32_t timestamp_displayInitialized = 0;
    while (1)
    {
        // handle all display objects
        // (handle blinking, apply content from other taks using 'noUpdate')
        displayTop.handle();
        displayMid.handle();
        displayBot.handle();

        // re-initialize display when due
        if (esp_log_timestamp() - timestamp_displayInitialized > DISPLAY_RE_INIT_INTERVAL)
        {
            ESP_LOGW(TAG, "re-initializing display (interval=%ds)...", DISPLAY_RE_INIT_INTERVAL / 1000);
            ESP_LOGD(TAG, "shutdown display");
            max7219_set_shutdown_mode(&three7SegDisplays, true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            ESP_LOGD(TAG, "init display");
            max7219_init(&three7SegDisplays);
            max7219_set_brightness(&three7SegDisplays, DISPLAY_BRIGHTNESS);
            timestamp_displayInitialized = esp_log_timestamp();
        }

        // display top: show current pressure (4 digits, variable decimal palaces)
        if (!displayTop.isLocked())
        {
            snprintf(formatted, 10, "%.3f", pressureSensor.readBar());
            formatted[5] = '\0'; // limit to 5 characters
            snprintf(buf, 15, "%s bar", formatted);
            displayTop.showString(buf);
        }

        // display middle: show current valve percent (3 digits, variable decimal palaces)
        if (!displayMid.isLocked()) // dont update here when currently used by task_contol (shows target pressure)
        {
            snprintf(formatted, 10, "%04.2f", servo.getPercent());
            formatted[4] = '\0'; // limit to 4 characters
            snprintf(buf, 15, "v%s per", formatted);
            displayMid.showString(buf);
        }

        // displat bot:
        if (!displayBot.isLocked())
        {
            snprintf(buf, 15, "%4.0f Lit", flowSensor.getVolume_liter());
            snprintf(buf1, 15, "%4.0f Lpm", flowSensor.getFlowRate_literPerSecond() * 60);
            // displayBot.showString(buf);
            displayBot.blinkStrings(buf, buf1, 2000, 1000);
            // TODO rotate through volume, flow, target-pressure etc...
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}


//#################################
//######## display library ########
//#################################

//--------------------------
//----- display config -----
//--------------------------
// definitions used from config.h
//#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 0, 0)
//#define HOST    HSPI_HOST
//#else
//#define HOST    SPI2_HOST
//#endif
//#define DISPLAY_PIN_NUM_MOSI GPIO_NUM_23
//#define DISPLAY_PIN_NUM_CLK GPIO_NUM_22
//#define DISPLAY_PIN_NUM_CS GPIO_NUM_27
//#define DISPLAY_DELAY 2000
//#define DISPLAY_BRIGHTNESS 8 // 0 - 15
//#define DISPLAY_MODULE_COUNT_CONNECTED_IN_SERIES 3
//#define DISPLAY_CLOCK_SPEED_HZ 1000000 //max 1MHz (default if not defined)


//use configured clock speed if defined
#ifndef DISPLAY_CLOCK_SPEED_HZ
#define DISPLAY_CLOCK_SPEED_HZ MAX7219_MAX_CLOCK_SPEED_HZ
#endif


//==============================
//======== init display ========
//==============================
//initialize display with parameters defined in config.hpp
//TODO: dont use global variables/macros here
max7219_t display_init(){
    ESP_LOGI(TAG, "initializing display...");
    // Configure SPI bus
    spi_bus_config_t cfg;
    memset(&cfg, 0, sizeof(spi_bus_config_t)); //init bus config with 0 to prevent bugs with random flags
    cfg.mosi_io_num = DISPLAY_PIN_NUM_MOSI;
    cfg.miso_io_num = -1;
    cfg.sclk_io_num = DISPLAY_PIN_NUM_CLK;
    cfg.quadwp_io_num = -1;
    cfg.quadhd_io_num = -1;
    cfg.max_transfer_sz = 0;
    cfg.flags = 0;
    ESP_ERROR_CHECK(spi_bus_initialize(HOST, &cfg, 1));

    // Configure device
    max7219_t dev;
    dev.cascade_size = DISPLAY_MODULE_COUNT_CONNECTED_IN_SERIES;
    dev.digits = 0;
    dev.mirrored = true;
    ESP_ERROR_CHECK(max7219_init_desc(&dev, HOST, DISPLAY_CLOCK_SPEED_HZ, DISPLAY_PIN_NUM_CS));
    ESP_ERROR_CHECK(max7219_init(&dev));
    //0...15
    ESP_ERROR_CHECK(max7219_set_brightness(&dev, DISPLAY_BRIGHTNESS));
    ESP_LOGI(TAG, "initializing display - done");
    return dev;
    //display = dev;
}



//===================================
//======= display welcome msg =======
//===================================
void display_ShowWelcomeMsg(max7219_t dev){
    //display welcome message on two 7 segment displays
    //show name and date
    ESP_LOGI(TAG, "showing startup message...");
    max7219_clear(&dev);
    max7219_draw_text_7seg(&dev, 0, "CUTTER  15.03.2024");
    //                                   1234567812 34 5678
    vTaskDelay(pdMS_TO_TICKS(700));
    //scroll "hello" over 2 displays
    for (int offset = 0; offset < 23; offset++) {
        max7219_clear(&dev);
        char hello[40] = "                HELL0                 ";
        max7219_draw_text_7seg(&dev, 0, hello + (22 - offset) );
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    //noticed rare bug that one display does not initialize / is not configured correctly after start
    //initialize display again after the welcome message in case it did not work the first time
    ESP_ERROR_CHECK(max7219_init(&dev));
    ESP_ERROR_CHECK(max7219_set_brightness(&dev, DISPLAY_BRIGHTNESS));
}





//---------------------------------
//---------- constructor ----------
//---------------------------------
handledDisplay::handledDisplay(max7219_t displayDevice, uint8_t posStart_f) {
    ESP_LOGI(TAG, "Creating handledDisplay instance with startPos at %i", posStart);
    //copy variables
    posStart = posStart_f;
    init(displayDevice);
}
handledDisplay::handledDisplay(uint8_t posStart_f) {
    ESP_LOGI(TAG, "Creating handledDisplay instance with startPos at %i", posStart);
    ESP_LOGI(TAG, "(note: .init(...) has to be run manually later!)");
    posStart = posStart_f;
}

//----------------
//----- init -----
//----------------
void handledDisplay::init(max7219_t displayDevice){
    ESP_LOGI(TAG, "initialized display object with reference to hardware");
    dev = displayDevice;
    isInitialized = true;
}


//================================
//========== showString ==========
//================================
//function that displays a given string on the display
void handledDisplay::showString(const char * buf, uint8_t pos_f, bool noUpdate){
    //calculate actual absolute position
    posCurrent = posStart + pos_f;
    //copy the desired string
    strcpy(strOn, buf);
    //exit blinking mode
    if (mode == displayMode::BLINK_STRINGS){
        mode = displayMode::NORMAL;
        ESP_LOGI(TAG, "pos:%i - disable blink strings mode -> normal mode str='%s'", posStart, strOn);
    }
    if (!noUpdate)  // when noUpdate is set display updates at next manual handle run
        handle(); // draws the text depending on mode
}



//TODO: blinkStrings() and blink() are very similar - can be optimized?
//only difficulty is the reset behaivor of blinkStrings through showString (blink does not reset)

//==================================
//========== blinkStrings ==========
//==================================
//function switches between two strings in a given interval
void handledDisplay::blinkStrings(const char * strOn_f, const char * strOff_f, uint32_t msOn_f, uint32_t msOff_f, bool noUpdate){
    //copy/update variables
    strcpy(strOn, strOn_f);
    strcpy(strOff, strOff_f);
    msOn = msOn_f;
    msOff = msOff_f;
    //if changed to blink mode just now:
    if (mode != displayMode::BLINK_STRINGS) {
        //switch mode
        ESP_LOGI(TAG, "pos:%i - toggle blink strings mode on/off=%ld/%ld stings='%s'/'%s'", posStart, msOn, msOff, strOn, strOff);
        mode = displayMode::BLINK_STRINGS;
        //start with on state
        state = true;
        timestampOn = esp_log_timestamp();
    }
    //run handle function for display update
    if (!noUpdate)  // when noUpdate is set display updates at next manual handle run
        handle();
}



//===============================
//============ blink ============
//===============================
//function triggers certain count and interval of off durations
void handledDisplay::blink(uint8_t count_f, uint32_t msOn_f, uint32_t msOff_f, const char * strOff_f, bool noUpdate) {
    //copy/update parameters
    count = count_f;
    msOn = msOn_f;
    msOff = msOff_f;
    strcpy(strOff, strOff_f);
    //FIXME this strings length must be dynamic depending on display size (posEnd - posStart) -> otherwise overwrites next segments if other display size or start pos
    //if changed to blink mode just now:
    if (mode != displayMode::BLINK) {
        //set to blink mode
        mode = displayMode::BLINK;
        ESP_LOGI(TAG, "pos:%i - start blinking: count=%i  on/off=%ld/%ld sting='%s'",posStart, count, msOn, msOff, strOff);
        //start with off state
        state = false;
        timestampOff = esp_log_timestamp();
    }
    //run handle function for display update
    if (!noUpdate)  // when noUpdate is set display updates at next manual handle run
        handle();
}



//================================
//============ handle ============
//================================
//function that handles time based modes
//writes text to the 7 segment display depending on the current mode
void handledDisplay::handle() {
    // check if initialized (device is actually valid)
    if (!isInitialized) {
        ESP_LOGE(TAG, "display not initialized! make sure to pass max7219 device first by running `.init(device)`");
        return;
    }

    switch (mode){
        case displayMode::NORMAL:
            //daw given string
            max7219_draw_text_7seg(&dev, posCurrent, strOn);
            ESP_LOGD(TAG, "writing string '%s' to display pos %d", strOn, posStart);
            break;

        case displayMode::BLINK:
        case displayMode::BLINK_STRINGS:
            //--- define state on/off ---
            if (state == true){ //display in ON state
                if (esp_log_timestamp() - timestampOn > msOn){
                    state = false;
                    timestampOff = esp_log_timestamp();
                    //decrement remaining counts in BLINK mode each cycle
                    if (mode == displayMode::BLINK) count--;
                }
            } else { //display in OFF state
                if (esp_log_timestamp() - timestampOff > msOff) {
                    state = true;
                    timestampOn = esp_log_timestamp();
                }
            }
            //--- draw text of current state ---
            if (state) {
                max7219_draw_text_7seg(&dev, posStart, strOn);
            } else {
                max7219_draw_text_7seg(&dev, posStart, strOff);
            }

            //--- check finished condition in BLINK mode ---
            if (mode == displayMode::BLINK){
                if (count == 0) {
                    mode = displayMode::NORMAL;
                    ESP_LOGI(TAG, "pos:%i - finished blinking -> normal mode", posStart);
                }
            }
            break;
    }
}
