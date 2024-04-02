#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_log.h"
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "sensordeklaration.h"
#include "digitalsensoren.h"



void init_digitalsensors()
{
    for (int i = 0; i < digitalsensor_anzahl; i++)
    {
        gpio_pad_select_gpio(Digitalsensoren[i].gpio_pin);
        gpio_set_direction(Digitalsensoren[i].gpio_pin, GPIO_MODE_INPUT);
    }
}




void read_digitalsensors()
{
    for (int i = 0; i < digitalsensor_anzahl; i++)
    {
        uint32_t samples_on = 0;
        uint32_t samples_off = 0;
        uint32_t count = 0;
        // uint32_t samples_off = 0;
        float average = 0;
        for (int j = 0; j < Digitalsensoren[i].sample_count; j++)
        {
            // printf("sample");
            if (gpio_get_level(Digitalsensoren[i].gpio_pin) == 1)
            {
                samples_on++;
            }
            else
            {
                samples_off++;
            }
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }

        if (Digitalsensoren[i].inverted == true)
        {
            count = samples_off;
        }
        else
        {
            count = samples_on;
        }

        average = (float)count * (float)Digitalsensoren[i].factor / (float)Digitalsensoren[i].sample_count;
        Digitalsensoren[i].value = average;

        printf("=> Digitalsensor: %s, samples: %d => duty/average: %.2f \n", Digitalsensoren[i].name, Digitalsensoren[i].sample_count, Digitalsensoren[i].value);
    }
}
