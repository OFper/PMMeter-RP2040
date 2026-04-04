#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <sh1106.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C

static uint8_t oledBuffer[SCREEN_WIDTH * SCREEN_HEIGHT / 8];
sh1106 display(SCREEN_WIDTH, SCREEN_HEIGHT);
bool displayOk = false;

// PMS5003 on UART pins GP0=RX, GP1=TX
// Serial1 is the hardware UART for these pins on RP2040.

struct PMSData {
  uint16_t pm1_0;
  uint16_t pm2_5;
  uint16_t pm10_0;
  bool valid;
};

PMSData pms = {0, 0, 0, false};

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;
  }

  Serial.println("PMMeter-RP2040 starting...");

  Wire.begin();

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

void drawDisplay(const PMSData &data) {
  if (!displayOk) {
    return;
  }

  display.clear();

  if (data.valid) {
    display.setTextSize(1);
    display.setTextColor(1);
    display.setCursor(0, 12);
    display.print("PM1.0:");
    display.setTextSize(2);
    display.setCursor(70, 10);
    display.print(data.pm1_0);
    display.setTextSize(1);
    display.print(" ug/m3");

    display.setTextSize(1);
    display.setCursor(0, 30);
    display.print("PM2.5:");
    display.setTextSize(2);
    display.setCursor(70, 28);
    display.print(data.pm2_5);
    display.setTextSize(1);
    display.print(" ug/m3");

    display.setTextSize(1);
    display.setCursor(0, 48);
    display.print("PM10:");
    display.setTextSize(2);
    display.setCursor(70, 46);
    display.print(data.pm10_0);
    display.setTextSize(1);
    display.print(" ug/m3");
  } else {
    display.setTextSize(1);
    display.setTextColor(1);
    display.setCursor(0, 12);
    display.println("Waiting for PMS5003 data...");
  }

  display.display();
}

void loop() {
  if (readPMSFrame(pms)) {
    drawDisplay(pms);
  }
}
