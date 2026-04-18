#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <sh1106.h>
#include <DHT.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C

#define DHT_PIN 7
#define DHT_TYPE DHT11
#define BATTERY_PIN 26
#define BATTERY_DIVIDER_RATIO 2.0f
#define ADC_MAX_VALUE 4095.0f
#define ADC_REF_VOLTAGE 3.3f
#define BATTERY_MIN_VOLTAGE 3.00f
#define BATTERY_MAX_VOLTAGE 4.20f
#define BATTERY_SAMPLES 8
// PM2.5 level thresholds (µg/m3) for 4-level indicator
#define PM2_5_LEVEL_GOOD 35
#define PM2_5_LEVEL_MODERATE 100
#define PM2_5_LEVEL_HIGH 300
#define PM_BLINK_INTERVAL_MS 500

static uint8_t oledBuffer[SCREEN_WIDTH * SCREEN_HEIGHT / 8];
sh1106 display(SCREEN_WIDTH, SCREEN_HEIGHT);
bool displayOk = false;

float temperature = NAN;
float humidity = NAN;
float batteryVoltage = 0.0f;
unsigned long lastSensorUpdateMs = 0;
const unsigned long SENSOR_UPDATE_INTERVAL_MS = 2000;

// PMS5003 on UART pins GP0=RX, GP1=TX
// Serial1 is the hardware UART for these pins on RP2040.

struct PMSData {
  uint16_t pm1_0;
  uint16_t pm2_5;
  uint16_t pm10_0;
  bool valid;
};

PMSData pms = {0, 0, 0, false};
DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(115200);

  Serial.println("PMMeter-RP2040 starting...");

  Wire.begin();
  analogReadResolution(12);
  pinMode(BATTERY_PIN, INPUT);
  dht.begin();

  if (display.setup(Wire, OLED_ADDRESS, OLED_RESET, oledBuffer) != 0) {
    Serial.println("OLED init failed");
    displayOk = false;
  } else {
    displayOk = true;
    Serial.println("OLED init succeeded");
    display.clear();
    display.setTextSize(1);
    display.setTextColor(1);
    display.setCursor(0, 12);
    display.println("PMMeter-RP2040");
    display.println("Display test...");
    display.println("M130-12864-4G");
    display.display();
    delay(1000);

    display.clear();
    display.setCursor(0, 12);
    display.println("Waiting for PMS5003 data...");
    display.display();
  }

  Serial1.begin(9600);
  updateEnvironmentalSensors();
  drawDisplay(pms);
}

bool readPMSFrame(PMSData &data) {
  static enum {WAIT_HEADER1, WAIT_HEADER2, READ_LENGTH, READ_PAYLOAD} state = WAIT_HEADER1;
  static uint8_t buffer[32];
  static uint8_t index = 0;
  static uint16_t frameLength = 0;

  while (Serial1.available()) {
    uint8_t b = Serial1.read();

    switch (state) {
      case WAIT_HEADER1:
        if (b == 0x42) {
          state = WAIT_HEADER2;
        }
        break;

      case WAIT_HEADER2:
        if (b == 0x4D) {
          state = READ_LENGTH;
          index = 0;
        } else if (b != 0x42) {
          state = WAIT_HEADER1;
        }
        break;

      case READ_LENGTH:
        buffer[index++] = b;
        if (index == 2) {
          frameLength = (buffer[0] << 8) | buffer[1];
          if (frameLength != 28) {
            state = WAIT_HEADER1;
          } else {
            index = 0;
            state = READ_PAYLOAD;
          }
        }
        break;

      case READ_PAYLOAD:
        buffer[index++] = b;
        if (index == frameLength) {
          uint16_t checksum = 0x42 + 0x4D + 0x00 + 0x1C;
          for (uint8_t i = 0; i < frameLength - 2; i++) {
            checksum += buffer[i];
          }
          uint16_t frameChecksum = (buffer[frameLength - 2] << 8) | buffer[frameLength - 1];

          if (checksum == frameChecksum) {
            data.pm1_0 = (buffer[0] << 8) | buffer[1];
            data.pm2_5 = (buffer[2] << 8) | buffer[3];
            data.pm10_0 = (buffer[4] << 8) | buffer[5];
            data.valid = true;
            char debug[64];
            snprintf(debug, sizeof(debug), "PMS5003 data: PM1.0=%u PM2.5=%u PM10=%u", data.pm1_0, data.pm2_5, data.pm10_0);
            Serial.println(debug);
          } else {
            Serial.println("PMS5003 checksum mismatch");
          }
          state = WAIT_HEADER1;
          return data.valid;
        }
        break;
    }
  }

  return false;
}

float readBatteryVoltage() {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < BATTERY_SAMPLES; i++) {
    sum += analogRead(BATTERY_PIN);
    delay(5);
  }
  float average = sum / (float)BATTERY_SAMPLES;
  return average * (ADC_REF_VOLTAGE / ADC_MAX_VALUE) * BATTERY_DIVIDER_RATIO;
}

void updateEnvironmentalSensors() {
  float newTemp = dht.readTemperature();
  float newHum = dht.readHumidity();

  if (!isnan(newTemp) && !isnan(newHum)) {
    temperature = newTemp;
    humidity = newHum;
    char debug[64];
    snprintf(debug, sizeof(debug), "DHT11: T=%.1fC H=%.1f%%", temperature, humidity);
    Serial.println(debug);
  } else {
    Serial.println("DHT11 read failed");
  }

  batteryVoltage = readBatteryVoltage();
  char battDebug[64];
  snprintf(battDebug, sizeof(battDebug), "Battery: %.2f V", batteryVoltage);
  Serial.println(battDebug);
}

float batteryPercent() {
  float pct = (batteryVoltage - BATTERY_MIN_VOLTAGE) / (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE) * 100.0f;
  if (pct < 0.0f) pct = 0.0f;
  if (pct > 100.0f) pct = 100.0f;
  return pct;
}

// Map battery percent to discrete 4-level indicator (0-4)
uint8_t batteryLevel() {
  float pct = batteryPercent();
  if (pct <= 0.0f) return 0;
  if (pct <= 25.0f) return 1;
  if (pct <= 50.0f) return 2;
  if (pct <= 75.0f) return 3;
  return 4;
}

// Draw a battery icon with 'level' filled segments (0..4)
void drawBatteryIcon(int x, int y, int w, int h, uint8_t level) {
  // New compact 4-segment battery styled like reference image
  const int segCount = 4;
  int terminalW = (w >= 32) ? 4 : 3;
  // Need some room for terminal and 1px padding on each side
  if (w <= terminalW + segCount + 4 || h <= 8) return;

  int bodyW = w - terminalW;
  int radius = (bodyW >= 36) ? 3 : 2;
  // Outer rounded frame (body only)
  display.drawRoundRect(x, y, bodyW, h, radius, 1);

  // Terminal (outline) on the right
  int termX = x + bodyW;
  int termY = y + (h / 4);
  int termH = max(1, h / 2);
  display.drawRect(termX, termY, terminalW, termH, 1);

  // Inner area with exactly 1px padding on top/bottom and separate head/tail gaps
  const int padding = 1;
  const int headTailGap = 1; // default gap applied to tail; head will be reduced
  int headGap = headTailGap + 1; // reduce head gap by 1px
  if (headGap < 0) headGap = 0;
  const int tailGap = headTailGap;
  int innerX = x + padding + headGap;
  int innerY = y + padding;
  int innerW = bodyW - padding * 2 - headGap - tailGap;
  if (innerW < 1) innerW = 1;
  int innerH = h - padding * 2;

  const int segSpacing = 1; // 1px gap between segments
  int segW = (innerW - (segCount - 1) * segSpacing) / segCount;
  if (segW < 1) segW = 1;

  // inset segments vertically by 1px so there's a 1px top/bottom gap inside the outer frame
  int segInnerY = innerY + 1;
  int segInnerH = innerH - 2;
  if (segInnerH < 1) segInnerH = 1;

  for (int i = 0; i < segCount; i++) {
    int sx = innerX + i * (segW + segSpacing);
    // draw segment outline (inset vertically)
    display.drawRect(sx, segInnerY, segW, segInnerH, 1);
    if (i < level) {
      // fill the interior of the segment leaving the 1px outline intact
      if (segW > 2 && segInnerH > 2) {
        display.fillRect(sx + 1, segInnerY + 1, segW - 2, segInnerH - 2, 1);
      } else {
        display.fillRect(sx, segInnerY, segW, segInnerH, 1);
      }
    }
  }
}

// Draw PM2.5 safety indicator as a textual 4-level label
// Levels: GOOD, MODERATE, UNHEALTHY, HAZARDOUS
// If PM2.5 > MODERATE, the background will blink to draw attention.
void drawPMIndicator(int x, int y, int w, int h, uint16_t pmValue, bool valid) {
  display.setTextSize(1);
  int radius = 2;

  if (!valid) {
    display.drawRoundRect(x, y, w, h, radius, 1);
    int tx = x + 2;
    int ty = y + (h - 8) / 2;
    display.setCursor(tx, ty);
    display.print("--");
    return;
  }

  const char *label;
  int severity = 0; // 0:GOOD, 1:MODERATE, 2:UNHEALTHY, 3:HAZARDOUS
  if (pmValue <= PM2_5_LEVEL_GOOD) { label = "GOOD"; severity = 0; }
  else if (pmValue <= PM2_5_LEVEL_MODERATE) { label = "MODERATE"; severity = 1; }
  else if (pmValue <= PM2_5_LEVEL_HIGH) { label = "UNHEALTHY"; severity = 2; }
  else { label = "HAZARDOUS"; severity = 3; }

  bool shouldBlink = (pmValue > PM2_5_LEVEL_MODERATE);
  bool blinkOn = ((millis() / PM_BLINK_INTERVAL_MS) & 1) == 1;

  int textW = strlen(label) * 6; // approx width per char at textSize=1
  int tx = x + (w - textW) / 2;
  int ty = y + (h - 8) / 2;

  if (shouldBlink && severity >= 2) {
    // toggle between filled background (attention) and outline
    if (blinkOn) {
      display.fillRoundRect(x, y, w, h, radius, 1);
      display.setTextColor(0, 1);
      display.setCursor(tx, ty);
      display.print(label);
      display.setTextColor(1);
    } else {
      display.drawRoundRect(x, y, w, h, radius, 1);
      display.setCursor(tx, ty);
      display.print(label);
    }
  } else {
    // normal rendering (no blink) -- hazardous still emphasized if not blinking
    if (severity == 3) {
      display.fillRoundRect(x, y, w, h, radius, 1);
      display.setTextColor(0, 1);
      display.setCursor(tx, ty);
      display.print(label);
      display.setTextColor(1);
    } else {
      display.drawRoundRect(x, y, w, h, radius, 1);
      display.setCursor(tx, ty);
      display.print(label);
    }
  }
}

void drawDisplay(const PMSData &data) {
  if (!displayOk) {
    return;
  }

  display.clear();
  display.setTextColor(1);

  // Top-left: Temperature and Humidity (larger, professional layout)
  if (!isnan(temperature) && !isnan(humidity)) {
    display.setTextSize(2);
    display.setCursor(2, 12);
    display.print(temperature, 1);
    display.print("C");

    // Humidity below temperature
    int humY = 12 + 18; // spacing for size 2 text
    display.setCursor(2, humY);
    display.print(humidity, 1);
    display.print("%");
  } else {
    display.setTextSize(1);
    display.setCursor(2, 12);
    display.print("DHT11 error");
  }

  // PM2.5 value (right side)
  display.setTextSize(1);
  display.setCursor(78, 12);
  display.print("PM2.5");
  display.setTextSize(2);
  display.setCursor(78, 24);
  if (data.valid) {
    display.print(data.pm2_5);
  } else {
    display.print("--");
  }

  // Separator line
  display.drawFastHLine(0, 46, SCREEN_WIDTH, 1);

  // Battery icon: compact and bottom-left, reduced size for proportion
  uint8_t batt_level = batteryLevel();
  const int battW = 28; // reduced for proportional bottom placement (shrunk 1px from right)
  const int battH = 10;
  const int margin = 4;
  const int battX = margin;
  const int battY = SCREEN_HEIGHT - battH - margin;
  drawBatteryIcon(battX, battY, battW, battH, batt_level);
  // Battery voltage display removed; icon shows only battery level

  // PM2.5 safety indicator (text) at bottom-right
  const int indW = 60; // wide enough for labels like "HAZARDOUS"
  const int indH = battH; // match battery height for balance
  const int indX = SCREEN_WIDTH - indW - margin;
  const int indY = SCREEN_HEIGHT - indH - margin;
  drawPMIndicator(indX, indY, indW, indH, data.pm2_5, data.valid);

  if (!data.valid) {
    display.setTextSize(1);
    display.setCursor(0, 36);
    display.print("Waiting for PMS5003 data...");
  }

  display.display();
}

void loop() {
  bool redraw = false;

  if (readPMSFrame(pms)) {
    redraw = true;
  }

  if (millis() - lastSensorUpdateMs >= SENSOR_UPDATE_INTERVAL_MS) {
    lastSensorUpdateMs = millis();
    updateEnvironmentalSensors();
    redraw = true;
  }

  // If PM2.5 is above MODERATE, trigger periodic redraws for blinking
  static unsigned long lastBlinkMs = 0;
  if (pms.valid && pms.pm2_5 > PM2_5_LEVEL_MODERATE) {
    if (millis() - lastBlinkMs >= PM_BLINK_INTERVAL_MS) {
      lastBlinkMs = millis();
      redraw = true;
    }
  }

  if (redraw) {
    drawDisplay(pms);
  }
}
