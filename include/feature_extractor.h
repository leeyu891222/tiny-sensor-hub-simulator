#pragma once

#include <stddef.h>
#include "app_types.h"

// Compute statistical features from a window of accelerometer samples.
FeatureWindow FeatureExtractor_Compute(const SensorSample* samples, size_t count);