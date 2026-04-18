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

void drawDisplay(const PMSData &data) {
  if (!displayOk) {
    return;
  }

  display.clear();
  display.setTextColor(1);

  // Battery status
  display.setTextSize(1);
  display.setCursor(0, 12);
  display.print("BAT");
  display.setTextSize(2);
  display.setCursor(0, 24);
  display.print(batteryPercent(), 0);
  display.print("%");

  // PM2.5 value
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

  // Temperature / Humidity
  display.setTextSize(1);
  if (!isnan(temperature) && !isnan(humidity)) {
    display.setCursor(0, 52);
    display.print("TEMP ");
    display.print(temperature, 1);
    display.print("C");

    display.setCursor(72, 52);
    display.print("HUM ");
    display.print(humidity, 1);
    display.print("%");
  } else {
    display.setCursor(0, 52);
    display.print("DHT11 error");
  }

  if (!data.valid) {
    display.setTextSize(1);
    display.setCursor(0, 36);
    display.print("Waiting for PMS5003 data...");
  }

  display.display();
}

void loop() {
  if (readPMSFrame(pms)) {
    drawDisplay(pms);
  }

  if (millis() - lastSensorUpdateMs >= SENSOR_UPDATE_INTERVAL_MS) {
    lastSensorUpdateMs = millis();
    updateEnvironmentalSensors();
    drawDisplay(pms);
  }
}
