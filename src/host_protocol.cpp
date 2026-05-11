#include "host_protocol.h"

#include <string.h>

#include "app_types.h"
#include "system_state.h"

static void printStatus() {
  SystemMode mode = SystemState_GetMode();
  SensorPattern pattern = SystemState_GetPattern();
  MotionState motion = SystemState_GetMotionState();
  InferenceResult inf = SystemState_GetLastInference();

  Serial.println();
  Serial.println("===== STATUS =====");
  Serial.print("Mode: ");
  Serial.println(modeToString(mode));
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
  Serial.println("  DUMP_FEATURE");
  Serial.println("  RESET_STATS");
  Serial.println("  SET_MODE NORMAL");
  Serial.println("  SET_MODE DEBUG");
  Serial.println("  SET_MODE LOW_POWER");
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