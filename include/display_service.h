#pragma once

#include <Arduino.h>

// Initialize OLED display and I2C bus.
// Returns true if initialization succeeds.
bool DisplayService_Init();

// Update OLED with latest system state.
void DisplayService_Update();