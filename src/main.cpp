#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// =====================
// Hardware config
// =====================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define BUTTON_PIN 18

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// =====================
// Sensor / AI config
// =====================
#define SAMPLE_QUEUE_LEN 64
#define FEATURE_QUEUE_LEN 4
#define INFERENCE_QUEUE_LEN 4

#define FEATURE_WINDOW_SIZE 50  // 50 samples = 1 sec if 20ms/sample

// =====================
// System state enums
// =====================
enum SystemMode {
  MODE_NORMAL = 0,
  MODE_DEBUG,
  MODE_LOW_POWER
};

enum MotionState {
  MOTION_STILL = 0,
  MOTION_MOVING,
  MOTION_SHAKING,
  MOTION_IMPACT
};

enum SensorPattern {
  PATTERN_STILL = 0,
  PATTERN_MOVING,
  PATTERN_SHAKING,
  PATTERN_IMPACT
};

// =====================
// Data structures
// =====================
struct SensorSample {
  float ax;
  float ay;
  float az;
  uint32_t timestampMs;
};

struct FeatureWindow {
  float meanX;
  float meanY;
  float meanZ;

  float varX;
  float varY;
  float varZ;

  float energy;
  float maxAbs;
  uint32_t windowStartMs;
  uint32_t windowEndMs;
};

struct InferenceResult {
  MotionState state;
  float confidence;
  float energy;
  float maxAbs;
  uint32_t timestampMs;
};

struct SystemStats {
  uint32_t samplesGenerated;
  uint32_t featuresComputed;
  uint32_t inferencesRun;
  uint32_t hostNotifications;
  uint32_t suppressedNotifications;
  uint32_t sampleQueueDrops;
};

// =====================
// Global system state
// =====================
SystemMode currentMode = MODE_NORMAL;
SensorPattern currentPattern = PATTERN_STILL;
MotionState currentMotionState = MOTION_STILL;

FeatureWindow lastFeature = {};
InferenceResult lastInference = {};
SystemStats stats = {};

// =====================
// RTOS objects
// =====================
TaskHandle_t buttonTaskHandle = nullptr;

SemaphoreHandle_t stateMutex = nullptr;
SemaphoreHandle_t i2cMutex = nullptr;

QueueHandle_t sampleQueue = nullptr;
QueueHandle_t featureQueue = nullptr;
QueueHandle_t inferenceQueue = nullptr;

// =====================
// Helper functions
// =====================
const char* modeToString(SystemMode mode) {
  switch (mode) {
    case MODE_NORMAL:
      return "NORMAL";
    case MODE_DEBUG:
      return "DEBUG";
    case MODE_LOW_POWER:
      return "LOW_POWER";
    default:
      return "UNKNOWN";
  }
}

const char* motionToString(MotionState state) {
  switch (state) {
    case MOTION_STILL:
      return "STILL";
    case MOTION_MOVING:
      return "MOVING";
    case MOTION_SHAKING:
      return "SHAKING";
    case MOTION_IMPACT:
      return "IMPACT";
    default:
      return "UNKNOWN";
  }
}

const char* patternToString(SensorPattern pattern) {
  switch (pattern) {
    case PATTERN_STILL:
      return "STILL";
    case PATTERN_MOVING:
      return "MOVING";
    case PATTERN_SHAKING:
      return "SHAKING";
    case PATTERN_IMPACT:
      return "IMPACT";
    default:
      return "UNKNOWN";
  }
}

bool parseMode(const String& text, SystemMode* modeOut) {
  if (text == "NORMAL") {
    *modeOut = MODE_NORMAL;
    return true;
  }
  if (text == "DEBUG") {
    *modeOut = MODE_DEBUG;
    return true;
  }
  if (text == "LOW_POWER") {
    *modeOut = MODE_LOW_POWER;
    return true;
  }
  return false;
}

bool parsePattern(const String& text, SensorPattern* patternOut) {
  if (text == "STILL") {
    *patternOut = PATTERN_STILL;
    return true;
  }
  if (text == "MOVING") {
    *patternOut = PATTERN_MOVING;
    return true;
  }
  if (text == "SHAKING") {
    *patternOut = PATTERN_SHAKING;
    return true;
  }
  if (text == "IMPACT") {
    *patternOut = PATTERN_IMPACT;
    return true;
  }
  return false;
}

// =====================
// Thread-safe state access
// =====================
SystemMode getModeSafe() {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  SystemMode mode = currentMode;
  xSemaphoreGive(stateMutex);
  return mode;
}

SensorPattern getPatternSafe() {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  SensorPattern pattern = currentPattern;
  xSemaphoreGive(stateMutex);
  return pattern;
}

MotionState getMotionStateSafe() {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  MotionState state = currentMotionState;
  xSemaphoreGive(stateMutex);
  return state;
}

void setModeSafe(SystemMode mode) {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  currentMode = mode;
  xSemaphoreGive(stateMutex);
}

void setPatternSafe(SensorPattern pattern) {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  currentPattern = pattern;
  xSemaphoreGive(stateMutex);
}

void setMotionStateSafe(MotionState state) {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  currentMotionState = state;
  xSemaphoreGive(stateMutex);
}

void updateLastFeatureSafe(const FeatureWindow& feature) {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  lastFeature = feature;
  stats.featuresComputed++;
  xSemaphoreGive(stateMutex);
}

void updateLastInferenceSafe(const InferenceResult& result) {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  lastInference = result;
  stats.inferencesRun++;
  xSemaphoreGive(stateMutex);
}

SystemStats getStatsSafe() {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  SystemStats copy = stats;
  xSemaphoreGive(stateMutex);
  return copy;
}

FeatureWindow getLastFeatureSafe() {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  FeatureWindow copy = lastFeature;
  xSemaphoreGive(stateMutex);
  return copy;
}

InferenceResult getLastInferenceSafe() {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  InferenceResult copy = lastInference;
  xSemaphoreGive(stateMutex);
  return copy;
}

void resetStatsSafe() {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  memset(&stats, 0, sizeof(stats));
  xSemaphoreGive(stateMutex);
}

void incrementSamplesGeneratedSafe() {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  stats.samplesGenerated++;
  xSemaphoreGive(stateMutex);
}

void incrementSampleDropSafe() {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  stats.sampleQueueDrops++;
  xSemaphoreGive(stateMutex);
}

void incrementHostNotificationSafe() {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  stats.hostNotifications++;
  xSemaphoreGive(stateMutex);
}

void incrementSuppressedNotificationSafe() {
  xSemaphoreTake(stateMutex, portMAX_DELAY);
  stats.suppressedNotifications++;
  xSemaphoreGive(stateMutex);
}

void switchModeSafe() {
  xSemaphoreTake(stateMutex, portMAX_DELAY);

  if (currentMode == MODE_NORMAL) {
    currentMode = MODE_DEBUG;
  } else if (currentMode == MODE_DEBUG) {
    currentMode = MODE_LOW_POWER;
  } else {
    currentMode = MODE_NORMAL;
  }

  SystemMode newMode = currentMode;

  xSemaphoreGive(stateMutex);

  Serial.print("[ButtonTask] Mode changed to: ");
  Serial.println(modeToString(newMode));
}

// =====================
// Sensor simulation
// =====================
float randomNoise(float amplitude) {
  return ((float)random(-1000, 1000) / 1000.0f) * amplitude;
}

SensorSample generateSample(SensorPattern pattern) {
  static uint32_t sampleIndex = 0;
  sampleIndex++;

  SensorSample s;
  s.timestampMs = millis();

  float t = sampleIndex * 0.1f;

  switch (pattern) {
    case PATTERN_STILL:
      // 靜止：大約只有 1g 重力 + 小雜訊
      s.ax = 0.00f + randomNoise(0.02f);
      s.ay = 0.00f + randomNoise(0.02f);
      s.az = 1.00f + randomNoise(0.02f);
      break;

    case PATTERN_MOVING:
      // 移動：中等週期變化
      s.ax = 0.25f * sin(t) + randomNoise(0.05f);
      s.ay = 0.18f * cos(t * 0.7f) + randomNoise(0.05f);
      s.az = 1.00f + 0.15f * sin(t * 1.3f) + randomNoise(0.05f);
      break;

    case PATTERN_SHAKING:
      // 搖晃：高能量震盪
      s.ax = 0.90f * sin(t * 3.0f) + randomNoise(0.20f);
      s.ay = 0.80f * cos(t * 2.7f) + randomNoise(0.20f);
      s.az = 1.00f + 0.70f * sin(t * 3.5f) + randomNoise(0.20f);
      break;

    case PATTERN_IMPACT:
      // 撞擊：大部分時間接近靜止，但週期性產生尖峰
      s.ax = 0.00f + randomNoise(0.03f);
      s.ay = 0.00f + randomNoise(0.03f);
      s.az = 1.00f + randomNoise(0.03f);

      if ((sampleIndex % 50) == 0) {
        s.ax += 2.5f;
        s.ay += 1.8f;
        s.az += 2.0f;
      }
      break;

    default:
      s.ax = 0;
      s.ay = 0;
      s.az = 1.0f;
      break;
  }

  return s;
}

// =====================
// Feature extraction
// =====================
FeatureWindow computeFeatureWindow(SensorSample* samples, size_t count) {
  FeatureWindow f = {};

  if (count == 0) {
    return f;
  }

  f.windowStartMs = samples[0].timestampMs;
  f.windowEndMs = samples[count - 1].timestampMs;

  float sumX = 0;
  float sumY = 0;
  float sumZ = 0;

  for (size_t i = 0; i < count; i++) {
    sumX += samples[i].ax;
    sumY += samples[i].ay;
    sumZ += samples[i].az;
  }

  f.meanX = sumX / count;
  f.meanY = sumY / count;
  f.meanZ = sumZ / count;

  float varX = 0;
  float varY = 0;
  float varZ = 0;
  float energy = 0;
  float maxAbs = 0;

  for (size_t i = 0; i < count; i++) {
    float dx = samples[i].ax - f.meanX;
    float dy = samples[i].ay - f.meanY;
    float dz = samples[i].az - f.meanZ;

    varX += dx * dx;
    varY += dy * dy;
    varZ += dz * dz;

    float mag = sqrt(samples[i].ax * samples[i].ax +
                     samples[i].ay * samples[i].ay +
                     samples[i].az * samples[i].az);

    energy += mag * mag;

    if (mag > maxAbs) {
      maxAbs = mag;
    }
  }

  f.varX = varX / count;
  f.varY = varY / count;
  f.varZ = varZ / count;
  f.energy = energy / count;
  f.maxAbs = maxAbs;

  return f;
}

// =====================
// Lightweight classifier
// =====================
InferenceResult classifyFeature(const FeatureWindow& f) {
  InferenceResult r = {};
  r.timestampMs = millis();
  r.energy = f.energy;
  r.maxAbs = f.maxAbs;

  float totalVar = f.varX + f.varY + f.varZ;

  if (f.maxAbs > 2.4f) {
    r.state = MOTION_IMPACT;
    r.confidence = 0.95f;
  } else if (f.energy > 1.8f || totalVar > 0.45f) {
    r.state = MOTION_SHAKING;
    r.confidence = 0.88f;
  } else if (totalVar > 0.025f) {
    r.state = MOTION_MOVING;
    r.confidence = 0.80f;
  } else {
    r.state = MOTION_STILL;
    r.confidence = 0.90f;
  }

  return r;
}

// =====================
// OLED display
// =====================
void updateDisplay() {
  SystemMode mode = getModeSafe();
  SensorPattern pattern = getPatternSafe();
  MotionState motion = getMotionStateSafe();
  InferenceResult inf = getLastInferenceSafe();

  xSemaphoreTake(i2cMutex, portMAX_DELAY);

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("TinyML Sensor Hub");

  display.setCursor(0, 12);
  display.print("Mode: ");
  display.println(modeToString(mode));

  display.setCursor(0, 24);
  display.print("State: ");
  display.println(motionToString(motion));

  display.setCursor(0, 36);
  display.print("Pattern: ");
  display.println(patternToString(pattern));

  display.setCursor(0, 48);
  display.print("Conf: ");
  display.print(inf.confidence, 2);

  display.display();

  xSemaphoreGive(i2cMutex);
}

// =====================
// Host command functions
// =====================
void printHelp() {
  Serial.println();
  Serial.println("Available commands:");
  Serial.println("  HELP");
  Serial.println("  GET_STATUS");
  Serial.println("  GET_STATS");
  Serial.println("  DUMP_FEATURE");
  Serial.println("  RESET_STATS");
  Serial.println("  SET_MODE NORMAL");
  Serial.println("  SET_MODE DEBUG");
  Serial.println("  SET_MODE LOW_POWER");
  Serial.println("  SET_PATTERN STILL");
  Serial.println("  SET_PATTERN MOVING");
  Serial.println("  SET_PATTERN SHAKING");
  Serial.println("  SET_PATTERN IMPACT");
  Serial.println();
}

void printStatus() {
  SystemMode mode = getModeSafe();
  SensorPattern pattern = getPatternSafe();
  MotionState motion = getMotionStateSafe();
  InferenceResult inf = getLastInferenceSafe();

  Serial.println();
  Serial.println("===== STATUS =====");
  Serial.print("Mode: ");
  Serial.println(modeToString(mode));
  Serial.print("Sensor Pattern: ");
  Serial.println(patternToString(pattern));
  Serial.print("Motion State: ");
  Serial.println(motionToString(motion));
  Serial.print("Confidence: ");
  Serial.println(inf.confidence, 3);
  Serial.print("Energy: ");
  Serial.println(inf.energy, 3);
  Serial.print("Max Abs: ");
  Serial.println(inf.maxAbs, 3);
  Serial.println("==================");
  Serial.println();
}

void printStats() {
  SystemStats s = getStatsSafe();

  Serial.println();
  Serial.println("===== STATS =====");
  Serial.print("Samples generated: ");
  Serial.println(s.samplesGenerated);
  Serial.print("Features computed: ");
  Serial.println(s.featuresComputed);
  Serial.print("Inferences run: ");
  Serial.println(s.inferencesRun);
  Serial.print("Host notifications: ");
  Serial.println(s.hostNotifications);
  Serial.print("Suppressed notifications: ");
  Serial.println(s.suppressedNotifications);
  Serial.print("Sample queue drops: ");
  Serial.println(s.sampleQueueDrops);

  if (s.samplesGenerated > 0) {
    float notifyRate = 100.0f * (float)s.hostNotifications / (float)s.samplesGenerated;
    Serial.print("Notify/sample ratio: ");
    Serial.print(notifyRate, 3);
    Serial.println("%");
  }

  Serial.println("=================");
  Serial.println();
}

void dumpFeature() {
  FeatureWindow f = getLastFeatureSafe();

  Serial.println();
  Serial.println("===== FEATURE =====");
  Serial.print("mean: ");
  Serial.print(f.meanX, 3);
  Serial.print(", ");
  Serial.print(f.meanY, 3);
  Serial.print(", ");
  Serial.println(f.meanZ, 3);

  Serial.print("var: ");
  Serial.print(f.varX, 4);
  Serial.print(", ");
  Serial.print(f.varY, 4);
  Serial.print(", ");
  Serial.println(f.varZ, 4);

  Serial.print("energy: ");
  Serial.println(f.energy, 4);

  Serial.print("max_abs: ");
  Serial.println(f.maxAbs, 4);

  Serial.print("window: ");
  Serial.print(f.windowStartMs);
  Serial.print(" -> ");
  Serial.println(f.windowEndMs);

  Serial.println("===================");
  Serial.println();
}

void handleCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();

  if (cmd.length() == 0) {
    return;
  }

  if (cmd == "HELP") {
    printHelp();
    return;
  }

  if (cmd == "GET_STATUS") {
    printStatus();
    return;
  }

  if (cmd == "GET_STATS") {
    printStats();
    return;
  }

  if (cmd == "DUMP_FEATURE") {
    dumpFeature();
    return;
  }

  if (cmd == "RESET_STATS") {
    resetStatsSafe();
    Serial.println("OK RESET_STATS");
    return;
  }

  if (cmd.startsWith("SET_MODE ")) {
    String modeText = cmd.substring(strlen("SET_MODE "));
    modeText.trim();

    SystemMode newMode;
    if (parseMode(modeText, &newMode)) {
      setModeSafe(newMode);
      Serial.print("OK SET_MODE ");
      Serial.println(modeToString(newMode));
    } else {
      Serial.print("ERR Unknown mode: ");
      Serial.println(modeText);
    }
    return;
  }

  if (cmd.startsWith("SET_PATTERN ")) {
    String patternText = cmd.substring(strlen("SET_PATTERN "));
    patternText.trim();

    SensorPattern newPattern;
    if (parsePattern(patternText, &newPattern)) {
      setPatternSafe(newPattern);
      Serial.print("OK SET_PATTERN ");
      Serial.println(patternToString(newPattern));
    } else {
      Serial.print("ERR Unknown pattern: ");
      Serial.println(patternText);
    }
    return;
  }

  Serial.print("ERR Unknown command: ");
  Serial.println(cmd);
  Serial.println("Type HELP for commands.");
}

// =====================
// ISR
// =====================
void IRAM_ATTR buttonISR() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if (buttonTaskHandle != nullptr) {
    vTaskNotifyGiveFromISR(buttonTaskHandle, &xHigherPriorityTaskWoken);
  }

  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

// =====================
// Tasks
// =====================
void ButtonTask(void* parameter) {
  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    vTaskDelay(pdMS_TO_TICKS(30));

    if (digitalRead(BUTTON_PIN) == LOW) {
      switchModeSafe();

      while (digitalRead(BUTTON_PIN) == LOW) {
        vTaskDelay(pdMS_TO_TICKS(10));
      }

      vTaskDelay(pdMS_TO_TICKS(30));
    }
  }
}

void HostCommTask(void* parameter) {
  String line;

  printHelp();

  while (true) {
    while (Serial.available() > 0) {
      char c = Serial.read();

      if (c == '\n' || c == '\r') {
        if (line.length() > 0) {
          handleCommand(line);
          line = "";
        }
      } else {
        line += c;

        if (line.length() > 80) {
          Serial.println("ERR Command too long");
          line = "";
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void SensorTask(void* parameter) {
  while (true) {
    SensorPattern pattern = getPatternSafe();
    SensorSample sample = generateSample(pattern);

    BaseType_t ok = xQueueSend(sampleQueue, &sample, 0);

    if (ok == pdTRUE) {
      incrementSamplesGeneratedSafe();
    } else {
      incrementSampleDropSafe();
    }

    SystemMode mode = getModeSafe();

    if (mode == MODE_LOW_POWER) {
      vTaskDelay(pdMS_TO_TICKS(100));  // 10 Hz
    } else {
      vTaskDelay(pdMS_TO_TICKS(20));   // 50 Hz
    }
  }
}

void FeatureTask(void* parameter) {
  SensorSample window[FEATURE_WINDOW_SIZE];
  size_t count = 0;

  while (true) {
    SensorSample sample;

    if (xQueueReceive(sampleQueue, &sample, portMAX_DELAY) == pdTRUE) {
      window[count++] = sample;

      if (count >= FEATURE_WINDOW_SIZE) {
        FeatureWindow feature = computeFeatureWindow(window, FEATURE_WINDOW_SIZE);
        updateLastFeatureSafe(feature);

        xQueueSend(featureQueue, &feature, portMAX_DELAY);

        count = 0;
      }
    }
  }
}

void InferenceTask(void* parameter) {
  while (true) {
    FeatureWindow feature;

    if (xQueueReceive(featureQueue, &feature, portMAX_DELAY) == pdTRUE) {
      InferenceResult result = classifyFeature(feature);
      updateLastInferenceSafe(result);

      xQueueSend(inferenceQueue, &result, portMAX_DELAY);
    }
  }
}

void EventManagerTask(void* parameter) {
  MotionState lastNotifiedState = MOTION_STILL;
  bool firstResult = true;

  while (true) {
    InferenceResult result;

    if (xQueueReceive(inferenceQueue, &result, portMAX_DELAY) == pdTRUE) {
      bool shouldNotify = false;
      MotionState previous = getMotionStateSafe();

      setMotionStateSafe(result.state);

      if (firstResult) {
        shouldNotify = true;
        firstResult = false;
      } else if (result.state != lastNotifiedState) {
        shouldNotify = true;
      } else if (result.state == MOTION_IMPACT) {
        shouldNotify = true;
      }

      if (shouldNotify) {
        incrementHostNotificationSafe();

        Serial.print("EVENT MOTION ");
        Serial.print(motionToString(previous));
        Serial.print(" -> ");
        Serial.print(motionToString(result.state));
        Serial.print(" conf=");
        Serial.print(result.confidence, 2);
        Serial.print(" energy=");
        Serial.print(result.energy, 2);
        Serial.print(" max=");
        Serial.println(result.maxAbs, 2);

        lastNotifiedState = result.state;
      } else {
        incrementSuppressedNotificationSafe();
      }

      SystemMode mode = getModeSafe();

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
  }
}

void DisplayTask(void* parameter) {
  while (true) {
    updateDisplay();
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// =====================
// Arduino setup / loop
// =====================
void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println("Booting TinyML Sensor Hub Simulator...");

  randomSeed(analogRead(0));

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED init failed");
    while (true) {
      delay(1000);
    }
  }

  stateMutex = xSemaphoreCreateMutex();
  i2cMutex = xSemaphoreCreateMutex();

  sampleQueue = xQueueCreate(SAMPLE_QUEUE_LEN, sizeof(SensorSample));
  featureQueue = xQueueCreate(FEATURE_QUEUE_LEN, sizeof(FeatureWindow));
  inferenceQueue = xQueueCreate(INFERENCE_QUEUE_LEN, sizeof(InferenceResult));

  if (stateMutex == nullptr || i2cMutex == nullptr ||
      sampleQueue == nullptr || featureQueue == nullptr || inferenceQueue == nullptr) {
    Serial.println("Failed to create RTOS objects");
    while (true) {
      delay(1000);
    }
  }

  updateDisplay();

  xTaskCreatePinnedToCore(ButtonTask, "ButtonTask", 2048, nullptr, 4, &buttonTaskHandle, 1);
  xTaskCreatePinnedToCore(HostCommTask, "HostCommTask", 4096, nullptr, 4, nullptr, 1);
  xTaskCreatePinnedToCore(SensorTask, "SensorTask", 4096, nullptr, 3, nullptr, 1);
  xTaskCreatePinnedToCore(FeatureTask, "FeatureTask", 4096, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(InferenceTask, "InferenceTask", 4096, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(EventManagerTask, "EventManagerTask", 4096, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(DisplayTask, "DisplayTask", 4096, nullptr, 1, nullptr, 1);

  attachInterrupt(
    digitalPinToInterrupt(BUTTON_PIN),
    buttonISR,
    FALLING
  );

  Serial.println("System started.");
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}