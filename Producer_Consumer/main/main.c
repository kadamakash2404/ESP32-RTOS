
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
#include "driver/ledc.h"
#include "driver/gpio.h"

#define LED_GPIO        26
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_MODE       LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL    LEDC_CHANNEL_0
#define LEDC_DUTY_RES   LEDC_TIMER_12_BIT   // 8-bit resolution
#define LEDC_FREQUENCY  5000               // 5 kHz


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
            ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, received_value);
            ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
            ESP_LOGI(TAG, "Consumed: %d", received_value);
        }
    }
}



void Init_Peripherals(void)
{

    // Configure LEDC Timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Configure LEDC Channel
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = LED_GPIO,
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER,
        .duty           = 0, // Start with LED off
        .hpoint         = 0
    };

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));



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
    Init_Peripherals();
    adc_queue = xQueueCreate(10, sizeof(int));

    xTaskCreate(producer_task, "producer", 4096, NULL, 10, NULL);
    xTaskCreate(consumer_task, "consumer", 4096, NULL, 5, NULL);

 
}