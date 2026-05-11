#pragma once

#include <Arduino.h>
#include "app_types.h"

// Initialize system state module.
// Must be called before using any SystemState_* API.
bool SystemState_Init();

// =====================
// Mode / pattern / motion state
// =====================

SystemMode SystemState_GetMode();
SensorPattern SystemState_GetPattern();
MotionState SystemState_GetMotionState();

void SystemState_SetMode(SystemMode mode);
void SystemState_SetPattern(SensorPattern pattern);
void SystemState_SetMotionState(MotionState state);

// Switch NORMAL -> DEBUG -> LOW_POWER -> NORMAL.
// Returns the new mode.
SystemMode SystemState_SwitchMode();

// =====================
// Last feature / inference
// =====================

void SystemState_UpdateLastFeature(const FeatureWindow& feature);
void SystemState_UpdateLastInference(const InferenceResult& result);

FeatureWindow SystemState_GetLastFeature();
InferenceResult SystemState_GetLastInference();

// =====================
// Statistics
// =====================

SystemStats SystemState_GetStats();
void SystemState_ResetStats();

void SystemState_IncrementSamplesGenerated();
void SystemState_IncrementFeaturesComputed();
void SystemState_IncrementInferencesRun();
void SystemState_IncrementHostNotifications();
void SystemState_IncrementSuppressedNotifications();
void SystemState_IncrementSampleQueueDrops();

// =====================
// Classifier
// =====================

ClassifierMode SystemState_GetClassifierMode();
void SystemState_SetClassifierMode(ClassifierMode mode);