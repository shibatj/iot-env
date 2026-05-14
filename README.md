# IoT Environmental Monitor (ESP32 + Supabase)

An ESP32-based environmental monitoring station that reads multiple sensors, displays live data on a 20×4 LCD, and pushes readings to a Supabase database every 10 seconds.

![Platform](https://img.shields.io/badge/platform-ESP32-blue)
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
| SW5 | Cycle LCD refresh rate (150 ms → 1 s → 2 s → 3 s → 4 s) |

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
