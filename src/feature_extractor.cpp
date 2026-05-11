#include "feature_extractor.h"

#include <math.h>

FeatureWindow FeatureExtractor_Compute(const SensorSample* samples, size_t count) {
  FeatureWindow f = {};

  if (samples == nullptr || count == 0) {
    return f;
  }

  f.windowStartMs = samples[0].timestampMs;
  f.windowEndMs = samples[count - 1].timestampMs;

  float sumX = 0.0f;
  float sumY = 0.0f;
  float sumZ = 0.0f;

  for (size_t i = 0; i < count; i++) {
    sumX += samples[i].ax;
    sumY += samples[i].ay;
    sumZ += samples[i].az;
  }

  f.meanX = sumX / count;
  f.meanY = sumY / count;
  f.meanZ = sumZ / count;

  float varX = 0.0f;
  float varY = 0.0f;
  float varZ = 0.0f;
  float energy = 0.0f;
  float maxAbs = 0.0f;

  for (size_t i = 0; i < count; i++) {
    float dx = samples[i].ax - f.meanX;
    float dy = samples[i].ay - f.meanY;
    float dz = samples[i].az - f.meanZ;

    varX += dx * dx;
    varY += dy * dy;
    varZ += dz * dz;

    float mag = sqrtf(samples[i].ax * samples[i].ax +
                      samples[i].ay * samples[i].ay +
                      samples[i].az * samples[i].az);

    energy += mag * mag;

    if (mag > maxAbs) {
      maxAbs = mag;
    }
  }

  f.varX = varX / count;
  f.varY = varY / count;
  f.varZ = varZ / count;
  f.energy = energy / count;
  f.maxAbs = maxAbs;

  return f;
}