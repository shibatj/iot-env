# IoT Environmental Monitor (ESP32 + Supabase)

An ESP32-based environmental monitoring station that reads multiple sensors, displays live data on a 20×4 LCD, and pushes readings to a Supabase database every 10 seconds.

![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![ESP32 Core](https://img.shields.io/badge/esp32%20core-2.0.17-red)
![Language](https://img.shields.io/badge/language-C%2B%2B-orange)
![Framework](https://img.shields.io/badge/framework-Arduino-00979D)
![Cloud](https://img.shields.io/badge/cloud-Supabase-3ECF8E)

## Features

- 📊 **Multi-sensor monitoring** — temperature, humidity, pressure, dust, sound level, and IR motion count
- 📺 **5-page LCD UI** — navigate between sensors using an analog button keypad
- ☁️ **Cloud sync** — pushes JSON readings to Supabase REST API every 10s
- ⏱️ **Adjustable refresh rate** — toggle between 150 ms (realtime) and 4 s
- 🌡️ **Temperature offset compensation** — corrects for ESP32 self-heating affecting BME280
- 📈 **HTTP status page** — view last response code, success count, and time since last upload
- 🔄 **Snapshot-consistent display** — LCD shows the same values that were last sent to the cloud, so the screen and database stay in sync

## Hardware

| Component | Purpose |
|---|---|
| ESP32 dev board | MCU + WiFi |
| BME280 | Temperature, humidity, pressure (I²C `0x76`) |
| LiquidCrystal I2C 20×4 | Display (I²C `0x27`) |
| INMP441 | Digital MEMS microphone (I²S) |
| IR sensor | Motion / object counter |
| Dust sensor (GP2Y1014AU or similar) | Particulate density *(currently simulated — see Notes)* |
| AD keyboard module | 5 buttons on a single analog pin |

### Pinout

| Pin | Function |
|---|---|
| GPIO 21 | I²C SDA (BME280 + LCD) |
| GPIO 22 | I²C SCL (BME280 + LCD) |
| GPIO 34 | AD keyboard (analog in) |
| GPIO 33 | IR sensor input |
| GPIO 32 | Dust sensor LED control |
| GPIO 35 | Dust sensor analog out |
| GPIO 15 | I²S WS (LRCLK) |
| GPIO 2  | I²S SCK (BCLK) |
| GPIO 13 | I²S SD (data in) |

## Software Dependencies

Install via Arduino Library Manager:

- `WiFi` (built-in with ESP32 core)
- `HTTPClient` (built-in with ESP32 core)
- `Wire` (built-in)
- `LiquidCrystal_I2C` by Frank de Brabander
- `Adafruit_Sensor`
- `Adafruit_BME280`
- `driver/i2s.h` (ESP-IDF, ships with ESP32 core)

You'll also need the **ESP32 board package** in Arduino IDE (Boards Manager → "esp32" by Espressif Systems).

> ⚠️ **Use ESP32 core version `2.0.17`.** This sketch uses the legacy I²S driver API (`I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB`), which was deprecated and removed in core **3.x** and causes a runtime `abort()` during `i2s_driver_install()`. In Boards Manager, search "esp32" and select version **2.0.17** from the dropdown before installing.

## Setup

### 1. Clone the repo

```bash
git clone https://github.com/<your-username>/<repo-name>.git
cd <repo-name>
```

### 2. Configure credentials

> ⚠️ **Security note:** Never commit real WiFi passwords or API keys to a public repo. Use a `secrets.h` file added to `.gitignore`, or environment-based config.

Open `iot_env.ino` and replace the placeholder values:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
const char* SUPABASE_URL = "https://<your-project>.supabase.co/rest/v1/<your-table>";
const char* SUPABASE_KEY = "YOUR_SUPABASE_ANON_KEY";
```

### 3. Create the Supabase table

In your Supabase project, create a table (e.g. `sensor_data`) with these columns:

| Column | Type |
|---|---|
| `id` | `int8` (primary key, identity) |
| `created_at` | `timestamptz` (default `now()`) |
| `dust`  | `float8` |
| `temp`  | `float8` |
| `humi`  | `float8` |
| `pres`  | `float8` |
| `light` | `int4` |
| `sound` | `int8` |

Make sure RLS policies allow `INSERT` for the anon role, or use a service key (server-side only).

### 4. Upload to ESP32

Select your board (e.g. *ESP32 Dev Module*) and the correct COM port, then click **Upload**.

## LCD Pages

| Page | Content |
|---|---|
| 0 | Dust sensor — raw ADC, voltage, density (µg/m³) |
| 1 | BME280 — temperature (°C), humidity (%), pressure (hPa) |
| 2 | IR sensor — current state + trigger count |
| 3 | INMP441 microphone — peak amplitude |
| 4 | HTTP log — last response code, success count, time since last POST |

## Button Controls (AD Keyboard)

| Button | Action |
|---|---|
| SW1 | Jump to HTTP log page (page 4) |
| SW2 | Previous page |
| SW3 | Next page |
| SW4 | Cycle LCD refresh rate (150 ms → 1 s → 2 s → 3 s → 4 s) |
| SW5 | Clear IR sensor count (reset to 0) |

## Notes & Caveats

- **Dust sensor is simulated.** The hardware code path is wired up, but the current build generates plausible random values (12–48 µg/m³) because the physical sensor isn't operational. To re-enable real readings, replace the simulation block in `updateSensorValues()` with ADC sampling logic for your sensor.
- **Temperature offset.** `TEMP_OFFSET_C = -4.0` compensates for heat from the ESP32 chip warming the nearby BME280. Recalibrate this value for your enclosure by comparing against a known reference thermometer.
- **Snapshot timing.** Every cloud upload takes a "snapshot" of all sensor values; the LCD displays from that snapshot until the next upload (10 s later). This keeps the screen in sync with the database, but means the LCD updates effectively at the cloud-upload rate for cloud-sourced fields.
- **WiFi blocking.** `setup()` blocks indefinitely until WiFi connects. Add a timeout if you want the device to boot in offline mode.

## Project Structure

```
.
├── iot_env.ino       # Main Arduino sketch
└── README.md         # You are here
```

## License

MIT — see [LICENSE](LICENSE) for details.

## Acknowledgements

- [Adafruit](https://www.adafruit.com/) for the BME280 library
- [Supabase](https://supabase.com/) for the free-tier backend
- ESP-IDF I²S driver examples for the INMP441 configuration


# Arduino IDE Library Requirements

This sketch targets the **ESP32** board family and depends on a few external Arduino libraries.

## 1. Board Support — ESP32 Core

Required once per Arduino IDE installation.

> ⚠️ **Install version `2.0.17` specifically — not 3.x.**
> This sketch uses the legacy I²S driver API which was removed in ESP32 core 3.x and will cause a runtime `abort()` during boot. Version 2.0.17 is the last stable 2.x release.

1. Open **File → Preferences**
2. In *Additional Board Manager URLs*, add:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Open **Tools → Board → Boards Manager**
4. Search **`esp32`** → find **"esp32" by Espressif Systems** → in the version dropdown, select **`2.0.17`** → click **Install**

If you already have 3.x installed, click on it in Boards Manager and choose **Remove** first, then install 2.0.17.

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
