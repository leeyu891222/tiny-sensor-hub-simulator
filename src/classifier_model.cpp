#include "classifier_model.h"

#include <Arduino.h>
#include <math.h>

struct ModelPrototype {
  MotionState state;
  float x0_totalVar;
  float x1_energy;
  float x2_maxAbs;
};

// Hand-calibrated prototype model.
//
// Feature vector:
//   x0 = normalized total variance
//   x1 = normalized energy above gravity baseline
//   x2 = normalized max acceleration above gravity baseline
//
// These constants act as tiny model parameters stored in flash.
static const ModelPrototype kPrototypes[] = {
  { MOTION_STILL,   0.00f, 0.00f, 0.00f },
  { MOTION_MOVING,  0.08f, 0.04f, 0.06f },
  { MOTION_SHAKING, 0.75f, 0.55f, 0.35f },
  { MOTION_IMPACT,  0.20f, 0.25f, 0.95f },
};

static float clampFloat(float v, float minValue, float maxValue) {
  if (v < minValue) {
    return minValue;
  }

  if (v > maxValue) {
    return maxValue;
  }

  return v;
}

static void buildFeatureVector(const FeatureWindow& f, float* x0, float* x1, float* x2) {
  float totalVar = f.varX + f.varY + f.varZ;

  // Normalize to roughly comparable ranges.
  *x0 = clampFloat(totalVar / 0.80f, 0.0f, 1.5f);
  *x1 = clampFloat((f.energy - 1.0f) / 1.50f, 0.0f, 1.5f);
  *x2 = clampFloat((f.maxAbs - 1.0f) / 3.50f, 0.0f, 1.5f);
}

static float weightedDistanceSquared(
  float x0,
  float x1,
  float x2,
  const ModelPrototype& p
) {
  float d0 = x0 - p.x0_totalVar;
  float d1 = x1 - p.x1_energy;
  float d2 = x2 - p.x2_maxAbs;

  // Give maxAbs more weight because it is the strongest signal for IMPACT.
  const float w0 = 1.0f;
  const float w1 = 1.0f;
  const float w2 = 2.0f;

  return (w0 * d0 * d0) + (w1 * d1 * d1) + (w2 * d2 * d2);
}

InferenceResult ClassifierModel_Run(const FeatureWindow& feature) {
  InferenceResult result = {};

  result.timestampMs = millis();
  result.energy = feature.energy;
  result.maxAbs = feature.maxAbs;

  float x0;
  float x1;
  float x2;
  buildFeatureVector(feature, &x0, &x1, &x2);

  float bestDistance = 999999.0f;
  float secondBestDistance = 999999.0f;
  MotionState bestState = MOTION_STILL;

  const size_t prototypeCount = sizeof(kPrototypes) / sizeof(kPrototypes[0]);

  for (size_t i = 0; i < prototypeCount; i++) {
    float d = weightedDistanceSquared(x0, x1, x2, kPrototypes[i]);

    if (d < bestDistance) {
      secondBestDistance = bestDistance;
      bestDistance = d;
      bestState = kPrototypes[i].state;
    } else if (d < secondBestDistance) {
      secondBestDistance = d;
    }
  }

  result.state = bestState;

  // Simple confidence estimate based on separation between best and second-best.
  // Larger separation means higher confidence.
  float confidence = secondBestDistance / (bestDistance + secondBestDistance + 0.0001f);
  result.confidence = clampFloat(confidence, 0.50f, 0.99f);

  return result;
}