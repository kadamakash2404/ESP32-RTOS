/**
 * @file main.c
 * @brief FreeRTOS Event Group based Button & LED Control Example
 *
 * This application demonstrates:
 * - GPIO interrupt handling
 * - FreeRTOS Event Groups for inter-task signaling
 * - Multi-task LED control architecture
 * - Mutex-protected shared configuration
 *
 * Target MCU  : ESP32  
 * Framework   : ESP-IDF  
 * RTOS        : FreeRTOS  
 *
 * @author Akash
 * @date 2026
 */


/*==================== Standard C Libraries ====================*/
#include<stdio.h>
#include<stdlib.h>

/*==================== ESP-IDF core ====================*/
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"

/*==================== FreeRTOS ====================*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*==================== Drivers ====================*/
#include "driver/gpio.h"




/**
 * @defgroup GPIO_PINS GPIO Pin Definitions
 * @brief Hardware mapping for buttons and LEDs
 * @{
 */

/** @brief GPIO connected to Push Button A */
#define Push_BtnA_Pin  36

/** @brief GPIO connected to Push Button B */
#define Push_BtnB_Pin  39

/** @brief GPIO connected to Push Button C */
#define Push_BtnC_Pin  34

/** @brief GPIO connected to Push Button D */
#define Push_BtnD_Pin  35

/** @brief GPIO connected to RED LED */
#define RED_Led_Pin    26

/** @brief GPIO connected to GREEN LED */
#define GREEN_Led_Pin  27

/** @} */



/**
 * @brief Button mask to pass to the GPIO confic structure
 */
#define ButtonMask      ((1ULL<<Push_BtnA_Pin) | (1ULL<<Push_BtnB_Pin) | (1ULL<<Push_BtnC_Pin) | (1ULL<<Push_BtnD_Pin))


/**
 * @brief Event Group bit mapping for button events
 *
 * Push_BtnA_Pin -> BIT_INPUT1  
 * Push_BtnB_Pin -> BIT_INPUT2  
 * Push_BtnC_Pin -> BIT_INPUT3  
 * Push_BtnD_Pin -> BIT_INPUT4
 */
#define BIT_INPUT1 BIT0
#define BIT_INPUT2 BIT1
#define BIT_INPUT3 BIT2
#define BIT_INPUT4 BIT3

/**
 * @brief FreeRTOS handle to initalize event group for button inputs
 *  type is static so the scope is limited to this file only
 */
static EventGroupHandle_t gpio_event_group;

/**
 * @brief FreeRTOS task handle for red led task
 * type is static so the scope is limited to this file only
 */
static TaskHandle_t RedLEDControlTaskHandle = NULL;

/**
 * @brief FreeRTOS task handle for green led 
 * type is static so the scope is limited to this file only
 */
static TaskHandle_t GreenLEDControlTaskHandle = NULL;

/**
 * @brief FreeRTOS task handle for button task
 */
static TaskHandle_t BTNControlTaskHandle = NULL;

/**
 * @brief Semaphore declaration for led configurations
 */
static SemaphoreHandle_t config_mutex;

/**
 * @enum led_mode_t
 * @brief Defines different LED operating modes
 */
typedef enum 
{
    LED_MODE_OFF,
    LED_MODE_ON,
    LED_MODE_BLINK,
    LED_MODE_ALTER
}led_mode_t;

/**
 * @struct led_control_t
 * @brief Configuration structure for individual LED
 */
typedef struct 
{
    led_mode_t mode;
    uint32_t periond_ms;
}led_control_t;

/**
 * @struct system_led_config_t
 * @brief Holds configuration for both RED and GREEN LEDs
 */
typedef struct
{
    led_control_t RED_led;
    led_control_t GREEN_led;
}system_led_config_t;



static system_led_config_t current = {
    .RED_led = {
        .mode = LED_MODE_OFF,
        .periond_ms = 1000
    },
    .GREEN_led = {
        .mode = LED_MODE_OFF,
        .periond_ms = 1000
    }
    
};

/**
 * @brief GPIO Interrupt Service Routine
 *
 * Triggered on button positive edge.
 * Sets corresponding event bit in Event Group.
 *
 * @param arg GPIO number passed during ISR registration
 *
 * @note ISR runs in IRAM context
 */
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

    if (xHigherPriorityTaskWoken) 
    {
        portYIELD_FROM_ISR();
    }
}


/**
 * @brief Button Processing Task
 *
 * Waits for event bits from ISR and updates
 * shared LED configuration accordingly.
 *
 * Uses mutex to protect shared structure.
 *
 * @param arg Unused
 */
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
            current.GREEN_led.mode = LED_MODE_BLINK;
            current.GREEN_led.periond_ms = 1000;
        }

        if(bits & BIT_INPUT2)
        {
            ESP_LOGI(TAG, "Button 2 Pressed");
            current.RED_led.mode = LED_MODE_BLINK;
            current.RED_led.periond_ms = 1000;
            current.GREEN_led.mode = LED_MODE_ON;
        }

        if(bits & BIT_INPUT3)
        {
            ESP_LOGI(TAG, "Button 3 Pressed");
            current.RED_led.mode = LED_MODE_BLINK;
            current.RED_led.periond_ms= 1000;
            current.GREEN_led.mode = LED_MODE_BLINK;
            current.GREEN_led.periond_ms = 200;
            
        }

        if(bits & BIT_INPUT4)
        {
            ESP_LOGI(TAG, "Button 4 Pressed");
            current.RED_led.mode = LED_MODE_BLINK;
            current.RED_led.periond_ms = 500;
            current.GREEN_led.mode = LED_MODE_BLINK;
            current.GREEN_led.periond_ms = 500;
            current.RED_led.mode = LED_MODE_ALTER;
            current.GREEN_led.mode = LED_MODE_ALTER;
        }
        xSemaphoreGive(config_mutex);
    }
}


/**
 * @brief RED LED Control Task
 *
 * Periodically checks shared configuration
 * and drives RED LED according to selected mode.
 *
 * Supports:
 * - OFF
 * - ON
 * - BLINK
 * - ALTERNATE
 *
 * @param arg Unused
 */
void RED_LED_Control(void* arg)
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

            case LED_MODE_ALTER:
                toggle = !toggle;
                gpio_set_level(RED_Led_Pin,toggle);
                gpio_set_level(GREEN_Led_Pin,!toggle);
                vTaskDelay(pdMS_TO_TICKS(copy.RED_led.periond_ms/2));

            default:
                vTaskDelay(pdMS_TO_TICKS(50));
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}


/**
 * @brief GREEN LED Control Task
 *
 * Controls GREEN LED behavior based on
 * current system configuration.
 *
 * @param arg Unused
 */
void GREEN_LED_Control(void* arg)
{
     system_led_config_t copy;
    uint8_t toggle = 0;

    while(1)
    {
        xSemaphoreTake(config_mutex,portMAX_DELAY);
        copy = current;
        xSemaphoreGive(config_mutex);

        switch(copy.GREEN_led.mode)
        {
            case LED_MODE_OFF:
                gpio_set_level(GREEN_Led_Pin,0);
                vTaskDelay(pdMS_TO_TICKS(50));
                break;

            case LED_MODE_ON:
                gpio_set_level(GREEN_Led_Pin,1);
                vTaskDelay(pdMS_TO_TICKS(50));
                break;

            case LED_MODE_BLINK:
                toggle = !toggle;
                gpio_set_level(GREEN_Led_Pin,toggle);
                vTaskDelay(pdMS_TO_TICKS(copy.GREEN_led.periond_ms/2));
                break;

            case LED_MODE_ALTER:
            {
                vTaskDelay(pdMS_TO_TICKS(50));
                break;
            }

            default:
                // vTaskDelay(pdMS_TO_TICKS(50));
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}


/**
 * @brief Initializes GPIOs, ISR service and synchronization primitives
 *
 * - Configures LED pins as output
 * - Configures Button pins as input with interrupt
 * - Installs ISR service
 * - Creates mutex
 */
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

/**
 * @brief Application entry point
 *
 * Creates:
 * - Event group
 * - Peripheral configuration
 * - Button task
 * - RED LED task
 * - GREEN LED task
 *
 * Task Priority:
 * - Button Task : 6
 * - LED Tasks   : 5
 */
void app_main()
{
    gpio_event_group = xEventGroupCreate();

    Init_Peripherales();

    xTaskCreate(BTN_Control,"BTN control task",4096,NULL,6,&BTNControlTaskHandle);
    xTaskCreate(RED_LED_Control," RED LED control task",4096,NULL,5,&RedLEDControlTaskHandle);
    xTaskCreate(GREEN_LED_Control," GREEN LED control task",4096,NULL,5,&GreenLEDControlTaskHandle);

}