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
#include "analogsensoren.h"

#include <rom/ets_sys.h>



void init_analogsensors()
{

    adc1_config_width(ADC_WIDTH_BIT_12); //=> max aufloesung 4096

    for (int i = 0; i < analogsensor_anzahl; i++)
    {
        adc1_config_channel_atten(Analogsensoren[i].channel, Analogsensoren[i].atten);
        printf("Analogsensor: %s configured!\n", Analogsensoren[i].name);
    }
}



void read_analogsensors()
{
    for (int i = 0; i < analogsensor_anzahl; i++)
    {
        if (Analogsensoren[i].MODE == DC)
        {
            read_analogsensors_dc(i);
        }
        else if (Analogsensoren[i].MODE == AC)
        {
            read_analogsensors_ac(i);
        }
        else
        {
            printf("Error, sensor %s : MODE nicht definiert!", Analogsensoren[i].name);
        }
    }
}



// DC Analogsensoren auslesen (Mittelwert)
void read_analogsensors_dc(int sensornum)
{
    uint32_t measure = 0;
    for (int j = 0; j < 10000; j++)
    {
        measure = measure + adc1_get_raw(Analogsensoren[sensornum].channel1) / 4096 * 3.3;
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    uint32_t voltage = measure / 10000;

    Analogsensoren[sensornum].value = (voltage * Analogsensoren[sensornum].umrechnung_faktor + Analogsensoren[sensornum].umrechnung_offset) * Analogsensoren[sensornum].umrechnung_einheit;
    printf("=> DC Analogsensor: %s, UIn=%d, Value: %.2f \n", Analogsensoren[sensornum].name, voltage, Analogsensoren[sensornum].value);
}



// AC Analogsensoren auslesen (effektivwert)
void read_analogsensors_ac(int sensornum)
{

    //==========Gesamtmittelwert bestimmen==========
    uint32_t measure_avg = 0;
    for (int j = 0; j < 1000; j++)
    {
        measure_avg = measure_avg + adc1_get_raw(Analogsensoren[sensornum].channel1);
        ets_delay_us(1000);
        // vTaskDelay(pdMS_TO_TICKS(2));
    }
    uint32_t voltage_avg = measure_avg / 1000;
    // printf("Average: %d\n",voltage_avg);

    //==========Effektivmittelwert bestimmen==========
    // Referenzspannung konstanter wert oder jedesmal aus avg bestimmen?
    uint32_t voltage_bias = voltage_avg; // Auswahl!!
    uint32_t voltage_eff = 0;
    uint32_t measure = 0;
    uint16_t count = 0;
    for (int j = 0; j < 1000; j++)
    {
        measure = adc1_get_raw(Analogsensoren[sensornum].channel1) / 4096 * 3.3;
        if (measure > voltage_bias)
        {
            // spannung oberhalb vom bias addieren
            voltage_eff = voltage_eff + measure - voltage_bias;
            count++;
        }
        else
        {
            // spannung unterhalb vom bias addieren (hochklappen)
            voltage_eff = voltage_eff + voltage_bias - measure;
            count++;
        }
        ets_delay_us(1000);
    }
    // mittelwert/effektivwert berechnen
    voltage_eff = voltage_eff / count;
    // printf("Effektiv: %d\n",voltage_eff);

    Analogsensoren[sensornum].value = (voltage_eff * Analogsensoren[sensornum].umrechnung_faktor + Analogsensoren[sensornum].umrechnung_offset) * Analogsensoren[sensornum].umrechnung_einheit;
    printf("=> AC Analogsensor: %s, Average/Bias=%d, Effektiv/Value: %.2f \n", Analogsensoren[sensornum].name, voltage_avg, Analogsensoren[sensornum].value);
}
