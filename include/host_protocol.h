#pragma once

#include <Arduino.h>

// Print supported command list.
void HostProtocol_PrintHelp();

// Process one complete command line from host.
// Example:
//   GET_STATUS
//   SET_PATTERN MOVING
//   SET_MODE DEBUG
void HostProtocol_ProcessLine(String line);