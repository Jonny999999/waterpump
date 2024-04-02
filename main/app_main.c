#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "freertos/semphr.h"
// #include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "mqtt_client.h"

#include "driver/adc.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include <inttypes.h>
#include "esp_system.h"
#include "esp_log.h"

#include "esp_adc_cal.h"

#include "sensordeklaration.h"
#include "analogsensoren.h"
#include "digitalsensoren.h"

#include "mqtt.h"
#include "wasserpumpe.h"

static const char *TAG = "MQTT_EXAMPLE";

static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;



static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}



static void wifi_init(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_LOGI(TAG, "start the WIFI SSID:[%s]", CONFIG_WIFI_SSID);
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Waiting for wifi");
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
}



void blink(const gpio_num_t gpio_pin, const uint8_t count, const uint8_t delayOn_ms, const uint8_t delayOff_ms)
{
    for (uint8_t i = 0; i < count; i++)
    {
        gpio_set_level(gpio_pin, 1);
        vTaskDelay(delayOn_ms / portTICK_PERIOD_MS);
        gpio_set_level(gpio_pin, 0);
        vTaskDelay(delayOff_ms / portTICK_PERIOD_MS);
    }
}




void init_gpios()
{

    // LED Platine (GPIO 12) als output deklarieren
    gpio_pad_select_gpio(12);
    gpio_set_direction(12, GPIO_MODE_OUTPUT);

    // stepper 1
    gpio_pad_select_gpio(15);
    gpio_set_direction(15, GPIO_MODE_OUTPUT);

    // stepper 2
    gpio_pad_select_gpio(2);
    gpio_set_direction(2, GPIO_MODE_OUTPUT);

    // Stepper 3
    gpio_pad_select_gpio(16);
    gpio_set_direction(16, GPIO_MODE_OUTPUT);

    // Stepper 4
    gpio_pad_select_gpio(4);
    gpio_set_direction(4, GPIO_MODE_OUTPUT);

    // Relais
    gpio_pad_select_gpio(13);
    gpio_set_direction(13, GPIO_MODE_OUTPUT);

    //===5V Spannungsregler (enable pin)===
    gpio_pad_select_gpio(17);
    gpio_set_direction(17, GPIO_MODE_OUTPUT);
}



//==================blink Task==============
void task_blink(void *pvParameter)
{
    while (1)
    {
        gpio_set_level(12, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(12, 1);
        vTaskDelay(30 / portTICK_PERIOD_MS);
    }
}



//===============================MQTT Task==============================
void MQTT_task(void *pvParameter)
{
    printf("\n starting mqtt task!\n");

    // siehe mqtt.c:
    mqtt_app_start();
    vTaskDelay(5000 / portTICK_PERIOD_MS); // warten bis verbunden ist

    int delay = mqtt_intervall_ms / 2;

    while (1)
    {
        // vTaskDelay(18000 / portTICK_PERIOD_MS);
        vTaskDelay(delay / portTICK_PERIOD_MS);

        // vTaskDelay(18000 / portTICK_PERIOD_MS);
        vTaskDelay(delay / portTICK_PERIOD_MS);

        if (analogsensor_anzahl > 0)
        {
            printf("\n=======MQTT publish analogsensoren...=======\n");
            mqtt_PublishAnalogsensors(Analogsensoren, analogsensor_anzahl);
        }

        if (digitalsensor_anzahl > 0)
        {
            printf("\n=======MQTT publish digitalsensoren...=======\n");
            mqtt_PublishDigitalsensors(Digitalsensoren, digitalsensor_anzahl);
            printf("=======MQTT done!=======\n\n");
        }
    }
    // esp_restart();
}



//=======================GPIO Digitaleingaenge Task======================
// static void task_digitalsensoren(void* arg){
void task_digitalsensoren(void *pvParameter)
{

    printf("Digitansensoren: INIT");
    init_digitalsensors();

    while (1)
    {
        vTaskDelay(15000 / portTICK_PERIOD_MS);
        printf("\n=======GPIO Digitaleingaenge=======\n");
        read_digitalsensors();
        printf("======digitaldone===========\n");
    }
}



//==================ADC Task==================
void task_analogsensoren(void *pvParameter)
{
    printf("\n starting adc-task\n");

    init_analogsensors();

    int delay = analog_intervall_ms; // delay aus glob variable sensordeklaration.c
    while (1)
    {
        printf("\n=======ADC=======\n");
        read_analogsensors();
        printf("=================\n");
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
}



void task_wasserpumpe(void *pvParameter)
{
    // uint32_t time_scanned=esp_log_timestamp();

    while (1)
    {
        // printf("wasserpumpeloop: druck: %f \n",wp.pressure);
        vTaskDelay(4000 / portTICK_PERIOD_MS);
        wp_ReadPressure();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        mqtt_PublishWasserpumpe(wp);
    }
}




void app_main()
{
    //==boot, loglevel
    esp_err_t esp_timer_init();
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);
    nvs_flash_init();

    //==Wlan starten
    wifi_init();
    printf("\nwaiting for wlan\n");
    vTaskDelay(1500 / portTICK_PERIOD_MS);

    //==testweise bestimmte sensortypen ignorieren
    /// analogsensor_anzahl = 0;
    /// digitalsensor_anzahl = 0;
    /// aussensensor_anzahl = 0;
    // MQTT_SubscribedTopics_anzahl = 0;

    printf("wait done, init gpios...\n");
    //===GPIO Ausgaenge (led, stepper, 5v) deklarieren===
    init_gpios();

    printf("enavle 5v regulator + relais blink\n");
    //===5V Spannungsregler aktivieren (enable pin)===
    gpio_set_level(17, 1);

    // replausblink
    gpio_set_level(13, 1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    gpio_set_level(13, 0);

    //==LED blink task==
    xTaskCreate(&task_blink, "task_blink", configMINIMAL_STACK_SIZE, NULL, 5, NULL);

    printf("IF\n");

    printf("IF\n");
    //===Digitalsensoren task===
    // Wenn mind. 1 Sensor deklariert ist
    if (digitalsensor_anzahl > 0)
    {
        xTaskCreate(task_digitalsensoren, "task_digitalsensoren", 2048 * 3, NULL, 10, NULL);
    }

    printf("IF\n");
    //===Analogsensoren task===
    if (analogsensor_anzahl > 0)
    {
        xTaskCreate(&task_analogsensoren, "task_analogsensoren", 1024 * 3, NULL, 5, NULL);
    }

    xTaskCreate(&task_wasserpumpe, "task_wasserpumpe", 1024 * 3, NULL, 5, NULL);

    //===MQTT Task===
    // warten bis erste werte eingelesen wurden
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    xTaskCreate(&MQTT_task, "MQTT_task", 1024 * 5, NULL, 5, NULL);

    // Zur Information alle 10s freien Arbeitsspeicher ausgeben
    // while(1){
    // ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    // vTaskDelay(20000 / portTICK_PERIOD_MS);
    // }

    // mqtt_tests();
}
