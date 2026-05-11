#pragma once

#include "app_types.h"

// Run a tiny model-based classifier.
// This uses fixed prototype vectors as lightweight model parameters.
InferenceResult ClassifierModel_Run(const FeatureWindow& feature);