extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/adc.h"

#include "wifi.h"
#include "mqtt.h"
}

#include "vfd.hpp"
#include "servo.hpp"
#include "pressureSensor.hpp"
#include "display.hpp"

#include "mqtt.hpp"
#include "pumpControl.hpp"
#include "mode.hpp"
#include "global.hpp"
#include "config.h"

// tag for logging
static const char *TAG = "main";


extern "C" void app_main(void)
{
    // set loglevel
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("VFD", ESP_LOG_DEBUG);
    esp_log_level_set("servo", ESP_LOG_WARN);
    esp_log_level_set("control", ESP_LOG_INFO);
    esp_log_level_set("pressure", ESP_LOG_WARN);
    esp_log_level_set("flowSensor", ESP_LOG_DEBUG);
    esp_log_level_set("regulateValve", ESP_LOG_WARN);
    esp_log_level_set("regulateMotor", ESP_LOG_INFO);
    esp_log_level_set("mqtt-task", ESP_LOG_WARN);
    esp_log_level_set("mqtt-cpp", ESP_LOG_WARN);
    esp_log_level_set("lookupTable", ESP_LOG_WARN);
    esp_log_level_set("display", ESP_LOG_INFO);

    // enable 5V voltage regulator (needed for pressure sensor and display)
    gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_17, 1);

    // connect wifi
    wifi_connect();

    // connect mqtt broker
    mqtt_app_start();

    // turn servo power supply on (onboard relay)
    servo.enable();

    // enable interrupts, initialize flow sensor
    gpio_install_isr_service(0);
    flowSensor.init();

    // initialize 7 segment display
    vTaskDelay(10 / portTICK_PERIOD_MS); // ensure 5v are up
    // create and initialize display device/driver (3 modules connected in series)
    three7SegDisplays = display_init();
    max7219_set_brightness(&three7SegDisplays, DISPLAY_BRIGHTNESS);
    // initialize the global display objects (pass display device)
    displayTop.init(three7SegDisplays);
    displayMid.init(three7SegDisplays);
    displayBot.init(three7SegDisplays);

    // create control task (handle Buttons, Poti and define System-mode)
    xTaskCreate(&task_control, "task_control", 4096, &control, 5, NULL); // implemented in mode.cpp

    // create mqtt task (repeatedly publish variables)
    xTaskCreate(&task_mqtt, "task_mqtt", 4096 * 4, NULL, 5, NULL); // implemented in mqtt.cpp

    // create display task (handle display content)
    xTaskCreate(&task_display, "task_display", 4096, NULL, 5, NULL); // implemented in display.cpp

    // TODO add more tasks here e.g. "regulate-pressure", ...

    //===================
    //===== TESTING =====
    //===================
    //--- test vfd ---
// #define TEST_VFD
#ifdef TEST_VFD
    // test on/off
    motor.turnOn();
    // test speed levels
    while (1)
    {
        // set motor speed using poti
        int potiRaw = adc1_get_raw(ADC_POTI);
        float potiPercent = (float)potiRaw / 4095 * 100;
        motor.setSpeedLevel(potiPercent * 4 / 100);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
#endif

    //--- test servo, pressure-sensor ---
// #define TEST_SERVO
#ifdef TEST_SERVO
    while (1)
    {
        // test pressure sensor
        pressureSensor.readBar();
        // read poti
        int potiRaw = adc1_get_raw(ADC_POTI);
        float potiPercent = (float)potiRaw / 4095 * 100;
        ESP_LOGI(TAG, "poti adc=%d per=%f", potiRaw, potiPercent);
        // apply poti to servo
        servo.setPercentage(potiPercent);
        // servo.setAngle(180*potiPercent/100);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
#endif

    //--- test flow sensor ---
// #define TEST_FLOW_SENSOR
#ifdef TEST_FLOW_SENSOR
    while (1)
    {
        ESP_LOGI(TAG, "pulse-count = %ld", flowSensor.getPulseCount());
        vTaskDelay(150 / portTICK_PERIOD_MS);
        flowSensor.read();
    }
#endif

    //=====================
    //===== main loop =====
    //=====================
    // variables
    controlMode_t modeNow = IDLE;
    controlMode_t modePrev = IDLE;
    // repeatedly run actions depending on current system mode
    while (1)
    {
        // read/update flow sensor
        flowSensor.read(); //TODO read slower, move this to separate task?

        // get current and store previous mode
        modePrev = modeNow;
        modeNow = control.getMode();

        // run actions depending on current mode
        switch (modeNow)
        {
        case REGULATE_REDUCED:
        case REGULATE_PRESSURE:
            // turn on motor initially
            if (modePrev != REGULATE_PRESSURE && modePrev != REGULATE_REDUCED)
            {
                ESP_LOGW(TAG, "changed to %s -> enable motor", controlMode_str[(int)modeNow]);
                motor.turnOn(1); // start motor at medium speed
            }
            // regulate valve pos
            valveControl.compute(pressureSensor.readBar());
            // regulate motor speed
            regulateMotor(valveControl.getPressureDiff(), &servo, &motor);
            break;

        case IDLE:
        default:
            // turn off motor when previously in regulate mode
            if (modePrev == REGULATE_PRESSURE || modePrev == REGULATE_REDUCED)
            {
                ESP_LOGW(TAG, "changed from %s -> disable motor, close valve", controlMode_str[(int)modePrev]);
                motor.setSpeedLevel(0);
                motor.turnOff();
                servo.setPercentage(0);
                valveControl.reset(); // reset PID controller
            }
            break;
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
        //vTaskDelay(portMAX_DELAY);
    } // end main-loop
} // end app_main()
