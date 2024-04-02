#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "sensordeklaration.h"
#include "wasserpumpe.h"

#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "mqtt.h"
#define X1 15
#define X2 2
#define X3 16
#define X4 4


// config
struct wasserpumpe_t wp = {
    .name = "wasserpumpe 1",
    .state = IDLE,
    .mode = POTI,
    .druck = 0,
    .druck_max = 0,
    .druck_min = 0,
    .fu_level = 0,
    .druck_soll = 0,
    .st1 = false,
    .st2 = false,
    .st3 = false,
    .st4 = false};



float VoltageToBar(uint32_t voltage_mv)
{
    voltage_mv = voltage_mv * 2; // x2 da spannungsteiler 2k2 2k2
    float psi = (75 * (voltage_mv / (float)1000) - 37.5);
    float bar = psi * 0.0689476;
    return bar;
}



void wp_ReadPressure()
{
    uint32_t average = 0;
    uint32_t max = 0;
    uint32_t min = 2 ^ 30;

    // fuer 4s messen
    printf("start reading pressure");
    for (int j = 0; j < 300; j++)
    {
        uint32_t voltage = adc1_get_raw(ADC1_CHANNEL_0) / 4095 * 3.3; // gpio36
        // mittelwert bestimmen
        average = average + voltage;

        // min-max bestimmen
        if (voltage < min)
        {
            min = voltage;
        }
        if (voltage > max)
        {
            max = voltage;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
    wp.druck = VoltageToBar(average / 2000);
    wp.druck_min = VoltageToBar(min);
    wp.druck_max = VoltageToBar(max);
    printf("\nDruck ausgelesen: min: %f; mitte: %f; max: %f\n", wp.druck_min, wp.druck, wp.druck_max);
}



void wp_FuSetLevel(int level)
{
    // gpio pin zuweisung

    wp.fu_level = level;
    switch (level)
    {
    case 0:
        gpio_set_level(X1, 0);
        gpio_set_level(X2, 0);
        gpio_set_level(X3, 0);
        gpio_set_level(X4, 0); // startpin
        wp.st1 = false;
        wp.st2 = false;
        wp.st3 = false;
        wp.st4 = false;
        break;
    case 1:
        gpio_set_level(X1, 1);
        gpio_set_level(X2, 1);
        gpio_set_level(X3, 0);
        gpio_set_level(X4, 1); // startpin
        wp.st1 = true;
        wp.st2 = true;
        wp.st3 = false;
        wp.st4 = true;
        break;
    case 2:
        gpio_set_level(X1, 1);
        gpio_set_level(X2, 0);
        gpio_set_level(X3, 1);
        gpio_set_level(X4, 1); // startpin
        wp.st1 = true;
        wp.st2 = false;
        wp.st3 = true;
        wp.st4 = true;
        break;
    case 3:
        gpio_set_level(X1, 1);
        gpio_set_level(X2, 0);
        gpio_set_level(X3, 0);
        gpio_set_level(X4, 1); // startpin
        wp.st1 = true;
        wp.st2 = false;
        wp.st3 = false;
        wp.st4 = true;
        break;
    case 4:
        gpio_set_level(X1, 0);
        gpio_set_level(X2, 1);
        gpio_set_level(X3, 1);
        gpio_set_level(X4, 1); // startpin
        wp.st1 = false;
        wp.st2 = true;
        wp.st3 = true;
        wp.st4 = true;
        break;
    case 5:
        gpio_set_level(X1, 0);
        gpio_set_level(X2, 1);
        gpio_set_level(X3, 0);
        gpio_set_level(X4, 1); // startpin
        wp.st1 = false;
        wp.st2 = true;
        wp.st3 = false;
        wp.st4 = true;
        break;
    case 6:
        gpio_set_level(X1, 0);
        gpio_set_level(X2, 0);
        gpio_set_level(X3, 1);
        gpio_set_level(X4, 1); // startpin
        wp.st1 = false;
        wp.st2 = false;
        wp.st3 = true;
        wp.st4 = true;
        break;
    case 7:
        gpio_set_level(X1, 1);
        gpio_set_level(X2, 1);
        gpio_set_level(X3, 1);
        gpio_set_level(X4, 1); // startpin
        wp.st1 = true;
        wp.st2 = true;
        wp.st3 = true;
        wp.st4 = true;
        break;
    }
    mqtt_PublishWasserpumpe(wp);
}
