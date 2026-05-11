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

// =====================
// String helpers
// =====================

const char* modeToString(SystemMode mode);
const char* motionToString(MotionState state);
const char* patternToString(SensorPattern pattern);

bool parseMode(const String& text, SystemMode* modeOut);
bool parsePattern(const String& text, SensorPattern* patternOut);