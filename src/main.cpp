#include <Arduino.h>

#include "app_config.h"
#include "app_types.h"
#include "sensor_sim.h"
#include "feature_extractor.h"
#include "classifier_rule.h"
#include "system_state.h"
#include "host_protocol.h"
#include "display_service.h"
#include "event_manager.h"





// =====================
// RTOS objects
// =====================
TaskHandle_t buttonTaskHandle = nullptr;


QueueHandle_t sampleQueue = nullptr;
QueueHandle_t featureQueue = nullptr;
QueueHandle_t inferenceQueue = nullptr;




// =====================
// ISR
// =====================
void IRAM_ATTR buttonISR() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if (buttonTaskHandle != nullptr) {
    vTaskNotifyGiveFromISR(buttonTaskHandle, &xHigherPriorityTaskWoken);
  }

  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

// =====================
// Tasks
// =====================
void ButtonTask(void* parameter) {
  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    vTaskDelay(pdMS_TO_TICKS(APP_BUTTON_DEBOUNCE_PRESS_MS));

    if (digitalRead(APP_BUTTON_PIN) == LOW) {
      SystemMode newMode = SystemState_SwitchMode();

      Serial.print("[ButtonTask] Mode changed to: ");
      Serial.println(modeToString(newMode));

      while (digitalRead(APP_BUTTON_PIN) == LOW) {
        vTaskDelay(pdMS_TO_TICKS(APP_BUTTON_RELEASE_POLL_MS));
      }

      vTaskDelay(pdMS_TO_TICKS(APP_BUTTON_DEBOUNCE_RELEASE_MS));
    }
  }
}

void HostCommTask(void* parameter) {
  String line;

  HostProtocol_PrintHelp();

  while (true) {
    while (Serial.available() > 0) {
      char c = Serial.read();

      if (c == '\n' || c == '\r') {
        if (line.length() > 0) {
          HostProtocol_ProcessLine(line);
          line = "";
        }
      } else {
        line += c;

        if (line.length() > APP_MAX_COMMAND_LENGTH) {
          Serial.println("ERR Command too long");
          line = "";
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(APP_HOST_COMM_PERIOD_MS));
  }
}

void SensorTask(void* parameter) {
  while (true) {
    SensorPattern pattern = SystemState_GetPattern();
    SensorSample sample = SensorSim_ReadSample(pattern);

    BaseType_t ok = xQueueSend(sampleQueue, &sample, 0);

    if (ok == pdTRUE) {
      SystemState_IncrementSamplesGenerated();
    } else {
      SystemState_IncrementSampleQueueDrops();
    }

    SystemMode mode = SystemState_GetMode();

    if (mode == MODE_LOW_POWER) {
      vTaskDelay(pdMS_TO_TICKS(APP_SENSOR_PERIOD_LOW_POWER_MS));
    } else {
      vTaskDelay(pdMS_TO_TICKS(APP_SENSOR_PERIOD_NORMAL_MS));
    }
  }
}

void FeatureTask(void* parameter) {
  SensorSample window[APP_FEATURE_WINDOW_SIZE];
  size_t count = 0;

  while (true) {
    SensorSample sample;

    if (xQueueReceive(sampleQueue, &sample, portMAX_DELAY) == pdTRUE) {
      window[count++] = sample;

      if (count >= APP_FEATURE_WINDOW_SIZE) {
        FeatureWindow feature = FeatureExtractor_Compute(window, APP_FEATURE_WINDOW_SIZE);
        SystemState_UpdateLastFeature(feature);

        xQueueSend(featureQueue, &feature, portMAX_DELAY);

        count = 0;
      }
    }
  }
}

void InferenceTask(void* parameter) {
  while (true) {
    FeatureWindow feature;

    if (xQueueReceive(featureQueue, &feature, portMAX_DELAY) == pdTRUE) {
      InferenceResult result = ClassifierRule_Run(feature);
      SystemState_UpdateLastInference(result);

      xQueueSend(inferenceQueue, &result, portMAX_DELAY);
    }
  }
}

void EventManagerTask(void* parameter) {
  while (true) {
    InferenceResult result;

    if (xQueueReceive(inferenceQueue, &result, portMAX_DELAY) == pdTRUE) {
      EventManager_ProcessInference(result);
    }
  }
}

void DisplayTask(void* parameter) {
  while (true) {
    DisplayService_Update();
    vTaskDelay(pdMS_TO_TICKS(APP_DISPLAY_PERIOD_MS));
  }
}

// =====================
// Arduino setup / loop
// =====================
void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println("Booting TinyML Sensor Hub Simulator...");

  pinMode(APP_BUTTON_PIN, INPUT_PULLUP);

  bool stateOk = SystemState_Init();
  SensorSim_Init();
  EventManager_Init();
  bool displayOk = DisplayService_Init();

  sampleQueue = xQueueCreate(APP_SAMPLE_QUEUE_LEN, sizeof(SensorSample));
  featureQueue = xQueueCreate(APP_FEATURE_QUEUE_LEN, sizeof(FeatureWindow));
  inferenceQueue = xQueueCreate(APP_INFERENCE_QUEUE_LEN, sizeof(InferenceResult));

  if (!stateOk || !displayOk ||
      sampleQueue == nullptr || featureQueue == nullptr || inferenceQueue == nullptr) {
    Serial.println("Failed to create RTOS objects");
    while (true) {
      delay(1000);
    }
  }

  DisplayService_Update();

  xTaskCreatePinnedToCore(
    ButtonTask,
    "ButtonTask",
    APP_STACK_BUTTON_TASK,
    nullptr,
    APP_PRIO_BUTTON_TASK,
    &buttonTaskHandle,
    1
  );
  xTaskCreatePinnedToCore(
    HostCommTask,
    "HostCommTask",
    APP_STACK_HOST_COMM_TASK,
    nullptr,
    APP_PRIO_HOST_COMM_TASK,
    nullptr,
    1
  );

  xTaskCreatePinnedToCore(
    SensorTask,
    "SensorTask",
    APP_STACK_SENSOR_TASK,
    nullptr,
    APP_PRIO_SENSOR_TASK,
    nullptr,
    1
  );

  xTaskCreatePinnedToCore(
    FeatureTask,
    "FeatureTask",
    APP_STACK_FEATURE_TASK,
    nullptr,
    APP_PRIO_FEATURE_TASK,
    nullptr,
    1
  );

  xTaskCreatePinnedToCore(
    InferenceTask,
    "InferenceTask",
    APP_STACK_INFERENCE_TASK,
    nullptr,
    APP_PRIO_INFERENCE_TASK,
    nullptr,
    1
  );

  xTaskCreatePinnedToCore(
    EventManagerTask,
    "EventManagerTask",
    APP_STACK_EVENT_MANAGER_TASK,
    nullptr,
    APP_PRIO_EVENT_MANAGER_TASK,
    nullptr,
    1
  );

  xTaskCreatePinnedToCore(
    DisplayTask,
    "DisplayTask",
    APP_STACK_DISPLAY_TASK,
    nullptr,
    APP_PRIO_DISPLAY_TASK,
    nullptr,
    1
  );

  attachInterrupt(
    digitalPinToInterrupt(APP_BUTTON_PIN),
    buttonISR,
    FALLING
  );

  Serial.println("System started.");
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}