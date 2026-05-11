#pragma once

#include "app_types.h"

// Initialize event manager internal state.
void EventManager_Init();

// Process one inference result.
// This function decides whether to notify the host.
void EventManager_ProcessInference(const InferenceResult& result);