#include<stdio.h>
#include<stdlib.h>

#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"


#define Push_BtnA_Pin  36
#define Push_BtnB_Pin  39
#define Push_BtnC_Pin  34
#define Push_BtnD_Pin  35

#define ButtonMask      ((1ULL<<Push_BtnA_Pin) | (1ULL<<Push_BtnB_Pin) | (1ULL<<Push_BtnC_Pin) | (1ULL<<Push_BtnD_Pin))

#define RED_Led_Pin  26
#define GREEN_Led_Pin 27


#define BIT_INPUT1 BIT0
#define BIT_INPUT2 BIT1
#define BIT_INPUT3 BIT2
#define BIT_INPUT4 BIT3

static EventGroupHandle_t gpio_event_group;

static TaskHandle_t LEDControlTaskHandle = NULL;
static TaskHandle_t BTNControlTaskHandle = NULL;

static SemaphoreHandle_t config_mutex;


typedef enum 
{
    LED_MODE_OFF,
    LED_MODE_ON,
    LED_MODE_BLINK,
    LED_MODE_ALTER
}led_mode_t;

typedef struct 
{
    led_mode_t mode;
    uint32_t periond_ms;
}led_control_t;


typedef struct
{
    led_control_t RED_led;
    // led_control_t GREEN_led;
}system_led_config_t;



static system_led_config_t current = {
    .RED_led = {
        .mode = LED_MODE_OFF,
        .periond_ms = 1000
    }
};


static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    uint32_t Btn_Num = (uint32_t) arg;
    switch (Btn_Num)
    {
        case Push_BtnA_Pin:
        {
            xEventGroupSetBitsFromISR(gpio_event_group,BIT_INPUT1,&xHigherPriorityTaskWoken);
            break;
        }
        case Push_BtnB_Pin:
        {
            xEventGroupSetBitsFromISR(gpio_event_group,BIT_INPUT2,&xHigherPriorityTaskWoken);
            break;
        }
        case Push_BtnC_Pin:
        {
            xEventGroupSetBitsFromISR(gpio_event_group,BIT_INPUT3,&xHigherPriorityTaskWoken);
            break;
        }
        case Push_BtnD_Pin:
        {
            xEventGroupSetBitsFromISR(gpio_event_group,BIT_INPUT4,&xHigherPriorityTaskWoken);
            break;
        }
    }

    if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
}
}

void BTN_Control(void* arg)
{
    const char* TAG = "BTN_Control";

    while(1)
    {
        EventBits_t bits = xEventGroupWaitBits(gpio_event_group,
                                                BIT_INPUT1 | BIT_INPUT2 | BIT_INPUT3 | BIT_INPUT4,
                                                pdTRUE,
                                                pdFALSE,
                                                portMAX_DELAY);

        xSemaphoreTake(config_mutex,portMAX_DELAY);

        if(bits & BIT_INPUT1)
        {
            ESP_LOGI(TAG, "Button 1 Pressed");
            current.RED_led.mode = LED_MODE_ON;
            // current.GREEN_led.mode = LED_MODE_BLINK;
            // current.GREEN_led.periond_ms = 1000;
        }

        if(bits & BIT_INPUT2)
        {
            ESP_LOGI(TAG, "Button 2 Pressed");
            current.RED_led.mode = LED_MODE_BLINK;
            current.RED_led.periond_ms = 1000;
            // current.GREEN_led.mode = LED_MODE_OFF;
        }

        if(bits & BIT_INPUT3)
        {
            ESP_LOGI(TAG, "Button 3 Pressed");
            current.RED_led.mode = LED_MODE_BLINK;
            // current.GREEN_led.mode = LED_MODE_BLINK;
            current.RED_led.periond_ms = 500;
            
        }

        if(bits & BIT_INPUT4)
        {
            ESP_LOGI(TAG, "Button 4 Pressed");
            current.RED_led.mode = LED_MODE_OFF;
            // current.RED_led.periond_ms = 500;
            // current.GREEN_led.mode = LED_MODE_BLINK;
            // current.GREEN_led.periond_ms = 500;
        }
        xSemaphoreGive(config_mutex);
    }
}

void LED_Control(void* arg)
{
    system_led_config_t copy;
    uint8_t toggle = 0;

    while(1)
    {
        xSemaphoreTake(config_mutex,portMAX_DELAY);
        copy = current;
        xSemaphoreGive(config_mutex);

        switch(copy.RED_led.mode)
        {
            case LED_MODE_OFF:
                gpio_set_level(RED_Led_Pin,0);
                vTaskDelay(pdMS_TO_TICKS(50));
                break;

            case LED_MODE_ON:
                gpio_set_level(RED_Led_Pin,1);
                vTaskDelay(pdMS_TO_TICKS(50));
                break;

            case LED_MODE_BLINK:
                toggle = !toggle;
                gpio_set_level(RED_Led_Pin,toggle);
                vTaskDelay(pdMS_TO_TICKS(copy.RED_led.periond_ms/2));
                break;

            default:
                vTaskDelay(pdMS_TO_TICKS(50));
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}


void Init_Peripherales(void)
{
    config_mutex = xSemaphoreCreateMutex();

    gpio_config_t GPIO_config = {};

    GPIO_config.pin_bit_mask = ((1ULL << RED_Led_Pin) | (1ULL << GREEN_Led_Pin));
    GPIO_config.mode = GPIO_MODE_OUTPUT;
    GPIO_config.pull_up_en = GPIO_PULLUP_DISABLE;
    GPIO_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    GPIO_config.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&GPIO_config)); 

    GPIO_config.pin_bit_mask = ButtonMask;
    GPIO_config.mode = GPIO_MODE_INPUT;
    GPIO_config.pull_up_en = GPIO_PULLUP_DISABLE;
    GPIO_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    GPIO_config.intr_type = GPIO_INTR_POSEDGE;
    ESP_ERROR_CHECK(gpio_config(&GPIO_config));

    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(Push_BtnA_Pin, gpio_isr_handler, (void*) Push_BtnA_Pin));
    ESP_ERROR_CHECK(gpio_isr_handler_add(Push_BtnB_Pin, gpio_isr_handler, (void*) Push_BtnB_Pin));
    ESP_ERROR_CHECK(gpio_isr_handler_add(Push_BtnC_Pin, gpio_isr_handler, (void*) Push_BtnC_Pin));
    ESP_ERROR_CHECK(gpio_isr_handler_add(Push_BtnD_Pin, gpio_isr_handler, (void*) Push_BtnD_Pin));
}


void app_main()
{
    gpio_event_group = xEventGroupCreate();

    Init_Peripherales();

    xTaskCreate(BTN_Control,"BTN control task",4096,NULL,5,&BTNControlTaskHandle);
    xTaskCreate(LED_Control,"LED control task",4096,NULL,5,&LEDControlTaskHandle);

}