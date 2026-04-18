# PMMeter-RP2040

โปรเจคนี้เป็นเฟิร์มแวร์สำหรับบอร์ด RP2040 (Raspberry Pi Pico) ที่อ่านข้อมูลฝุ่น PM จากเซนเซอร์ PMS5003 และแสดงผลบนจอ OLED 128x64.

## สิ่งที่มีอยู่ในโปรเจค

- `platformio.ini` - กำหนดบอร์ด `pico` และ dependency ที่ใช้
- `src/PMMeter-RP2040.ino` - โค้ด Arduino / PlatformIO สำหรับอ่าน PMS5003 และแสดงผลบน SH1106 OLED

## สายต่อหลัก

- PMS5003:
  - `RX` -> `GP0` (บอร์ด RP2040)
  - `TX` -> `GP1`
  - กราวด์และไฟเลี้ยงตามปกติ

- DHT11:
  - `DATA` -> `GP7`
  - `VCC` -> `3.3V` หรือ `5V` ตามโมดูล
  - `GND` -> `GND`

- voltage divider สำหรับวัดแบตเตอรี่ 18650:
  - สัญญาณเอาต์พุตต่อเข้ากับ `GP26`
  - กราวด์และไฟเลี้ยงเชื่อมต่อกับแบตเตอรี่และตัวต้านทานแบ่งแรงดันตามปกติ

- OLED M130-12864-4G (SH1106):
  - `SDA` -> `GP4`
  - `SCL` -> `GP5`
  - `VCC` -> `3.3V`
  - `GND` -> `GND`

> ปัจจุบันหน้าจอมีส่วนบน 10 พิกเซลเสียบางส่วน ดังนั้นเฟิร์มแวร์จะเริ่มวาดข้อความที่สูงกว่า 10 พิกเซลขึ้นไป

## ไลบรารีที่ใช้

- `Adafruit GFX Library`
- `Sitron_Labs_SH1106_Arduino_Library`
- `Adafruit Unified Sensor`
- `Adafruit DHT sensor library`

## วิธีติดตั้งและคอมไพล์

ใช้ PlatformIO ในโฟลเดอร์โปรเจค:

```bash
cd /Users/pongsak/development/PMMeter-RP2040
python3 -m platformio run
```

## วิธีอัปโหลด

เชื่อมต่อบอร์ด Pico กับคอมพิวเตอร์ แล้วใช้คำสั่ง:

```bash
python3 -m platformio run -t upload --upload-port /dev/cu.usbmodem11101
```

ถ้าใช้พอร์ตอื่น ให้เปลี่ยน `--upload-port` เป็นพอร์ตที่ถูกต้อง

## สถานะปัจจุบัน

- อ่านข้อมูล PMS5003 และแสดงค่า PM1.0, PM2.5, PM10 บน OLED
- อ่านค่า DHT11 บน GP7 และแสดงค่าอุณหภูมิ/ความชื้น
- อ่านแรงดันแบตเตอรี่จาก ADC GP26 ผ่าน voltage divider และแสดงค่าบน OLED
- มีการเลี่ยงการแสดงผลบริเวณ 10 พิกเซลด้านบนของจอ

## หมายเหตุ

- ถ้า OLED ไม่ติด ให้ตรวจสอบว่าเป็นจอ SH1106 หรือ SSD1306
- ถ้าใช้สายหรือตัวต่อไม่เหมาะสม อาจทำให้จอไม่แสดงผล
- โค้ดปัจจุบันใช้งาน `Sitron_Labs_SH1106_Arduino_Library` และมีการแพตช์ไฟล์ใน
  `.pio/libdeps/pico/Sitron_Labs_SH1106_Arduino_Library/src/sh1106.cpp`
  เพื่อแก้ปัญหา compile กับ `Wire.write()` บน RP2040

  ตัวอย่าง diff ที่แก้ไข:
  ```diff
  @@
  -            m_i2c_library->write(0x80);  // CO = 1, DC = 0
  -            m_i2c_library->write(COMMAND_PAGE_ADDRESS + (y_panel / 8));
  -            m_i2c_library->write(0x80);  // CO = 1, DC = 0
  -            m_i2c_library->write(COMMAND_COLUMN_ADDRESS_L | ((x_panel + 2) & 0x0F));
  -            m_i2c_library->write(0x80);  // CO = 1, DC = 0
  -            m_i2c_library->write(COMMAND_COLUMN_ADDRESS_H | ((x_panel + 2) >> 4));
  -            m_i2c_library->write(0x80);  // CO = 1, DC = 0
  -            m_i2c_library->write(COMMAND_READWRITEMODIFY_BEGIN);
  -            m_i2c_library->write(0x40);  // CO = 0, DC = 1
  +            m_i2c_library->write((uint8_t)0x80);  // CO = 1, DC = 0
  +            m_i2c_library->write((uint8_t)(COMMAND_PAGE_ADDRESS + (y_panel / 8)));
  +            m_i2c_library->write((uint8_t)0x80);  // CO = 1, DC = 0
  +            m_i2c_library->write((uint8_t)(COMMAND_COLUMN_ADDRESS_L | ((x_panel + 2) & 0x0F)));
  +            m_i2c_library->write((uint8_t)0x80);  // CO = 1, DC = 0
  +            m_i2c_library->write((uint8_t)(COMMAND_COLUMN_ADDRESS_H | ((x_panel + 2) >> 4)));
  +            m_i2c_library->write((uint8_t)0x80);  // CO = 1, DC = 0
  +            m_i2c_library->write((uint8_t)COMMAND_READWRITEMODIFY_BEGIN);
  +            m_i2c_library->write((uint8_t)0x40);  // CO = 0, DC = 1
  @@
  -            m_i2c_library->write(0xC0);  // CO = 1, DC = 1
  +            m_i2c_library->write((uint8_t)0xC0);  // CO = 1, DC = 1
  @@
  -                m_i2c_library->write((1 << (y_panel % 8)) | data);
  +                m_i2c_library->write((uint8_t)((1 << (y_panel % 8)) | data));
  @@
  -                m_i2c_library->write(~(1 << (y_panel % 8)) & data);
  +                m_i2c_library->write((uint8_t)(~(1 << (y_panel % 8)) & data));
  @@
  -            m_i2c_library->write(0x00);  // CO = 0, DC = 0
  +            m_i2c_library->write((uint8_t)0x00);  // CO = 0, DC = 0
  ```
