extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "esp_log.h"
#include "driver/adc.h"
}

#include <stdio.h>
#include "vfd.hpp"
#include "mode.hpp"
#include "servo.hpp"
#include "pressureSensor.hpp"

//tag for logging
static const char * TAG = "main";

extern "C" void app_main(void)
{
    // set loglevel
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("VFD", ESP_LOG_DEBUG);
    esp_log_level_set("servo", ESP_LOG_INFO);
    esp_log_level_set("control", ESP_LOG_INFO);

    //enable 5V volage regulator (needed for pressure sensor and flow meter)
    gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_17, 1);

    // create motor object
    Vfd4DigitalPins motor(GPIO_NUM_4, GPIO_NUM_16, GPIO_NUM_2, GPIO_NUM_15, false);

    // create servo object
    servoConfig_t servoConfig {
        .gpioPwmSignal = 27,
        .minAngle = 0,
        .maxAngle = 180,
        .invertDirection = false
    };
    ServoMotor servo(servoConfig);

    // create control object
    controlConfig_t controlConfig{
        .defaultMode = IDLE,
        .gpioSetButton = GPIO_NUM_11,
        .gpioStatusLed = GPIO_NUM_10
    };
    SystemModeController control(controlConfig);

    // create pressure sensor on gpio 36
    AnalogPressureSensor pressureSensor(ADC1_CHANNEL_0, 0.25, 2.5, 0, 30);


    // create control task
    // TODO: is this task necessary?
    //xTaskCreate(&task_control, "task_control", 4096, &control, 5, NULL);

    //TODO add tasks "regulate-pressure", "mqtt", ...

    //===== TESTING =====
//--- test servo ---
#ifdef SERVO_TEST
    while (1)
    {
        servo.runTestDrive();
    }
#endif


//--- test vfd ---
#ifdef VFD_TEST
    while (1)
    {
        motor.setSpeedLevel(0);
        motor.turnOn();
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        for (int i = 0; i < 10; i++)
        {
            motor.setSpeedLevel(i);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
        motor.turnOff();
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
#endif


// configure adc for poti
#define ADC_POTI ADC1_CHANNEL_6 //gpio34
    adc1_config_width(ADC_WIDTH_BIT_12);                  //=> max resolution 4096
    adc1_config_channel_atten(ADC_POTI, ADC_ATTEN_DB_11); //max voltage


// --- test poti, servo, pressure-sensor ---
while(1){
    // test pressure sensor
    pressureSensor.readBar();
    // read poti
    int potiRaw = adc1_get_raw(ADC_POTI);
    float potiPercent = (float)potiRaw/4095*100;
    ESP_LOGI(TAG, "poti adc=%d per=%f", potiRaw, potiPercent);
    // apply poti to servo
    servo.setAngle(180*potiPercent/100);
    vTaskDelay(100 / portTICK_PERIOD_MS);
}



    while (1)
    {
        vTaskDelay(portMAX_DELAY);
    }
}
