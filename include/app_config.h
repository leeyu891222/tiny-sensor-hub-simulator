#pragma once

// =====================
// Hardware pins
// =====================

#define APP_BUTTON_PIN 18

#define APP_I2C_SDA_PIN 21
#define APP_I2C_SCL_PIN 22

#define APP_HOST_INT_PIN 19

// =====================
// OLED config
// =====================

#define APP_SCREEN_WIDTH 128
#define APP_SCREEN_HEIGHT 64
#define APP_OLED_RESET -1
#define APP_OLED_ADDR 0x3C

// =====================
// Sensor / feature config
// =====================

#define APP_SAMPLE_QUEUE_LEN 64
#define APP_FEATURE_QUEUE_LEN 4
#define APP_INFERENCE_QUEUE_LEN 4

// 50 samples = 1 second if sampling rate is 50 Hz.
#define APP_FEATURE_WINDOW_SIZE 50

// =====================
// Sampling period
// =====================

// NORMAL / DEBUG mode: 50 Hz
#define APP_SENSOR_PERIOD_NORMAL_MS 20

// LOW_POWER mode: 10 Hz
#define APP_SENSOR_PERIOD_LOW_POWER_MS 100

// =====================
// Task timing
// =====================

#define APP_HOST_COMM_PERIOD_MS 20
#define APP_DISPLAY_PERIOD_MS 500

#define APP_BUTTON_DEBOUNCE_PRESS_MS 30
#define APP_BUTTON_DEBOUNCE_RELEASE_MS 30
#define APP_BUTTON_RELEASE_POLL_MS 10

#define APP_HOST_INT_PULSE_MS 50

// =====================
// Task stack sizes
// =====================

#define APP_STACK_BUTTON_TASK 2048
#define APP_STACK_HOST_COMM_TASK 4096
#define APP_STACK_SENSOR_TASK 4096
#define APP_STACK_FEATURE_TASK 4096
#define APP_STACK_INFERENCE_TASK 4096
#define APP_STACK_EVENT_MANAGER_TASK 4096
#define APP_STACK_DISPLAY_TASK 4096

// =====================
// Task priorities
// =====================

#define APP_PRIO_BUTTON_TASK 4
#define APP_PRIO_HOST_COMM_TASK 4
#define APP_PRIO_SENSOR_TASK 3
#define APP_PRIO_FEATURE_TASK 2
#define APP_PRIO_INFERENCE_TASK 2
#define APP_PRIO_EVENT_MANAGER_TASK 2
#define APP_PRIO_DISPLAY_TASK 1

// =====================
// Host command config
// =====================

#define APP_MAX_COMMAND_LENGTH 80

// =====================
// Health monitor config
// =====================

// Additional tolerance beyond expected feature window time.
#define APP_HEALTH_INFERENCE_TIMEOUT_MARGIN_MS 2000

// Heap warning threshold.
#define APP_HEALTH_MIN_FREE_HEAP_BYTES 20000

// =====================
// Event log config
// =====================

#define APP_EVENT_LOG_SIZE 10