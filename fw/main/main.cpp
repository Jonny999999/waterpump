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

#include <stdio.h>
#include "global.hpp"
#include "vfd.hpp"
#include "mode.hpp"
#include "servo.hpp"
#include "pressureSensor.hpp"
#include "pumpControl.hpp"
#include "mqtt.hpp"

//tag for logging
static const char * TAG = "main";

extern "C" void app_main(void)
{
    // set loglevel
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("VFD", ESP_LOG_DEBUG);
    esp_log_level_set("servo", ESP_LOG_INFO);
    esp_log_level_set("control", ESP_LOG_INFO);
    esp_log_level_set("regulateValve", ESP_LOG_WARN);
    esp_log_level_set("regulateMotor", ESP_LOG_INFO);
    esp_log_level_set("mqtt-task", ESP_LOG_WARN);
    esp_log_level_set("lookupTable", ESP_LOG_VERBOSE);

    // enable 5V volage regulator (needed for pressure sensor and flow meter)
    gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_17, 1);

    // connect wifi
    wifi_connect();

    // connect mqtt
    mqtt_app_start();

    // create motor object
    Vfd4DigitalPins motor(GPIO_NUM_4, GPIO_NUM_16, GPIO_NUM_2, GPIO_NUM_15, true);

    // turn servo power supply on (onboard relay)
    servo.enable();

    // create control task (handle Buttons, Poti and define System-mode)
    xTaskCreate(&task_control, "task_control", 4096, &control, 5, NULL); // implemented in mode.cpp

    // create mqtt task (repeatedly publish variables)
    xTaskCreate(&task_mqtt, "task_mqtt", 4096 * 4, &control, 5, NULL); // implemented in mqtt.cpp

    // TODO add tasks "regulate-pressure", "mqtt", ...

    //===== TESTING =====

//--- test vfd ---
// #define VFD_TEST
#ifdef VFD_TEST
    // test on/off
    motor.turnOn();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    motor.turnOff();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    motor.turnOn();
    // test speed levels
    while (1)
    {
        // vary speed level time based
        // motor.setSpeedLevel(0);
        // motor.turnOn();
        // vTaskDelay(2000 / portTICK_PERIOD_MS);
        // for (int i = 0; i < 10; i++)
        //{
        //     motor.setSpeedLevel(i);
        //     vTaskDelay(2000 / portTICK_PERIOD_MS);
        // }
        // motor.turnOff();
        // vTaskDelay(5000 / portTICK_PERIOD_MS);

        // set motor speed using poti
        int potiRaw = adc1_get_raw(ADC_POTI);
        float potiPercent = (float)potiRaw / 4095 * 100;
        motor.setSpeedLevel(potiPercent * 4 / 100);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
#endif

// --- test poti, servo, pressure-sensor ---
// #define SERVO_TEST
#ifdef SERVO_TEST
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



    // repeately run actions depending on current system mode
    controlMode_t modeNow = IDLE;
    controlMode_t modePrev = IDLE;
    while (1)
    {
        // get current and store previous mode
        modePrev = modeNow;
        modeNow = control.getMode();

        // run actions depending on current mode
        switch (modeNow)
        {
        case REGULATE_PRESSURE:
            // initially turn on motor
            if (modePrev != REGULATE_PRESSURE)
            {
                ESP_LOGW(TAG, "changed to REGULATE_PRESSURE -> enable motor");
                motor.turnOn(2); // start motor at medium speed
            }
            // regulate valve pos
            valveControl.compute(pressureSensor.readBar());
            // regulate motor speed
            regulateMotor(valveControl.getPressureDiff(), &servo, &motor);
            break;

        default:
            // turn motor off when previously in regulate mode
            if (modePrev == REGULATE_PRESSURE)
            {
                ESP_LOGW(TAG, "changed from REGULATE_PRESSURE -> disable motor, close valve");
                motor.turnOff();
                servo.setPercentage(0);
                valveControl.reset(); // reset regulator
            }
            break;
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    while (1)
    {
        vTaskDelay(portMAX_DELAY);
    }
}
