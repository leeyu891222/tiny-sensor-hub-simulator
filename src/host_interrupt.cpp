#include "host_interrupt.h"

#include "app_config.h"

static TaskHandle_t g_hostInterruptTaskHandle = nullptr;

bool HostInterrupt_Init() {
  pinMode(APP_HOST_INT_PIN, OUTPUT);
  digitalWrite(APP_HOST_INT_PIN, LOW);

  return true;
}

void HostInterrupt_Trigger() {
  if (g_hostInterruptTaskHandle != nullptr) {
    xTaskNotifyGive(g_hostInterruptTaskHandle);
  }
}

void HostInterruptTask(void* parameter) {
  g_hostInterruptTaskHandle = xTaskGetCurrentTaskHandle();

  while (true) {
    // Wait until EventManager triggers a host interrupt.
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    digitalWrite(APP_HOST_INT_PIN, HIGH);
    vTaskDelay(pdMS_TO_TICKS(APP_HOST_INT_PULSE_MS));
    digitalWrite(APP_HOST_INT_PIN, LOW);
  }
}