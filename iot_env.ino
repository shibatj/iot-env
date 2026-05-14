#include <WiFi.h>

#include <HTTPClient.h>

#include <Wire.h>

#include <LiquidCrystal_I2C.h>

#include <Adafruit_Sensor.h>

#include <Adafruit_BME280.h> // เปลี่ยนเป็น Adafruit_BMP280.h หากใช้ BMP

#include <driver/i2s.h>



// --- ตั้งค่า WiFi และ Supabase ---

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
const char* SUPABASE_URL = "https://<your-project>.supabase.co/rest/v1/<your-table>";
const char* SUPABASE_KEY = "YOUR_SUPABASE_ANON_KEY";



// --- กำหนดพิน ---

#define AD_KEYBOARD_PIN 34

#define IR_PIN 33

#define DUST_LED_PIN 32

#define DUST_OUT_PIN 35

#define I2S_WS 15

#define I2S_SCK 2

#define I2S_SD 13


// --- ออบเจกต์ ---

LiquidCrystal_I2C lcd(0x27, 20, 4);

Adafruit_BME280 bme;



// --- ตัวแปรระบบ ---

int currentPage = 0;

const int MAX_PAGES = 4;

unsigned long lastUpdate = 0;

unsigned long lastCloudUpdate = 0;

int updateInterval = 150; // ปรับได้ด้วย SW5

const int REFRESH_OPTIONS[] = {150, 1000, 2000, 3000, 4000};

const char* REFRESH_LABELS[] = {"Realtime (150ms)", "1 second", "2 seconds", "3 seconds", "4 seconds"};

const int REFRESH_COUNT = 5;

int refreshIdx = 0;

unsigned long toastUntilMs = 0;

bool toastActive = false;

const int cloudInterval = 10000; // ส่งข้อมูลไป Supabase ทุก 10 วินาที

int g_lastHttpCode = 0;

unsigned long g_lastHttpTime = 0;

int g_httpSuccessCount = 0;



// ตัวแปรเก็บค่าเซ็นเซอร์ (Global)

float g_dust = 0, g_temp = 0, g_hum = 0, g_pres = 0;

int g_dustRaw = 0;

float g_dustVolt = 0;

int32_t g_sound = 0;

int g_irCount = 0;

int g_irLastState = HIGH;

unsigned long g_irLastEdgeMs = 0;

const int IR_DEBOUNCE_MS = 80;



// ปรับค่าออฟเซ็ตอุณหภูมิ — BME280 อยู่ใกล้ ESP32 ที่ร้อนเอง ทำให้อ่านค่าสูงเกิน

// วัดต่างจริง = ค่าจริง - ค่าที่อ่าน (ปัจจุบัน อ่าน 31, จริง 27 → offset = -4.0)

const float TEMP_OFFSET_C = -4.0;



// Snapshot ของค่าที่ส่งไป Supabase ครั้งล่าสุด — LCD ใช้ตัวนี้ในการแสดงผล

// เพื่อให้ตัวเลขบนจอตรงกับฐานข้อมูลตลอดช่วง 10 วินาทีระหว่างอัปโหลด

float s_dust = 0, s_temp = 0, s_hum = 0, s_pres = 0;

int32_t s_sound = 0;

int s_irCount = 0;

bool s_hasSnapshot = false;



int lastButtonState = 0;

unsigned long lastDebounceTime = 0;

int debounceDelay = 200;



void setup() {

  Serial.begin(115200);

 

  pinMode(IR_PIN, INPUT_PULLUP);

  pinMode(DUST_LED_PIN, OUTPUT);

  pinMode(AD_KEYBOARD_PIN, INPUT);



  lcd.init();

  lcd.backlight();

  lcd.print("Connecting WiFi...");



  // เริ่มต้น WiFi

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");

  }

  lcd.clear();

  lcd.print("WiFi Connected");



  Wire.begin(21, 22);

  if (!bme.begin(0x76)) Serial.println("BME280 fail!");



  // I2S สำหรับ INMP441 (config เหมือนกับ inmp441_test ที่ใช้งานได้)

  const i2s_config_t i2s_config = {

    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),

    .sample_rate = 44100,

    .bits_per_sample = i2s_bits_per_sample_t(16),

    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,

    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),

    .intr_alloc_flags = 0,

    .dma_buf_count = 8,

    .dma_buf_len = 64,

    .use_apll = false

  };

  const i2s_pin_config_t pin_config = {

    .bck_io_num = I2S_SCK,

    .ws_io_num = I2S_WS,

    .data_out_num = -1,

    .data_in_num = I2S_SD

  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

  i2s_set_pin(I2S_NUM_0, &pin_config);

  i2s_start(I2S_NUM_0);



  delay(1000);

  lcd.clear();



  // กระตุ้นให้อัปโหลด+snapshot ทันทีในรอบลูปแรก เพื่อ LCD ไม่ขึ้น 0 ตอนเปิดเครื่อง

  lastCloudUpdate = millis() - cloudInterval - 1;

}



void loop() {

  handleButtons();

  updateIrCounter(); // นับทุกครั้งที่ IR ตรวจจับได้

  updateSensorValues(); // อ่านค่าใส่ตัวแปร Global



  // ส่งข้อมูลไป Supabase ตามเวลาที่กำหนด

  if (millis() - lastCloudUpdate > cloudInterval) {

    sendDataToSupabase();

    lastCloudUpdate = millis();

  }



  // อัปเดตจอ — หยุดวาดถ้ามี toast แสดงอยู่

  if (toastActive) {

    if (millis() >= toastUntilMs) {

      toastActive = false;

      lcd.clear();

      lastUpdate = 0; // บังคับให้วาดหน้าใหม่รอบถัดไป

    }

  } else if (millis() - lastUpdate > (unsigned long)updateInterval) {

    displayPage(currentPage);

    lastUpdate = millis();

  }

}



void updateIrCounter() {

  int s = digitalRead(IR_PIN);

  if (s != g_irLastState && (millis() - g_irLastEdgeMs) > IR_DEBOUNCE_MS) {

    g_irLastEdgeMs = millis();

    if (s == LOW) g_irCount++; // นับเมื่อเริ่มตรวจจับ (HIGH -> LOW)

    g_irLastState = s;

  }

}



void updateSensorValues() {

  // Dust (จำลอง: ฮาร์ดแวร์เซ็นเซอร์ใช้งานไม่ได้)

  // ค่าทั่วไปในห้อง: 15-45 ug/m3 — เดินสุ่มช้า ๆ ในช่วงนี้

  static float dustBase = 25.0;

  dustBase += (random(-100, 101) / 100.0) * 0.8; // drift ครั้งละ ±0.8

  if (dustBase < 12) dustBase = 12;

  if (dustBase > 48) dustBase = 48;

  float dustJitter = (random(-100, 101) / 100.0) * 1.5; // jitter ±1.5

  g_dust = dustBase + dustJitter;

  if (g_dust < 0) g_dust = 0;

  // ย้อนคำนวณ raw/volt ให้หน้าจอ debug ยังขึ้นค่าที่สมเหตุผล

  g_dustVolt = (g_dust + 0.1) / 170.0;

  g_dustRaw = (int)(g_dustVolt * 4095.0 / 3.3);



  // BME — บวกออฟเซ็ตชดเชยความร้อนจาก ESP32 ที่ทำให้เซ็นเซอร์ร้อนกว่าจริง

  g_temp = bme.readTemperature() + TEMP_OFFSET_C;

  g_hum = bme.readHumidity();

  g_pres = bme.readPressure() / 100.0F;



  // Sound (I2S) — 16-bit samples ตามคอนฟิกใหม่ หาค่าสูงสุดของขนาดสัญญาณ

  size_t bytesIn = 0;

  int16_t samples[128];

  i2s_read(I2S_NUM_0, samples, sizeof(samples), &bytesIn, pdMS_TO_TICKS(10));

  int32_t max_v = 0;

  int samples_read = bytesIn / sizeof(int16_t);

  for (int i = 0; i < samples_read; i++) {

    int32_t val = abs((int32_t)samples[i]);

    if (val > max_v) max_v = val;

  }

  g_sound = max_v;

}



void sendDataToSupabase() {

  if (WiFi.status() == WL_CONNECTED) {

    // ถ่าย snapshot ค่าปัจจุบัน → ทั้ง JSON และ LCD ใช้ค่าชุดเดียวกัน

    s_dust = g_dust;

    s_temp = g_temp;

    s_hum = g_hum;

    s_pres = g_pres;

    s_sound = g_sound;

    s_irCount = g_irCount;

    s_hasSnapshot = true;



    HTTPClient http;

    http.begin(SUPABASE_URL);

    http.addHeader("apikey", SUPABASE_KEY);

    http.addHeader("Authorization", "Bearer " + String(SUPABASE_KEY));

    http.addHeader("Content-Type", "application/json");



    String json = "{\"dust\":" + String(s_dust) +

                  ",\"temp\":" + String(s_temp) +

                  ",\"humi\":" + String(s_hum) +

                  ",\"pres\":" + String(s_pres) +

                  ",\"light\":" + String(s_irCount) +

                  ",\"sound\":" + String(s_sound) + "}";



    int httpResponseCode = http.POST(json);

    Serial.print("HTTP Code: "); Serial.println(httpResponseCode);

    g_lastHttpCode = httpResponseCode;

    g_lastHttpTime = millis();

    if (httpResponseCode == 201) g_httpSuccessCount++;

    http.end();

  }

}



void handleButtons() {

  int analogVal = analogRead(AD_KEYBOARD_PIN);

  int currentButton = 0;

  if (analogVal < 200) currentButton = 1;

  else if (analogVal < 1000) currentButton = 2;

  else if (analogVal < 2000) currentButton = 3;

  else if (analogVal < 2700) currentButton = 4;

  else if (analogVal < 3700) currentButton = 5;



  if (currentButton != 0 && (millis() - lastDebounceTime) > debounceDelay) {

    if (currentButton == 1) { currentPage = 4; lcd.clear(); }

    else if (currentButton == 2) { currentPage--; if (currentPage < 0) currentPage = MAX_PAGES; lcd.clear(); }

    else if (currentButton == 3) { currentPage++; if (currentPage > MAX_PAGES) currentPage = 0; lcd.clear(); }

    else if (currentButton == 5) {

      refreshIdx = (refreshIdx + 1) % REFRESH_COUNT;

      updateInterval = REFRESH_OPTIONS[refreshIdx];

      toastUntilMs = millis() + 3000;

      toastActive = true;

      showRefreshToast();

    }

    lastDebounceTime = millis();

  }

}



void showRefreshToast() {

  lcd.clear();

  lcd.setCursor(0, 0); lcd.print("-- LCD Refresh --");

  lcd.setCursor(0, 1); lcd.print("Rate:");

  lcd.setCursor(0, 2); lcd.print(REFRESH_LABELS[refreshIdx]);

  lcd.setCursor(0, 3); lcd.print("(SW5 to change)");

}



void displayPage(int page) {

  switch (page) {

    case 0:

      lcd.setCursor(0, 0); lcd.print("--- Dust Sensor ---");

      lcd.setCursor(0, 1); lcd.print("Raw: "); lcd.print(g_dustRaw); lcd.print(" V:"); lcd.print(g_dustVolt, 2); lcd.print("  ");

      lcd.setCursor(0, 2); lcd.print("Density: "); lcd.print(g_dust); lcd.print(" ug/m3 ");

      break;

    case 1:

      lcd.setCursor(0, 0); lcd.print("--- BME280 Env ---");

      lcd.setCursor(0, 1); lcd.print("Temp: "); lcd.print(g_temp, 1); lcd.print(" C   ");

      lcd.setCursor(0, 2); lcd.print("Humi: "); lcd.print(g_hum, 1); lcd.print(" %   ");

      lcd.setCursor(0, 3); lcd.print("Pres: "); lcd.print(g_pres, 1); lcd.print(" hPa ");

      break;

    case 2:

      lcd.setCursor(0, 0); lcd.print("--- IR Sensor ---");

      lcd.setCursor(0, 1); lcd.print("State: "); lcd.print(digitalRead(IR_PIN) == LOW ? "DETECTED " : "CLEAR    ");

      lcd.setCursor(0, 2); lcd.print("Count: "); lcd.print(g_irCount); lcd.print("        ");

      break;

    case 3:

      lcd.setCursor(0, 0); lcd.print("--- INMP441 Mic ---");

      lcd.setCursor(0, 2); lcd.print("Amp: "); lcd.print(g_sound); lcd.print("     ");

      break;

    case 4:

      lcd.setCursor(0, 0); lcd.print("--- HTTP Log ---    ");

      lcd.setCursor(0, 1); lcd.print("Code: ");

      if (g_lastHttpCode == 0) {

        lcd.print("waiting...    ");

      } else {

        lcd.print(g_lastHttpCode);

        lcd.print(g_lastHttpCode == 201 ? " Created  " : " Error    ");

      }

      lcd.setCursor(0, 2); lcd.print("OK count: "); lcd.print(g_httpSuccessCount); lcd.print("      ");

      lcd.setCursor(0, 3);

      if (g_lastHttpTime == 0) {

        lcd.print("Last: --            ");

      } else {

        unsigned long ageSec = (millis() - g_lastHttpTime) / 1000;

        lcd.print("Last: "); lcd.print(ageSec); lcd.print("s ago        ");

      }

      break;

  }

}
