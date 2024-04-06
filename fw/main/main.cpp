extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
}

#include <stdio.h>
#include "vfd.hpp"
#include "mode.hpp"

extern "C" void app_main(void)
{
    // set loglevel
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("VFD", ESP_LOG_DEBUG);
    esp_log_level_set("control", ESP_LOG_INFO);

    //enable 5V volage regulator (needed for pressure sensor and flow meter)
    gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_17, 1);

    // create motor object
    Vfd4DigitalPins motor(GPIO_NUM_4, GPIO_NUM_16, GPIO_NUM_2, GPIO_NUM_15, false);

    // create control object
    controlConfig_t controlConfig{
        .defaultMode = IDLE,
        .gpioSetButton = GPIO_NUM_11,
        .gpioStatusLed = GPIO_NUM_10
    };
    SystemModeController control(controlConfig);

    // create control task
    // TODO: is this task necessary?
    xTaskCreate(&task_control, "task_control", 4096, &control, 5, NULL);

    //TODO add tasks "regulate-pressure", "mqtt", ...


    while (1)
    {
        vTaskDelay(portMAX_DELAY);
        //    motor.setSpeedLevel(0);
        //    motor.turnOn();
        //    vTaskDelay(2000 / portTICK_PERIOD_MS);
        //    for (int i = 0; i < 10; i++)
        //    {
        //        motor.setSpeedLevel(i);
        //        vTaskDelay(2000 / portTICK_PERIOD_MS);
        //    }
        //    motor.turnOff();
        //    vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
