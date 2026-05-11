#pragma once

#include <Arduino.h>

// Initialize the simulated host interrupt GPIO.
// Must be called before HostInterrupt_Trigger().
bool HostInterrupt_Init();

// Trigger a host interrupt pulse.
// This function is non-blocking: it only notifies the HostInterruptTask.
void HostInterrupt_Trigger();

// Task that generates the actual GPIO pulse.
// Create this as a FreeRTOS task in main.cpp.
void HostInterruptTask(void* parameter);