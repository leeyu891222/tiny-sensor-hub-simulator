#include "event_log.h"

#include <Arduino.h>
#include <string.h>

#include "app_config.h"

static EventRecord g_eventLog[APP_EVENT_LOG_SIZE];
static size_t g_head = 0;   // next write position
static size_t g_count = 0;  // number of valid records

static SemaphoreHandle_t g_eventLogMutex = nullptr;

void EventLog_Init() {
  memset(g_eventLog, 0, sizeof(g_eventLog));
  g_head = 0;
  g_count = 0;

  g_eventLogMutex = xSemaphoreCreateMutex();
}

void EventLog_Add(const EventRecord& record) {
  if (g_eventLogMutex == nullptr) {
    return;
  }

  xSemaphoreTake(g_eventLogMutex, portMAX_DELAY);

  g_eventLog[g_head] = record;
  g_head = (g_head + 1) % APP_EVENT_LOG_SIZE;

  if (g_count < APP_EVENT_LOG_SIZE) {
    g_count++;
  }

  xSemaphoreGive(g_eventLogMutex);
}

void EventLog_Clear() {
  if (g_eventLogMutex == nullptr) {
    return;
  }

  xSemaphoreTake(g_eventLogMutex, portMAX_DELAY);

  memset(g_eventLog, 0, sizeof(g_eventLog));
  g_head = 0;
  g_count = 0;

  xSemaphoreGive(g_eventLogMutex);
}

size_t EventLog_Count() {
  if (g_eventLogMutex == nullptr) {
    return 0;
  }

  xSemaphoreTake(g_eventLogMutex, portMAX_DELAY);
  size_t count = g_count;
  xSemaphoreGive(g_eventLogMutex);

  return count;
}

void EventLog_Print() {
  if (g_eventLogMutex == nullptr) {
    Serial.println("ERR EventLog not initialized");
    return;
  }

  xSemaphoreTake(g_eventLogMutex, portMAX_DELAY);

  Serial.println();
  Serial.println("===== EVENT LOG =====");

  Serial.print("count=");
  Serial.print(g_count);
  Serial.print(" size=");
  Serial.println(APP_EVENT_LOG_SIZE);

  if (g_count == 0) {
    Serial.println("(empty)");
    Serial.println("=====================");
    Serial.println();

    xSemaphoreGive(g_eventLogMutex);
    return;
  }

  // Oldest record index:
  // If buffer not full, oldest is 0.
  // If full, oldest is g_head because g_head points to the next overwritten slot.
  size_t start = (g_count < APP_EVENT_LOG_SIZE) ? 0 : g_head;

  for (size_t i = 0; i < g_count; i++) {
    size_t idx = (start + i) % APP_EVENT_LOG_SIZE;
    const EventRecord& r = g_eventLog[idx];

    Serial.print("[");
    Serial.print(i);
    Serial.print("] t=");
    Serial.print(r.timestampMs);

    Serial.print(" reason=");
    Serial.print(eventReasonToString(r.reason));

    Serial.print(" ");
    Serial.print(motionToString(r.previousState));
    Serial.print("->");
    Serial.print(motionToString(r.newState));

    Serial.print(" conf=");
    Serial.print(r.confidence, 2);

    Serial.print(" energy=");
    Serial.print(r.energy, 2);

    Serial.print(" max=");
    Serial.println(r.maxAbs, 2);
  }

  Serial.println("=====================");
  Serial.println();

  xSemaphoreGive(g_eventLogMutex);
}