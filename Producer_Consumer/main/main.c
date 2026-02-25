
#include<stdio.h>
#include<stdint.h>

#include"sdkconfig.h"
#include"esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define ADC1_CHANNEL    ADC_CHANNEL_5
#define ADC2_CHANNEL    ADC_CHANNEL_8
#define ADC1_UNIT       ADC_UNIT_1
#define ADC2_UNIT       ADC_UNIT_2

#define ADC_ATTEN       ADC_ATTEN_DB_11
#define ADC_BITWIDTH    ADC_BITWIDTH_DEFAULT

QueueHandle_t adc_queue;

static adc_oneshot_unit_handle_t adc1_handle;
static adc_oneshot_unit_handle_t adc2_handle;

static const char *TAG = "ADC_DUAL";


void producer_task(void *arg)
{
    int adc_value = 0;

    while (1)
    {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC1_CHANNEL, &adc_value));

        if (xQueueSend(adc_queue, &adc_value, portMAX_DELAY))
        {
            ESP_LOGI(TAG, "Produced: %d", adc_value);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


void consumer_task(void *arg)
{
    int received_value;

    while (1)
    {
        if (xQueueReceive(adc_queue, &received_value, portMAX_DELAY))
        {
            ESP_LOGI(TAG, "Consumed: %d", received_value);
        }
    }
}


void Init_ADC(void)
{
    // Configure ADC1
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC1_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config1 = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC1_CHANNEL, &config1));

    // Configure ADC2
    adc_oneshot_unit_init_cfg_t init_config2 = {
        .unit_id = ADC2_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config2, &adc2_handle));

    adc_oneshot_chan_cfg_t config2 = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, ADC2_CHANNEL, &config2));
}
 

void app_main(void)
{
    Init_ADC();
    adc_queue = xQueueCreate(10, sizeof(int));

    xTaskCreate(producer_task, "producer", 4096, NULL, 10, NULL);
    xTaskCreate(consumer_task, "consumer", 4096, NULL, 5, NULL);

 
}