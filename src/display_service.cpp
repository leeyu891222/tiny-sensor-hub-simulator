#include "display_service.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "app_types.h"
#include "system_state.h"
#include "app_config.h"



static Adafruit_SSD1306 display(
  APP_SCREEN_WIDTH,
  APP_SCREEN_HEIGHT,
  &Wire,
  APP_OLED_RESET
);
static SemaphoreHandle_t g_i2cMutex = nullptr;

bool DisplayService_Init() {
  g_i2cMutex = xSemaphoreCreateMutex();

  if (g_i2cMutex == nullptr) {
    return false;
  }

    Wire.begin(APP_I2C_SDA_PIN, APP_I2C_SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, APP_OLED_ADDR)) {
    return false;
  }

  display.clearDisplay();
  display.display();

  return true;
}

void DisplayService_Update() {
  if (g_i2cMutex == nullptr) {
    return;
  }

  SystemMode mode = SystemState_GetMode();
  SensorPattern pattern = SystemState_GetPattern();
  MotionState motion = SystemState_GetMotionState();
  InferenceResult inf = SystemState_GetLastInference();

  xSemaphoreTake(g_i2cMutex, portMAX_DELAY);

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

  xSemaphoreGive(g_i2cMutex);
}