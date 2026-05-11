#include "system_state.h"

#include <string.h>

static SemaphoreHandle_t g_stateMutex = nullptr;

static SystemMode g_currentMode = MODE_NORMAL;
static SensorPattern g_currentPattern = PATTERN_STILL;
static MotionState g_currentMotionState = MOTION_STILL;

static FeatureWindow g_lastFeature = {};
static InferenceResult g_lastInference = {};
static SystemStats g_stats = {};

bool SystemState_Init() {
  g_stateMutex = xSemaphoreCreateMutex();

  if (g_stateMutex == nullptr) {
    return false;
  }

  g_currentMode = MODE_NORMAL;
  g_currentPattern = PATTERN_STILL;
  g_currentMotionState = MOTION_STILL;

  memset(&g_lastFeature, 0, sizeof(g_lastFeature));
  memset(&g_lastInference, 0, sizeof(g_lastInference));
  memset(&g_stats, 0, sizeof(g_stats));

  return true;
}

SystemMode SystemState_GetMode() {
  xSemaphoreTake(g_stateMutex, portMAX_DELAY);
  SystemMode mode = g_currentMode;
  xSemaphoreGive(g_stateMutex);
  return mode;
}

SensorPattern SystemState_GetPattern() {
  xSemaphoreTake(g_stateMutex, portMAX_DELAY);
  SensorPattern pattern = g_currentPattern;
  xSemaphoreGive(g_stateMutex);
  return pattern;
}

MotionState SystemState_GetMotionState() {
  xSemaphoreTake(g_stateMutex, portMAX_DELAY);
  MotionState state = g_currentMotionState;
  xSemaphoreGive(g_stateMutex);
  return state;
}

void SystemState_SetMode(SystemMode mode) {
  xSemaphoreTake(g_stateMutex, portMAX_DELAY);
  g_currentMode = mode;
  xSemaphoreGive(g_stateMutex);
}

void SystemState_SetPattern(SensorPattern pattern) {
  xSemaphoreTake(g_stateMutex, portMAX_DELAY);
  g_currentPattern = pattern;
  xSemaphoreGive(g_stateMutex);
}

void SystemState_SetMotionState(MotionState state) {
  xSemaphoreTake(g_stateMutex, portMAX_DELAY);
  g_currentMotionState = state;
  xSemaphoreGive(g_stateMutex);
}

SystemMode SystemState_SwitchMode() {
  xSemaphoreTake(g_stateMutex, portMAX_DELAY);

  if (g_currentMode == MODE_NORMAL) {
    g_currentMode = MODE_DEBUG;
  } else if (g_currentMode == MODE_DEBUG) {
    g_currentMode = MODE_LOW_POWER;
  } else {
    g_currentMode = MODE_NORMAL;
  }

  SystemMode newMode = g_currentMode;

  xSemaphoreGive(g_stateMutex);

  return newMode;
}

void SystemState_UpdateLastFeature(const FeatureWindow& feature) {
  xSemaphoreTake(g_stateMutex, portMAX_DELAY);
  g_lastFeature = feature;
  g_stats.featuresComputed++;
  xSemaphoreGive(g_stateMutex);
}

void SystemState_UpdateLastInference(const InferenceResult& result) {
  xSemaphoreTake(g_stateMutex, portMAX_DELAY);
  g_lastInference = result;
  g_stats.inferencesRun++;
  xSemaphoreGive(g_stateMutex);
}

FeatureWindow SystemState_GetLastFeature() {
  xSemaphoreTake(g_stateMutex, portMAX_DELAY);
  FeatureWindow copy = g_lastFeature;
  xSemaphoreGive(g_stateMutex);
  return copy;
}

InferenceResult SystemState_GetLastInference() {
  xSemaphoreTake(g_stateMutex, portMAX_DELAY);
  InferenceResult copy = g_lastInference;
  xSemaphoreGive(g_stateMutex);
  return copy;
}

SystemStats SystemState_GetStats() {
  xSemaphoreTake(g_stateMutex, portMAX_DELAY);
  SystemStats copy = g_stats;
  xSemaphoreGive(g_stateMutex);
  return copy;
}

void SystemState_ResetStats() {
  xSemaphoreTake(g_stateMutex, portMAX_DELAY);
  memset(&g_stats, 0, sizeof(g_stats));
  xSemaphoreGive(g_stateMutex);
}

void SystemState_IncrementSamplesGenerated() {
  xSemaphoreTake(g_stateMutex, portMAX_DELAY);
  g_stats.samplesGenerated++;
  xSemaphoreGive(g_stateMutex);
}

void SystemState_IncrementFeaturesComputed() {
  xSemaphoreTake(g_stateMutex, portMAX_DELAY);
  g_stats.featuresComputed++;
  xSemaphoreGive(g_stateMutex);
}

void SystemState_IncrementInferencesRun() {
  xSemaphoreTake(g_stateMutex, portMAX_DELAY);
  g_stats.inferencesRun++;
  xSemaphoreGive(g_stateMutex);
}

void SystemState_IncrementHostNotifications() {
  xSemaphoreTake(g_stateMutex, portMAX_DELAY);
  g_stats.hostNotifications++;
  xSemaphoreGive(g_stateMutex);
}

void SystemState_IncrementSuppressedNotifications() {
  xSemaphoreTake(g_stateMutex, portMAX_DELAY);
  g_stats.suppressedNotifications++;
  xSemaphoreGive(g_stateMutex);
}

void SystemState_IncrementSampleQueueDrops() {
  xSemaphoreTake(g_stateMutex, portMAX_DELAY);
  g_stats.sampleQueueDrops++;
  xSemaphoreGive(g_stateMutex);
}