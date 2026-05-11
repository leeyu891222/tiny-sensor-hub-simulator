#include "host_protocol.h"

#include <string.h>
#include <Esp.h>

#include "app_config.h"
#include "app_types.h"
#include "system_state.h"
#include "event_log.h"

static void printStatus() {
  SystemMode mode = SystemState_GetMode();
  SensorPattern pattern = SystemState_GetPattern();
  MotionState motion = SystemState_GetMotionState();
  InferenceResult inf = SystemState_GetLastInference();
  ClassifierMode classifierMode = SystemState_GetClassifierMode();

  Serial.println();
  Serial.println("===== STATUS =====");
  Serial.print("Mode: ");
  Serial.println(modeToString(mode));
  Serial.print("Classifier: ");
  Serial.println(classifierModeToString(classifierMode));
  Serial.print("Sensor Pattern: ");
  Serial.println(patternToString(pattern));
  Serial.print("Motion State: ");
  Serial.println(motionToString(motion));
  Serial.print("Confidence: ");
  Serial.println(inf.confidence, 3);
  Serial.print("Energy: ");
  Serial.println(inf.energy, 3);
  Serial.print("Max Abs: ");
  Serial.println(inf.maxAbs, 3);
  Serial.println("==================");
  Serial.println();
}

static void printStats() {
  SystemStats s = SystemState_GetStats();

  Serial.println();
  Serial.println("===== STATS =====");
  Serial.print("Samples generated: ");
  Serial.println(s.samplesGenerated);
  Serial.print("Features computed: ");
  Serial.println(s.featuresComputed);
  Serial.print("Inferences run: ");
  Serial.println(s.inferencesRun);
  Serial.print("Host notifications: ");
  Serial.println(s.hostNotifications);
  Serial.print("Suppressed notifications: ");
  Serial.println(s.suppressedNotifications);
  Serial.print("Sample queue drops: ");
  Serial.println(s.sampleQueueDrops);

  if (s.samplesGenerated > 0) {
    float notifyRate = 100.0f * (float)s.hostNotifications / (float)s.samplesGenerated;

    Serial.print("Notify/sample ratio: ");
    Serial.print(notifyRate, 3);
    Serial.println("%");

    Serial.print("Estimated wakeup reduction: ");
    Serial.print(100.0f - notifyRate, 3);
    Serial.println("%");
  }

  Serial.println("=================");
  Serial.println();
}

static void printHealth() {
  SystemMode mode = SystemState_GetMode();
  SystemStats s = SystemState_GetStats();
  InferenceResult inf = SystemState_GetLastInference();

  uint32_t now = millis();
  uint32_t uptimeMs = now;
  uint32_t freeHeap = ESP.getFreeHeap();

  bool hasInference = (s.inferencesRun > 0);
  uint32_t lastInferenceAgeMs = hasInference ? (now - inf.timestampMs) : 0;

  uint32_t expectedInferencePeriodMs;

  if (mode == MODE_LOW_POWER) {
    expectedInferencePeriodMs =
      APP_FEATURE_WINDOW_SIZE * APP_SENSOR_PERIOD_LOW_POWER_MS;
  } else {
    expectedInferencePeriodMs =
      APP_FEATURE_WINDOW_SIZE * APP_SENSOR_PERIOD_NORMAL_MS;
  }

  uint32_t inferenceTimeoutMs =
    expectedInferencePeriodMs + APP_HEALTH_INFERENCE_TIMEOUT_MARGIN_MS;

  bool heapOk = freeHeap >= APP_HEALTH_MIN_FREE_HEAP_BYTES;
  bool queueOk = (s.sampleQueueDrops == 0);
  bool inferenceOk = hasInference && (lastInferenceAgeMs <= inferenceTimeoutMs);

  bool healthOk = heapOk && queueOk && inferenceOk;

  Serial.println();
  Serial.println("===== HEALTH =====");

  Serial.print("Status: ");
  Serial.println(healthOk ? "OK" : "WARN");

  Serial.print("Uptime: ");
  Serial.print(uptimeMs);
  Serial.println(" ms");

  Serial.print("Mode: ");
  Serial.println(modeToString(mode));

  Serial.print("Free heap: ");
  Serial.print(freeHeap);
  Serial.println(" bytes");

  Serial.print("Min heap threshold: ");
  Serial.print(APP_HEALTH_MIN_FREE_HEAP_BYTES);
  Serial.println(" bytes");

  Serial.print("Inferences run: ");
  Serial.println(s.inferencesRun);

  if (hasInference) {
    Serial.print("Last inference age: ");
    Serial.print(lastInferenceAgeMs);
    Serial.println(" ms");
  } else {
    Serial.println("Last inference age: N/A");
  }

  Serial.print("Expected inference period: ");
  Serial.print(expectedInferencePeriodMs);
  Serial.println(" ms");

  Serial.print("Inference timeout: ");
  Serial.print(inferenceTimeoutMs);
  Serial.println(" ms");

  Serial.print("Sample queue drops: ");
  Serial.println(s.sampleQueueDrops);

  Serial.print("Pipeline: ");
  Serial.println(inferenceOk ? "ACTIVE" : "STALE");

  if (!healthOk) {
    Serial.println();
    Serial.println("Warnings:");

    if (!heapOk) {
      Serial.println("  - Free heap below threshold");
    }

    if (!queueOk) {
      Serial.println("  - Sample queue drops detected");
    }

    if (!hasInference) {
      Serial.println("  - No inference result yet");
    } else if (!inferenceOk) {
      Serial.println("  - Inference pipeline timeout");
    }
  }

  Serial.println("==================");
  Serial.println();
}

static void dumpFeature() {
  FeatureWindow f = SystemState_GetLastFeature();

  Serial.println();
  Serial.println("===== FEATURE =====");

  Serial.print("mean: ");
  Serial.print(f.meanX, 3);
  Serial.print(", ");
  Serial.print(f.meanY, 3);
  Serial.print(", ");
  Serial.println(f.meanZ, 3);

  Serial.print("var: ");
  Serial.print(f.varX, 4);
  Serial.print(", ");
  Serial.print(f.varY, 4);
  Serial.print(", ");
  Serial.println(f.varZ, 4);

  Serial.print("energy: ");
  Serial.println(f.energy, 4);

  Serial.print("max_abs: ");
  Serial.println(f.maxAbs, 4);

  Serial.print("window: ");
  Serial.print(f.windowStartMs);
  Serial.print(" -> ");
  Serial.println(f.windowEndMs);

  Serial.println("===================");
  Serial.println();
}

void HostProtocol_PrintHelp() {
  Serial.println();
  Serial.println("Available commands:");
  Serial.println("  HELP");
  Serial.println("  GET_STATUS");
  Serial.println("  GET_STATS");
  Serial.println("  GET_HEALTH");
  Serial.println("  GET_EVENT_LOG");
  Serial.println("  CLEAR_EVENT_LOG");  
  Serial.println("  DUMP_FEATURE");
  Serial.println("  RESET_STATS");
  Serial.println("  SET_MODE NORMAL");
  Serial.println("  SET_MODE DEBUG");
  Serial.println("  SET_MODE LOW_POWER");
  Serial.println("  SET_CLASSIFIER RULE");
  Serial.println("  SET_CLASSIFIER MODEL");
  Serial.println("  SET_PATTERN STILL");
  Serial.println("  SET_PATTERN MOVING");
  Serial.println("  SET_PATTERN SHAKING");
  Serial.println("  SET_PATTERN IMPACT");
  Serial.println();
}

void HostProtocol_ProcessLine(String line) {
  line.trim();
  line.toUpperCase();

  if (line.length() == 0) {
    return;
  }

  if (line == "HELP") {
    HostProtocol_PrintHelp();
    return;
  }

  if (line == "GET_STATUS") {
    printStatus();
    return;
  }

  if (line == "GET_STATS") {
    printStats();
    return;
  }

  if (line == "GET_HEALTH") {
    printHealth();
    return;
  }

  if (line == "GET_EVENT_LOG") {
    EventLog_Print();
    return;
  }

  if (line == "CLEAR_EVENT_LOG") {
    EventLog_Clear();
    Serial.println("OK CLEAR_EVENT_LOG");
    return;
  }

  if (line == "DUMP_FEATURE") {
    dumpFeature();
    return;
  }

  if (line == "RESET_STATS") {
    SystemState_ResetStats();
    Serial.println("OK RESET_STATS");
    return;
  }

  if (line.startsWith("SET_MODE ")) {
    String modeText = line.substring(strlen("SET_MODE "));
    modeText.trim();

    SystemMode newMode;
    if (parseMode(modeText, &newMode)) {
      SystemState_SetMode(newMode);

      Serial.print("OK SET_MODE ");
      Serial.println(modeToString(newMode));
    } else {
      Serial.print("ERR Unknown mode: ");
      Serial.println(modeText);
      Serial.println("Valid modes: NORMAL, DEBUG, LOW_POWER");
    }

    return;
  }

  if (line.startsWith("SET_CLASSIFIER ")) {
    String modeText = line.substring(strlen("SET_CLASSIFIER "));
    modeText.trim();

    ClassifierMode newClassifierMode;

    if (parseClassifierMode(modeText, &newClassifierMode)) {
      SystemState_SetClassifierMode(newClassifierMode);

      Serial.print("OK SET_CLASSIFIER ");
      Serial.println(classifierModeToString(newClassifierMode));
    } else {
      Serial.print("ERR Unknown classifier: ");
      Serial.println(modeText);
      Serial.println("Valid classifiers: RULE, MODEL");
    }

    return;
  }

  if (line.startsWith("SET_PATTERN ")) {
    String patternText = line.substring(strlen("SET_PATTERN "));
    patternText.trim();

    SensorPattern newPattern;
    if (parsePattern(patternText, &newPattern)) {
      SystemState_SetPattern(newPattern);

      Serial.print("OK SET_PATTERN ");
      Serial.println(patternToString(newPattern));
    } else {
      Serial.print("ERR Unknown pattern: ");
      Serial.println(patternText);
      Serial.println("Valid patterns: STILL, MOVING, SHAKING, IMPACT");
    }

    return;
  }

  Serial.print("ERR Unknown command: ");
  Serial.println(line);
  Serial.println("Type HELP for commands.");
}