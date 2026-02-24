# 📘 ESP32 GPIO Interrupt + FreeRTOS Task Notification Example

This project demonstrates GPIO interrupt handling and FreeRTOS task notification from ISR using the ESP-IDF on an ESP32.

It implements a push button interrupt that notifies a task, which then toggles an LED.
---
### 🚀 Features
- GPIO interrupt configuration
- ISR written with IRAM_ATTR
- Task notification from ISR (vTaskNotifyGiveFromISR)
- LED control in task context
- Proper esp_err_t error handling
- Doxygen-compatible documentation
---
### 🏗️ Project Structure
```
hello_world/
 ├── main/
 │    └── main.c
 ├── CMakeLists.txt
 ├── sdkconfig
 └── README.md
```
---
### 🔧 Hardware Setup
| Component    | GPIO |
|-------------|------|
| Push Button | GPIO 36 (Input, Rising Edge Interrupt) |
| LED         | GPIO 26 (Output) |

> ⚠️ GPIO 36 is input-only (correct for button use).
---
### 🧠 How It Works

#### 1️⃣ Peripheral Initialization
- Initalize_Peripherals() configures:
- GPIO 26 → Output (LED)
- GPIO 36 → Input with rising edge interrupt
- Installs ISR service
- Registers interrupt handler
- 
#### 2️⃣ Interrupt Service Routine
```
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(LEDTaskHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```
✔ Minimal ISR
✔ No heavy processing
✔ Only sends notification
This follows best ISR design practice.

#### 3️⃣ LED Task
```
static void LED_Task(void* arg)
{
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        BtnState = ~BtnState;
        gpio_set_level(Led_Pin, BtnState);
    }
}
```
Blocks until notified
Toggles LED
Runs in task context (safe to call driver APIs)
---
### 🔄 Execution Flow
```
Button Press
     ↓
GPIO Interrupt
     ↓
ISR sends notification
     ↓
LED Task wakes up
     ↓
LED toggles
```
This is the recommended architecture for ISR-to-task communication in FreeRTOS.
---
### 📦 Build & Flash

Make sure ESP-IDF is installed and environment is set.

idf.py build
idf.py flash
idf.py monitor
📖 Key FreeRTOS APIs Used
```
vTaskNotifyGiveFromISR()

ulTaskNotifyTake()

xTaskCreate()

portYIELD_FROM_ISR()
```
Task notifications are faster and lighter than queues for ISR signaling.
---
### 🛠️ Important Implementation Details
✔ 64-bit GPIO Mask
```
GPIO_config.pin_bit_mask = 1ULL << Led_Pin;
```

pin_bit_mask is uint64_t, so 1ULL must be used.

✔ Volatile Shared Variable
```
static volatile int BtnState;
```

Used because it is modified in ISR-triggered flow.

✔ Error Handling
```
ESP_ERROR_CHECK(Initalize_Peripherals());
```
Ensures system halts on critical failure.
---
### 📚 Generating Documentation (Doxygen)

If Doxygen is installed:

doxygen -g
# Configure Doxyfile (INPUT = main)
```
doxygen Doxyfile
```

Open:
```
html/index.html
```
---
### 🧪 Future Improvements

- [ ] Add software debounce
- [ ] Use gpio_get_level() instead of toggling blindly
- [ ] Replace global state with safer abstraction
- [ ] Convert into reusable GPIO driver component
- [ ] Add logging using ESP_LOGI
---
### 🎯 Learning Goals Covered

- GPIO driver usage
- Interrupt handling on ESP32
- FreeRTOS task notifications
- ISR-safe API usage
- Embedded firmware documentation practice
---
## 👨‍💻 Author
**Akash Kadam**
*Softwar Developer*









