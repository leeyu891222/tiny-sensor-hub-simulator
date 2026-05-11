# TinyML Sensor Hub Simulator

An RTOS-based simulated mobile sensor hub firmware project built with **ESP32 + FreeRTOS + Wokwi**.

This project demonstrates how a low-power sensor hub can process high-frequency raw sensor data locally, extract compact features, run lightweight motion-event classification, and notify the host CPU only when meaningful events occur.

The goal is not to build a complex AI model first. The main focus is to demonstrate solid embedded firmware fundamentals: RTOS task design, queues, ISR-to-task handoff, shared-state protection, driver/service-style modularization, host communication, and event-driven system design.

---

## Motivation

Modern mobile devices often include a low-power microcontroller, DSP, or sensor hub responsible for always-on sensing tasks. Instead of waking the main ARM application processor for every sensor sample, the sensor hub can:

- collect raw sensor data,
- perform local filtering and feature extraction,
- classify motion or context events,
- notify the host CPU only when important events occur.

This reduces host wakeups, lowers power consumption, and allows always-on sensing behavior such as raise-to-wake, pocket detection, activity recognition, impact detection, and context-aware power management.

This project simulates that idea using an ESP32 in Wokwi.

---

## Current Features

- ESP32-based simulated sensor hub
- FreeRTOS task pipeline
- Mock accelerometer data generation
- Motion patterns:
  - `STILL`
  - `MOVING`
  - `SHAKING`
  - `IMPACT`
- Window-based feature extraction
- Rule-based lightweight classifier baseline
- Event manager that suppresses unnecessary host notifications
- UART/Serial host command interface
- OLED display for current system state
- Button interrupt for mode switching
- Mutex-protected shared system state
- Queue-based inter-task communication
- Runtime statistics showing notification reduction

---

## System Architecture

```text
Host CPU / Serial Terminal
        ^
        |
        | UART commands / event notifications
        |
+-------+------------------------------------------------+
|                 Sensor Hub Firmware                    |
|                                                        |
|  HostCommTask                                          |
|      |                                                 |
|      v                                                 |
|  host_protocol                                         |
|                                                        |
|  SensorTask                                            |
|      | mock raw ax/ay/az samples                       |
|      v                                                 |
|  sampleQueue                                           |
|      v                                                 |
|  FeatureTask                                           |
|      | mean / variance / energy / max_abs              |
|      v                                                 |
|  featureQueue                                          |
|      v                                                 |
|  InferenceTask                                         |
|      | rule-based lightweight classification            |
|      v                                                 |
|  inferenceQueue                                        |
|      v                                                 |
|  EventManagerTask                                      |
|      | notify host only on meaningful events            |
|      v                                                 |
|  Serial EVENT output                                   |
|                                                        |
|  DisplayTask -> OLED                                   |
|  Button ISR -> ButtonTask -> mode switching            |
+--------------------------------------------------------+
```

---

## RTOS Task Design

| Task | Responsibility |
|---|---|
| `SensorTask` | Generates mock accelerometer samples at a configurable sampling rate. |
| `FeatureTask` | Collects a sample window and computes statistical features. |
| `InferenceTask` | Runs the lightweight rule-based classifier. |
| `EventManagerTask` | Decides whether the host CPU should be notified. |
| `HostCommTask` | Receives and parses host commands over Serial/UART. |
| `DisplayTask` | Periodically updates the OLED display. |
| `ButtonTask` | Handles button events deferred from GPIO interrupt. |

The task pipeline uses FreeRTOS queues:

```text
SensorTask -> sampleQueue -> FeatureTask
FeatureTask -> featureQueue -> InferenceTask
InferenceTask -> inferenceQueue -> EventManagerTask
```

---

## Firmware Concepts Demonstrated

### GPIO Interrupt and ISR-to-Task Handoff

The button is configured as an active-low GPIO input using internal pull-up. The ISR does not perform mode switching directly. It only notifies `ButtonTask` using a FreeRTOS task notification.

```text
Button falling edge
    -> buttonISR()
    -> vTaskNotifyGiveFromISR()
    -> ButtonTask handles debounce and mode switching
```

This demonstrates the firmware design principle:

> ISR should be short and defer real work to task context.

---

### Queue-Based Data Pipeline

Raw sensor samples are not processed directly inside `SensorTask`. Instead, each stage communicates through FreeRTOS queues.

This makes the pipeline modular and closer to real sensor-hub firmware design.

---

### Mutex-Protected Shared State

Runtime state is centralized in `system_state` and accessed through mutex-protected APIs.

Shared state includes:

- current mode,
- current sensor pattern,
- current motion state,
- latest feature window,
- latest inference result,
- runtime statistics.

This prevents direct uncontrolled access to global variables from multiple RTOS tasks.

---

### I2C Display Service

The OLED display is managed through `display_service`. I2C access is protected internally with a mutex.

Although the current sensor data is simulated, this structure allows future expansion where a real I2C IMU and the OLED may share the same I2C bus.

---

### Host Communication Protocol

The host CPU is simulated using a UART/Serial terminal. The host can configure the sensor hub and query status.

Supported commands:

```text
HELP
GET_STATUS
GET_STATS
DUMP_FEATURE
RESET_STATS
SET_MODE NORMAL
SET_MODE DEBUG
SET_MODE LOW_POWER
SET_PATTERN STILL
SET_PATTERN MOVING
SET_PATTERN SHAKING
SET_PATTERN IMPACT
```

This models how a host processor might configure or query a sensor hub.

---

## Sensor Simulation

The project currently uses mock accelerometer data instead of a physical IMU.

Each generated sample contains:

```cpp
struct SensorSample {
  float ax;
  float ay;
  float az;
  uint32_t timestampMs;
};
```

The simulated sensor backend supports these patterns:

### STILL

A stable phone-like state with small noise:

```text
ax ≈ 0g
ay ≈ 0g
az ≈ 1g
```

### MOVING

Moderate periodic movement with medium variance.

### SHAKING

High-energy movement with large variance.

### IMPACT

Mostly stable motion with periodic high acceleration peaks.

This is useful for validating the firmware pipeline without requiring physical hardware.

---

## Feature Extraction

`FeatureTask` collects a fixed-size window of raw accelerometer samples and computes:

- mean X/Y/Z,
- variance X/Y/Z,
- average energy,
- max acceleration magnitude,
- window start and end time.

The current window size is configured in `app_config.h`:

```cpp
#define APP_FEATURE_WINDOW_SIZE 50
```

At 50 Hz sampling, 50 samples represent approximately 1 second of sensor data.

---

## Lightweight Classifier Baseline

The current classifier is rule-based and intentionally simple.

It uses:

- `maxAbs` to detect `IMPACT`,
- `energy` and total variance to detect `SHAKING`,
- moderate variance to detect `MOVING`,
- low variance to detect `STILL`.

This is implemented in `classifier_rule.cpp`.

This module is designed to be replaceable. A future version can add:

- logistic regression,
- decision tree,
- small MLP,
- TensorFlow Lite Micro model.

The rest of the firmware pipeline does not need to change significantly.

---

## Event Filtering and Host Notification

`event_manager` decides whether to notify the host.

Current policy:

- notify on the first inference result,
- notify when motion state changes,
- always notify `IMPACT` because it is high priority,
- suppress repeated non-critical states.

This demonstrates the core sensor hub value:

> Process many raw samples locally, but notify the host only when needed.

Example runtime statistics:

```text
Samples generated: 1954
Features computed: 39
Inferences run: 39
Host notifications: 8
Suppressed notifications: 31
Sample queue drops: 0
Notify/sample ratio: 0.409%
Estimated wakeup reduction: 99.591%
```

---

## Project Structure

```text
include/
  app_config.h          Project-level configuration
  app_types.h           Shared enums, structs, parsing/string helpers
  system_state.h        Mutex-protected shared runtime state
  sensor_sim.h          Simulated accelerometer backend
  feature_extractor.h   Raw sample window -> feature window
  classifier_rule.h     Rule-based lightweight classifier
  event_manager.h       Host notification policy
  host_protocol.h       Host command parser
  display_service.h     OLED display service

src/
  main.cpp              RTOS task orchestration, ISR, setup/loop
  app_types.cpp
  system_state.cpp
  sensor_sim.cpp
  feature_extractor.cpp
  classifier_rule.cpp
  event_manager.cpp
  host_protocol.cpp
  display_service.cpp
```

---

## Configuration

Most project-level settings are centralized in `app_config.h`:

```cpp
#define APP_BUTTON_PIN 18
#define APP_I2C_SDA_PIN 21
#define APP_I2C_SCL_PIN 22

#define APP_SAMPLE_QUEUE_LEN 64
#define APP_FEATURE_QUEUE_LEN 4
#define APP_INFERENCE_QUEUE_LEN 4

#define APP_FEATURE_WINDOW_SIZE 50

#define APP_SENSOR_PERIOD_NORMAL_MS 20
#define APP_SENSOR_PERIOD_LOW_POWER_MS 100
```

This avoids scattering magic numbers throughout the project.

---

## Hardware Simulation

Current Wokwi components:

- ESP32 DevKit
- SSD1306 I2C OLED
- Pushbutton
- Serial terminal

Recommended wiring:

| Component | ESP32 Pin |
|---|---|
| OLED VCC | 3V3 |
| OLED GND | GND |
| OLED SDA | GPIO21 / D21 |
| OLED SCL | GPIO22 / D22 |
| Button side A | GPIO18 / D18 |
| Button side B | GND |
| Serial TX | TX0 |
| Serial RX | RX0 |

---

## How to Run

### 1. Build with PlatformIO

```bash
pio run
```

### 2. Start Wokwi Simulator

Open `diagram.json` and start the Wokwi simulator from VS Code.

### 3. Use Serial Terminal

Run commands such as:

```text
GET_STATUS
SET_PATTERN MOVING
DUMP_FEATURE
SET_PATTERN SHAKING
SET_PATTERN IMPACT
GET_STATS
SET_MODE DEBUG
SET_PATTERN STILL
```

---

## Demo Script

A recommended demo sequence:

```text
GET_STATUS
SET_PATTERN MOVING
DUMP_FEATURE
SET_PATTERN SHAKING
SET_PATTERN IMPACT
GET_STATS
SET_MODE DEBUG
SET_PATTERN STILL
```

Expected behavior:

1. `GET_STATUS` shows the current mode, sensor pattern, motion state, confidence, energy, and max acceleration.
2. `SET_PATTERN MOVING` causes the pipeline to classify movement.
3. `DUMP_FEATURE` shows extracted window features.
4. `SET_PATTERN SHAKING` increases energy and variance.
5. `SET_PATTERN IMPACT` triggers high-priority impact notifications.
6. `GET_STATS` shows how many raw samples were processed versus how many notifications were sent.
7. `SET_MODE DEBUG` enables additional inference logging.

---

## Example Output

```text
Booting TinyML Sensor Hub Simulator...
System started.
EVENT MOTION STILL -> STILL conf=0.90 energy=1.00 max=1.02

OK SET_PATTERN MOVING
EVENT MOTION STILL -> MOVING conf=0.80 energy=1.05 max=1.22

OK SET_PATTERN SHAKING
EVENT MOTION MOVING -> SHAKING conf=0.88 energy=1.74 max=2.07

OK SET_PATTERN IMPACT
EVENT MOTION SHAKING -> IMPACT conf=0.95 energy=2.13 max=4.26

===== STATS =====
Samples generated: 1954
Features computed: 39
Inferences run: 39
Host notifications: 8
Suppressed notifications: 31
Sample queue drops: 0
Notify/sample ratio: 0.409%
Estimated wakeup reduction: 99.591%
=================
```

---

## Design Highlights

### Firmware-Oriented Modularity

The project is divided into separate modules for sensor simulation, feature extraction, classification, event management, host protocol, display service, and shared system state.

### Replaceable Sensor Backend

The current sensor backend is simulated, but upper layers only depend on the `SensorSample` abstraction. This makes it possible to replace the simulated backend with a real I2C IMU driver later.

### Replaceable Classifier

The current rule-based classifier is a baseline. A future `classifier_tiny` module can be added without changing the RTOS pipeline.

### Host Wakeup Reduction

Runtime statistics show the ratio between raw samples and host notifications. This directly demonstrates why sensor hubs are useful in mobile systems.

---

## Future Work

Potential next steps:

1. Add an event cooldown policy for repeated `IMPACT` notifications.
2. Replace rule-based classifier with a small logistic regression or decision tree model.
3. Add a real I2C IMU driver backend.
4. Add binary host protocol support:

   ```text
   Header | Length | Command | Payload | CRC
   ```

5. Add CRC / ACK / NACK handling to host protocol.
6. Add watchdog and health monitoring.
7. Add stack watermark / queue usage diagnostics.
8. Port the firmware structure to Zephyr or ESP-IDF.
9. Simulate host-processor communication using SPI/mailbox style abstraction.

---

## Resume Description

```text
Built an RTOS-based simulated mobile sensor hub on ESP32/Wokwi. The firmware generates mock accelerometer streams, extracts statistical features, runs lightweight motion classification, and notifies the host CPU only on meaningful events. Implemented FreeRTOS task pipeline, queues, GPIO interrupt-to-task notification, mutex-protected shared state, UART host protocol, OLED display service, modular firmware architecture, and wakeup-reduction statistics.
```

---

## Interview Talking Points

- The sensor hub reduces host CPU wakeups by processing raw sensor samples locally.
- RTOS queues decouple sampling, feature extraction, inference, and event management.
- ISR is kept short and defers button handling to `ButtonTask`.
- Shared state is centralized and protected by a mutex.
- The classifier is intentionally modular so a future tiny ML model can replace the rule-based baseline.
- The project currently uses mock sensor data, but the sensor backend is abstracted for future real IMU integration.

