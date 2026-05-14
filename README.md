# Arduino IDE Library Requirements

This sketch targets the **ESP32** board family and depends on a few external Arduino libraries.

## 1. Board Support — ESP32 Core

Required once per Arduino IDE installation.

1. Open **File → Preferences**
2. In *Additional Board Manager URLs*, add:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Open **Tools → Board → Boards Manager**
4. Search **`esp32`** and install **"esp32" by Espressif Systems** (use version 2.0.x or later)

Then select **Tools → Board → ESP32 Arduino → ESP32 Dev Module**.

## 2. External Libraries

Open **Sketch → Include Library → Manage Libraries…** (`Ctrl+Shift+I`) and install:

| Library | Author | Notes |
|---|---|---|
| Adafruit BME280 Library | Adafruit | Will prompt to install dependencies — accept |
| Adafruit Unified Sensor | Adafruit | Auto-installed as a BME280 dependency |
| LiquidCrystal I2C | Frank de Brabander | Used for the 20×4 I²C LCD |

### Built-in (no install needed)

These are bundled with the ESP32 core — already available once Step 1 is done:

- `WiFi.h`
- `HTTPClient.h`
- `Wire.h`
- `driver/i2s.h`

## 3. Recommended Tools Settings

| Setting | Value |
|---|---|
| Board | ESP32 Dev Module |
| Upload Speed | 921600 |
| CPU Frequency | 240 MHz (WiFi/BT) |
| Flash Frequency | 80 MHz |
| Flash Size | 4 MB (32 Mb) |
| Partition Scheme | Default 4MB with spiffs |
| Core Debug Level | None |

## Troubleshooting

- **"BME280 fail!" in serial monitor** → Confirm I²C address. This sketch uses `0x76`; some boards use `0x77`. Change in `bme.begin(0x76)`.
- **LCD shows blocks / nothing** → Confirm I²C address with an I²C scanner sketch. This code uses `0x27`; some modules are `0x3F`.
- **Compile error on `driver/i2s.h`** → You're not on the ESP32 core. Reselect the board.
- **Upload fails / "Failed to connect"** → Hold the **BOOT** button on the ESP32 while uploading.
