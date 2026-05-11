#include "app_types.h"

const char* modeToString(SystemMode mode) {
  switch (mode) {
    case MODE_NORMAL:
      return "NORMAL";
    case MODE_DEBUG:
      return "DEBUG";
    case MODE_LOW_POWER:
      return "LOW_POWER";
    default:
      return "UNKNOWN";
  }
}

const char* motionToString(MotionState state) {
  switch (state) {
    case MOTION_STILL:
      return "STILL";
    case MOTION_MOVING:
      return "MOVING";
    case MOTION_SHAKING:
      return "SHAKING";
    case MOTION_IMPACT:
      return "IMPACT";
    default:
      return "UNKNOWN";
  }
}

const char* patternToString(SensorPattern pattern) {
  switch (pattern) {
    case PATTERN_STILL:
      return "STILL";
    case PATTERN_MOVING:
      return "MOVING";
    case PATTERN_SHAKING:
      return "SHAKING";
    case PATTERN_IMPACT:
      return "IMPACT";
    default:
      return "UNKNOWN";
  }
}

bool parseMode(const String& text, SystemMode* modeOut) {
  if (modeOut == nullptr) {
    return false;
  }

  if (text == "NORMAL") {
    *modeOut = MODE_NORMAL;
    return true;
  }

  if (text == "DEBUG") {
    *modeOut = MODE_DEBUG;
    return true;
  }

  if (text == "LOW_POWER") {
    *modeOut = MODE_LOW_POWER;
    return true;
  }

  return false;
}

bool parsePattern(const String& text, SensorPattern* patternOut) {
  if (patternOut == nullptr) {
    return false;
  }

  if (text == "STILL") {
    *patternOut = PATTERN_STILL;
    return true;
  }

  if (text == "MOVING") {
    *patternOut = PATTERN_MOVING;
    return true;
  }

  if (text == "SHAKING") {
    *patternOut = PATTERN_SHAKING;
    return true;
  }

  if (text == "IMPACT") {
    *patternOut = PATTERN_IMPACT;
    return true;
  }

  return false;
}

const char* eventReasonToString(EventReason reason) {
  switch (reason) {
    case EVENT_REASON_FIRST_RESULT:
      return "FIRST_RESULT";
    case EVENT_REASON_STATE_CHANGE:
      return "STATE_CHANGE";
    case EVENT_REASON_HIGH_PRIORITY_IMPACT:
      return "HIGH_PRIORITY_IMPACT";
    default:
      return "UNKNOWN";
  }
}

const char* classifierModeToString(ClassifierMode mode) {
  switch (mode) {
    case CLASSIFIER_RULE:
      return "RULE";
    case CLASSIFIER_MODEL:
      return "MODEL";
    default:
      return "UNKNOWN";
  }
}

bool parseClassifierMode(const String& text, ClassifierMode* modeOut) {
  if (modeOut == nullptr) {
    return false;
  }

  if (text == "RULE") {
    *modeOut = CLASSIFIER_RULE;
    return true;
  }

  if (text == "MODEL") {
    *modeOut = CLASSIFIER_MODEL;
    return true;
  }

  return false;
}