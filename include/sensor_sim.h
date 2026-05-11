#pragma once

#include "app_types.h"

// Initialize the simulated sensor backend.
void SensorSim_Init();

// Generate one mock accelerometer sample based on the selected pattern.
SensorSample SensorSim_ReadSample(SensorPattern pattern);