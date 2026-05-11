#include "event_manager.h"

#include <Arduino.h>

#include "system_state.h"
#include "host_interrupt.h"
#include "event_log.h"

static MotionState g_lastNotifiedState = MOTION_STILL;
static bool g_firstResult = true;

void EventManager_Init() {
  g_lastNotifiedState = MOTION_STILL;
  g_firstResult = true;
}

void EventManager_ProcessInference(const InferenceResult& result) {
  bool shouldNotify = false;

  MotionState previous = SystemState_GetMotionState();

  // Update global motion state based on latest inference result.
  SystemState_SetMotionState(result.state);

  EventReason reason = EVENT_REASON_STATE_CHANGE;

  if (g_firstResult) {
    shouldNotify = true;
    reason = EVENT_REASON_FIRST_RESULT;
    g_firstResult = false;
  } else if (result.state != g_lastNotifiedState) {
    shouldNotify = true;
    reason = EVENT_REASON_STATE_CHANGE;
  } else if (result.state == MOTION_IMPACT) {
    // IMPACT is considered high priority, so notify every time.
    shouldNotify = true;
    reason = EVENT_REASON_HIGH_PRIORITY_IMPACT;
  }

  if (shouldNotify) {
    SystemState_IncrementHostNotifications();

    EventRecord record = {};
    record.timestampMs = result.timestampMs;
    record.previousState = previous;
    record.newState = result.state;
    record.confidence = result.confidence;
    record.energy = result.energy;
    record.maxAbs = result.maxAbs;
    record.reason = reason;

    // Store event context before interrupting the host.
    EventLog_Add(record);

    // Simulate sensor hub interrupting the host CPU.
    HostInterrupt_Trigger();

    Serial.print("EVENT MOTION ");
    Serial.print(motionToString(previous));
    Serial.print(" -> ");
    Serial.print(motionToString(result.state));
    Serial.print(" reason=");
    Serial.print(eventReasonToString(reason));
    Serial.print(" conf=");
    Serial.print(result.confidence, 2);
    Serial.print(" energy=");
    Serial.print(result.energy, 2);
    Serial.print(" max=");
    Serial.println(result.maxAbs, 2);

    g_lastNotifiedState = result.state;
  } else {
    SystemState_IncrementSuppressedNotifications();
  }

  SystemMode mode = SystemState_GetMode();

  if (mode == MODE_DEBUG) {
    Serial.print("[Inference] state=");
    Serial.print(motionToString(result.state));
    Serial.print(", conf=");
    Serial.print(result.confidence, 2);
    Serial.print(", energy=");
    Serial.print(result.energy, 2);
    Serial.print(", max=");
    Serial.println(result.maxAbs, 2);
  }
}