#pragma once

#include <stddef.h>
#include "app_types.h"

// Initialize event log ring buffer.
void EventLog_Init();

// Add one event record to the ring buffer.
// If the buffer is full, the oldest record is overwritten.
void EventLog_Add(const EventRecord& record);

// Clear all records.
void EventLog_Clear();

// Return number of records currently stored.
size_t EventLog_Count();

// Print event log to Serial.
void EventLog_Print();