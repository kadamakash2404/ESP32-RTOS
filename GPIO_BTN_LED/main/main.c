
/**
 * @file main.c
 * @brief GPIO interrupt and LED control example
 *
 * This example demonstrates:
 * - GPIO interrupt handling
 * - FreeRTOS task notification from ISR
 * - LED toggling using task context
 */

#include<stdio.h>
#include<stdint.h>

#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#define ESP_INTR_FLAG_DEFAULT 0

#define Push_Btn_Pin  36
#define Led_Pin  26

/**
 * @brief Handle for LED task
 */
TaskHandle_t LEDTaskHandle = NULL;

/**
 * @brief Current button state (modified in ISR context)
 */
static volatile int BtnState;

/**
 * @brief GPIO interrupt service routine
 *
 * This ISR sends a task notification to LED task
 * when the push button interrupt occurs.
 *
 * @param arg Pointer to GPIO number (passed during ISR registration)
 */

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(LEDTaskHandle,&xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


/**
 * @brief Initialize GPIO peripherals
 *
 * Configures:
 * - Push button GPIO as input with interrupt
 * - LED GPIO as output
 *
 * @return esp_err_t
 *         - ESP_OK on success
 *         - Error code otherwise
 */
esp_err_t Initalize_Peripherals()
{
    esp_err_t  ret;
    gpio_config_t GPIO_config = {};

    GPIO_config.pin_bit_mask = 1ULL << Led_Pin;
    GPIO_config.mode = GPIO_MODE_OUTPUT;
    GPIO_config.pull_up_en = GPIO_PULLUP_DISABLE;
    GPIO_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    GPIO_config.intr_type = GPIO_INTR_DISABLE;
    ret = gpio_config(&GPIO_config); 

    GPIO_config.pin_bit_mask = 1ULL << Push_Btn_Pin;
    GPIO_config.mode = GPIO_MODE_INPUT;
    GPIO_config.pull_up_en = GPIO_PULLUP_DISABLE;
    GPIO_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    GPIO_config.intr_type = GPIO_INTR_POSEDGE;
    ret = gpio_config(&GPIO_config); 

    ret = gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    ret = gpio_isr_handler_add(Push_Btn_Pin, gpio_isr_handler, (void*) Push_Btn_Pin);
    return ret;
}


/**
 * @brief LED control task
 *
 * Waits for notification from ISR.
 * Toggles LED and updates button state.
 *
 * @param arg Task parameter (unused)
 */
static void LED_Task(void* arg)
{
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        BtnState = ~BtnState;
        gpio_set_level(Led_Pin,BtnState);
    }
}

/**
 * @brief Application entry point
 *
 * Creates LED task and initializes peripherals.
 */
void app_main(void)
{
    esp_err_t  ret;

    ESP_ERROR_CHECK(Initalize_Peripherals());
    
    
    xTaskCreate(LED_Task, "LED task", 512, NULL, 5, &LEDTaskHandle);
}