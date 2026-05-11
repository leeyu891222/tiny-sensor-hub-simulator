#pragma once

#include <Arduino.h>
#include <stdint.h>

// =====================
// System state enums
// =====================

enum SystemMode {
  MODE_NORMAL = 0,
  MODE_DEBUG,
  MODE_LOW_POWER
};

enum MotionState {
  MOTION_STILL = 0,
  MOTION_MOVING,
  MOTION_SHAKING,
  MOTION_IMPACT
};

enum SensorPattern {
  PATTERN_STILL = 0,
  PATTERN_MOVING,
  PATTERN_SHAKING,
  PATTERN_IMPACT
};

enum EventReason {
  EVENT_REASON_FIRST_RESULT = 0,
  EVENT_REASON_STATE_CHANGE,
  EVENT_REASON_HIGH_PRIORITY_IMPACT
};

enum ClassifierMode {
  CLASSIFIER_RULE = 0,
  CLASSIFIER_MODEL
};

// =====================
// Data structures
// =====================

struct SensorSample {
  float ax;
  float ay;
  float az;
  uint32_t timestampMs;
};

struct FeatureWindow {
  float meanX;
  float meanY;
  float meanZ;

  float varX;
  float varY;
  float varZ;

  float energy;
  float maxAbs;

  uint32_t windowStartMs;
  uint32_t windowEndMs;
};

struct InferenceResult {
  MotionState state;
  float confidence;
  float energy;
  float maxAbs;
  uint32_t timestampMs;
};

struct SystemStats {
  uint32_t samplesGenerated;
  uint32_t featuresComputed;
  uint32_t inferencesRun;
  uint32_t hostNotifications;
  uint32_t suppressedNotifications;
  uint32_t sampleQueueDrops;
};

struct EventRecord {
  uint32_t timestampMs;
  MotionState previousState;
  MotionState newState;
  float confidence;
  float energy;
  float maxAbs;
  EventReason reason;
};

// =====================
// String helpers
// =====================

const char* modeToString(SystemMode mode);
const char* motionToString(MotionState state);
const char* patternToString(SensorPattern pattern);
const char* eventReasonToString(EventReason reason);
const char* classifierModeToString(ClassifierMode mode);

bool parseClassifierMode(const String& text, ClassifierMode* modeOut);
bool parseMode(const String& text, SystemMode* modeOut);
bool parsePattern(const String& text, SensorPattern* patternOut);