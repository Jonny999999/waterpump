#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "vfd.hpp"
#include "freertos/task.h"

extern "C" void app_main(void)
{
    // set loglevel
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("VFD", ESP_LOG_DEBUG);

    // create motor object
    Vfd4DigitalPins motor(GPIO_NUM_4, GPIO_NUM_16, GPIO_NUM_2, GPIO_NUM_15, false);

    // test VFD
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
}
