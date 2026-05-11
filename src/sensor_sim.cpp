#include "sensor_sim.h"

#include <Arduino.h>
#include <math.h>

static uint32_t g_sampleIndex = 0;

static float randomNoise(float amplitude) {
  return ((float)random(-1000, 1000) / 1000.0f) * amplitude;
}

void SensorSim_Init() {
  g_sampleIndex = 0;

  // For simulation randomness.
  randomSeed(analogRead(0));
}

SensorSample SensorSim_ReadSample(SensorPattern pattern) {
  g_sampleIndex++;

  SensorSample s;
  s.timestampMs = millis();

  float t = g_sampleIndex * 0.1f;

  switch (pattern) {
    case PATTERN_STILL:
      // 靜止：大約只有 1g 重力 + 小雜訊
      s.ax = 0.00f + randomNoise(0.02f);
      s.ay = 0.00f + randomNoise(0.02f);
      s.az = 1.00f + randomNoise(0.02f);
      break;

    case PATTERN_MOVING:
      // 移動：中等週期變化
      s.ax = 0.25f * sin(t) + randomNoise(0.05f);
      s.ay = 0.18f * cos(t * 0.7f) + randomNoise(0.05f);
      s.az = 1.00f + 0.15f * sin(t * 1.3f) + randomNoise(0.05f);
      break;

    case PATTERN_SHAKING:
      // 搖晃：高能量震盪
      s.ax = 0.90f * sin(t * 3.0f) + randomNoise(0.20f);
      s.ay = 0.80f * cos(t * 2.7f) + randomNoise(0.20f);
      s.az = 1.00f + 0.70f * sin(t * 3.5f) + randomNoise(0.20f);
      break;

    case PATTERN_IMPACT:
      // 撞擊：大部分時間接近靜止，但週期性產生尖峰
      s.ax = 0.00f + randomNoise(0.03f);
      s.ay = 0.00f + randomNoise(0.03f);
      s.az = 1.00f + randomNoise(0.03f);

      if ((g_sampleIndex % 50) == 0) {
        s.ax += 2.5f;
        s.ay += 1.8f;
        s.az += 2.0f;
      }
      break;

    default:
      s.ax = 0.0f;
      s.ay = 0.0f;
      s.az = 1.0f;
      break;
  }

  return s;
}