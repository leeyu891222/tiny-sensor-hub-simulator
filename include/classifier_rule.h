#pragma once

#include "app_types.h"

// Run the baseline rule-based classifier.
// This is the current lightweight inference module.
// Later, this can be replaced by a tiny ML model implementation.
InferenceResult ClassifierRule_Run(const FeatureWindow& feature);