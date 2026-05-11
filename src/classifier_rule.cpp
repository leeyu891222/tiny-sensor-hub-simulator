#include "classifier_rule.h"

#include <Arduino.h>

InferenceResult ClassifierRule_Run(const FeatureWindow& f) {
  InferenceResult r = {};

  r.timestampMs = millis();
  r.energy = f.energy;
  r.maxAbs = f.maxAbs;

  float totalVar = f.varX + f.varY + f.varZ;

  if (f.maxAbs > 2.4f) {
    r.state = MOTION_IMPACT;
    r.confidence = 0.95f;
  } else if (f.energy > 1.8f || totalVar > 0.45f) {
    r.state = MOTION_SHAKING;
    r.confidence = 0.88f;
  } else if (totalVar > 0.025f) {
    r.state = MOTION_MOVING;
    r.confidence = 0.80f;
  } else {
    r.state = MOTION_STILL;
    r.confidence = 0.90f;
  }

  return r;
}