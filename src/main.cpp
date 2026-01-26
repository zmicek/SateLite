#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "ScioSense_ENS160.h"
#include <PTSolns_AHTx.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h> 
#include <ESP32Time.h>
#include "esp_bt.h"
#include "esp_bt_main.h"
#include <WiFi.h>
#include "esp_wifi.h"

#define BUTTON_PIN GPIO_NUM_19

#define OLED_MOSI 13
#define OLED_CLK  14
#define OLED_DC   26
#define OLED_CS   27
#define OLED_RST  25

#define SCREEN_WIDTH 64    // OLED display width, in pixels
#define SCREEN_HEIGHT 128  // OLED display height, in pixels
#define OLED_RESET -1      // can set an oled reset pin if desired

Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RST, OLED_CS);
void drawEye(int eyeX, int eyeY, int eyeWidth, int eyeHeight);

// Eye position and size
int leftEyeX = 20;       // X position of the left eye
int rightEyeX = 60;      // X position of the right eye
int eyeY = 18;           // Y position of both eyes
int eyeWidth = 25;       // Eye width
int eyeHeight = 30;      // Eye height

// Target offsets for smooth movement
int targetOffsetX = 0;
int targetOffsetY = 0;
int moveSpeed = 5;       // Control speed of movement

// Blinking and animation variables
int blinkState = 0;      // 0 = eyes open, 1 = eyes closed (blinking)
int blinkDelay = 4000;   // Blink delay (4 seconds)
unsigned long lastBlinkTime = 0;
unsigned long moveTime = 0;
bool blinkingEnabled = false;


//-------------------------------------------------------------
// Sensor related items
//-------------------------------------------------------------
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define SEALEVELPRESSURE_HPA (1013.25)

// Sensor data variables
float temp = 0;
float rh = 0;
uint8_t ens160_status = 0;
uint8_t aqi = 0;
uint16_t tvoc = 0;
uint16_t co2 = 0;
float const ALTITUDE = 62.0;               // Altitude at my location in meters
float const SEA_LEVEL_PRESSURE = 1013.25;  // Pressure at sea level
float temp_bmp = 0;
float pressure_bmp = 0;
float alt_bmp = 0;

ScioSense_ENS160 ens160(ENS160_I2CADDR_1);
PTSolns_AHTx aht;
Adafruit_BMP280 bmp;
ESP32Time rtc(3600); // Initialize RTC for gmt +1

// Button definition
int buttonState = 0;
volatile bool buttonPressed = false;
volatile bool ledState = false;
volatile unsigned long lastInterruptTime = 0;
const unsigned long debounceDelay = 200; // ms

// Interrupt Service Routine (ISR)
void IRAM_ATTR handleButtonPress() {
  unsigned long currentTime = millis();

  // Simple debounce
  if (currentTime - lastInterruptTime > debounceDelay) {
    buttonPressed = !buttonPressed;
    lastInterruptTime = currentTime;
  }
}



// // Timer definition
// volatile bool timerExpired = false;
// hw_timer_t * timer = NULL;

// void IRAM_ATTR onTimer() {
//   timerExpired = true; // set flag in ISR (keep ISR short)
// }

// Init functions
void serialInit() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }
  Serial.println("Serial initialized");
}

void sensorsInit() {
  // Initialize sensors here
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); 

  // ENS160 initialization
  Serial.print("ENS160...");
  ens160.begin();
  Serial.println(ens160.available() ? "done." : "failed!");
  if (ens160.available()) {
    // Print ENS160 versions
    Serial.print("\tRev: "); Serial.print(ens160.getMajorRev());
    Serial.print("."); Serial.print(ens160.getMinorRev());
    Serial.print("."); Serial.println(ens160.getBuild());

    Serial.print("\tStandard mode ");
    Serial.println(ens160.setMode(ENS160_OPMODE_STD) ? "done." : "failed!" );
  }

  // AHT21 initialization
  if (!aht.begin()) {
    Serial.println("AHT begin failed");
    for(;;){}
  }

  // BMP initialization
  bool status = bmp.begin(0x76); 

  if (!status) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    Serial.print("SensorID was: 0x");
    // Sensor ID will now correctly match the expected 0x58
    Serial.println(bmp.sensorID(), 16); 
    while (1) {
      delay(10);
    }
  }

  Serial.println("Sensors initialized");
}

void displayInit() {
  // Initialize display here
  display.begin(0x3D, true);
  display.display();
  display.setRotation(3);
  delay(2000);
  Serial.println("Display initialized");
}

void disableBluetoothCompletely() {
    btStop(); // Stops the controller
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    esp_bt_mem_release(ESP_BT_MODE_BTDM); // Releases memory to the heap
}

bool wifiInit() {
  // Set ESP32 to Station mode
  const char* ssid = "Yettel_79676F";
  const char* password = "TYRyFdCC";

  WiFi.mode(WIFI_STA);
  Serial.println("Starting WiFi...");
  WiFi.begin(ssid, password);

  for (int i = 0; i < 10; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi connected");
      Serial.println(WiFi.localIP());
      Serial.println("\nConnected!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      return true;
    }
    delay(1000);
  }
  return false;

}

void setTime() {
  // Sync the internal RTC with NTP server
  // This uses the built-in configTime() function internally
  // Parameters: gmtOffset_sec, daylightOffset_sec, ntpServer(s)
  configTime(0, 0, "pool.ntp.org"); 

  // Wait for the time to be retrieved
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    Serial.println("Time synchronized with NTP.");
    // The internal RTC is now set, even if WiFi disconnects later.
  } else {
    Serial.println("Failed to obtain time from NTP.");
  }
}

void setup() {
  pinMode(BUTTON_PIN, INPUT); // Set the pin as an input
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, FALLING);
  // Light sleep configuration
  esp_sleep_enable_timer_wakeup(60ULL * 1000000ULL);      // 60 seconds
  esp_sleep_enable_gpio_wakeup();
  gpio_wakeup_enable(BUTTON_PIN, GPIO_INTR_LOW_LEVEL);

  disableBluetoothCompletely();
  serialInit();
  
  displayInit();
  sensorsInit();

  // // Timer0, prescaler 80 -> 1 tick = 1 microsecond (80 MHz / 80 = 1 MHz)
  // timer = timerBegin(0, 80, true);               // timer index 0, count up
  // timerAttachInterrupt(timer, &onTimer, true);   // attach ISR
  // timerAlarmWrite(timer, 1000000 * 60, true);    // 1_000_000 us = 1 second, autoreload = true
  // timerAlarmEnable(timer);                       // start timer
}

String operationMode(uint8_t status) {
  // Operation Status
  // 0: Normal operation
  // 1: Warm-Up phase
  // 2: Initial Start-Up phase
  // 3: Invalid output

  String mode;
  if (status == 0) {
    mode = "Normal";
  } else if (status == 1) {
    mode = "Warm-up";
  } else if (status == 2) {
    mode = "Start-up";
  } else {
    mode = "Invalid";
  }
  
  return mode;
}

String aqi_uba_index(uint8_t status) {
  // AQI-UBA – UBA Air Quality Index
  // Operation Status
  // 1: Excellent
  // 2: Good
  // 3: Moderate
  // 4: Poor
  // 5: Unhealthy

  String mode;
  if (status == 1) {
    mode = "Excellent";
  } else if (status == 2) {
    mode = "Good";
  } else if (status == 3) {
    mode = "Moderate";
  } else if (status == 4) {
    mode = "Poor";
  } else if (status == 5) {
    mode = "Unhealthy";
  } else {
    mode = "Invalid";
  }
  
  return mode;
}

String eCo2_index(uint16_t value) {
  // Interpretation of CO2 and Equivalent CO2 values
  // >1500 Bad Heavily contaminated indoor air / Ventilation required
  // 1000 - 1500 Poor Contaminated indoor air / Ventilation recommended
  // 800 - 1000 Fair Optional ventilation
  // 600 - 800 Good Average
  // 400 - 600 Excellent Target

  String mode;
  if (value > 1500) {
    mode = "Bad";
  } else if (value >= 1000 && value <= 1500) {
    mode = "Poor";
  } else if (value >= 800 && value < 1000) {
    mode = "Fair";
  } else if (value >= 600 && value < 800) {
    mode = "Good";
  } else if (value >= 400 && value < 600) {
    mode = "Excellent";
  } else {
    mode = "Invalid";
  }

  return mode;
}

void readSensorData() {
  // Read sensor data here
  // AHTxStatus st = aht.readTemperatureHumidity(temp, rh, 120);
  // if (st == AHTX_OK) {
  //   Serial.print("T="); Serial.print(temp, 2);
  //   Serial.print("\nRH="); Serial.println(rh, 2);
  // } else {
  //   Serial.print("Error="); Serial.println((int)st);
  // }

  aht.readTemperatureHumidity(temp, rh, 120);

  temp_bmp = bmp.readTemperature();
  pressure_bmp = bmp.readPressure() / 100.0F;
  alt_bmp = bmp.readAltitude(SEALEVELPRESSURE_HPA);

  if (ens160.available()) {
    ens160.measure(true);
    ens160.measureRaw(true);
    ens160.set_envdata(temp_bmp, rh);
    ens160_status = ens160.read8(ENS160_I2CADDR_1, ENS160_REG_DATA_STATUS);
    ens160_status = ens160_status & 0x0C; 
    ens160_status = ens160_status >> 2; 
  }

  aqi = ens160.getAQI();
  tvoc = ens160.getTVOC();
  co2 = ens160.geteCO2();

  Serial.print("Sensor status readings: ");
  Serial.println(ens160_status);
  Serial.print("AQI: ");Serial.print(aqi);Serial.print("\t");
  Serial.print("TVOC: ");Serial.print(tvoc);Serial.print("ppb\t");
  Serial.print("eCO2: ");Serial.print(co2);Serial.print("ppm\t");
  Serial.println();

  Serial.print("T="); Serial.print(temp, 2);
  Serial.print("\nRH="); Serial.println(rh, 2);
  Serial.print("Temperature = ");  
  Serial.print(temp_bmp);
  Serial.println(" *C");
  Serial.print("Pressure = ");
  Serial.print(pressure_bmp); 
  Serial.println(" mbar");
  Serial.print("Approx. Altitude = ");
  Serial.print(alt_bmp);
  Serial.println(" m");
  Serial.println("");
}

void airQuality() {
  // Show sensor data on display
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setFont(NULL);
  display.setCursor(0, 0);
  String rtc_time = rtc.getTime("%d-%m-%Y %a %H:%M");
  display.println(rtc_time);
  display.drawLine(3, 8, 124, 8, 1);
  
  display.setCursor(0, 13);
  display.print("Operation:");
  String modeStr = operationMode(ens160_status);
  uint8_t len = modeStr.length();
  display.setCursor(SCREEN_HEIGHT - (6 * len), 13); 
  display.print(modeStr);

  display.setCursor(0, 26);
  display.println("TVOC:");
  display.print(" "); display.print(tvoc); display.print(" ppb");
  modeStr = aqi_uba_index(aqi);
  len = modeStr.length();
  display.setCursor(SCREEN_HEIGHT - (6 * len), 26); 
  display.print(modeStr);

  display.setCursor(0, 46);
  display.println("eCO2:");
  display.print(" "); display.print(co2); display.print(" ppm");
  modeStr = eCo2_index(co2);
  len = modeStr.length();
  display.setCursor(SCREEN_HEIGHT - (6 * len), 46); 
  display.print(modeStr);
  display.display();
}

void ambientConditions() {
  // Show sensor data on display
  // RTC time
  // Temperature, Humidity, Pressure, Altitude

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setFont(NULL);
  display.setCursor(0, 0);
  String rtc_time = rtc.getTime("%d-%m-%Y %a %H:%M");
  display.println(rtc_time);
  display.drawLine(3, 8, 124, 8, 1);
  display.setCursor(0, 13);
  display.print("Temp: "); display.print(temp_bmp); display.print(" C");
  display.setCursor(0, 24);
  display.print("RH: "); display.print(rh); display.print(" %");
  display.setCursor(0, 35);
  display.print("ATM: "); display.print(pressure_bmp); display.print(" mb");
  display.setCursor(0, 47);
  display.print("ALT: "); display.print(alt_bmp); display.print(" m");
  display.display();
}

// Eyes animation
void eyesAnimation() {

  unsigned long currentTime = millis();

  // Blinking logic
  if (currentTime - lastBlinkTime > blinkDelay && blinkState == 0) {
    blinkState = 1;  // Blink starts
    lastBlinkTime = currentTime;
  } else if (currentTime - lastBlinkTime > 150 && blinkState == 1) {//Decrease/Increase 150 to decrease/increase blink speed
    blinkState = 0;  // Blink ends
    lastBlinkTime = currentTime;
  }

  // Random eye movement logic
  if (currentTime - moveTime > random(1500, 3000) && blinkState == 0) {
    int movementType = random(0, 8);  // Choose movement: 0 to 7 (diagonal + center)
    if (movementType == 0) {          // Look left
      targetOffsetX = -10;
      targetOffsetY = 0;
    } else if (movementType == 1) {   // Look right
      targetOffsetX = 10;
      targetOffsetY = 0;
    } else if (movementType == 2) {   // Look up-left
      targetOffsetX = -10;
      targetOffsetY = -8;
    } else if (movementType == 3) {   // Look up-right
      targetOffsetX = 10;
      targetOffsetY = -8;
    } else if (movementType == 4) {   // Look down-left
      targetOffsetX = -10;
      targetOffsetY = 8;
    } else if (movementType == 5) {   // Look down-right
      targetOffsetX = 10;
      targetOffsetY = 8;
    } else {                          // Default (center)
      targetOffsetX = 0;
      targetOffsetY = 0;
    }
    moveTime = currentTime;
  }

  // Smooth interpolation for movement
  static int offsetX = 0;
  static int offsetY = 0;
  offsetX += (targetOffsetX - offsetX) / moveSpeed;
  offsetY += (targetOffsetY - offsetY) / moveSpeed;

  // Clear display for redraw
  display.clearDisplay();

  // Draw the left eye
  if (blinkState == 0) {
    drawEye(leftEyeX + offsetX, eyeY + offsetY, eyeWidth, eyeHeight);
  } else {
    // Eye blink (draw a thin horizontal line)
    display.fillRect(leftEyeX + offsetX, eyeY + offsetY + eyeHeight / 2 - 2, eyeWidth, 4, SH110X_WHITE);
  }

  // Draw the right eye
  if (blinkState == 0) {
    drawEye(rightEyeX + offsetX, eyeY + offsetY, eyeWidth, eyeHeight);
  } else {
    // Eye blink (draw a thin horizontal line)
    display.fillRect(rightEyeX + offsetX, eyeY + offsetY + eyeHeight / 2 - 2, eyeWidth, 4, SH110X_WHITE);
  }

  // Refresh the display
  display.display();

  // Small delay to prevent flickering
  delay(30);
}

// Function to draw a single eye
void drawEye(int eyeX, int eyeY, int eyeWidth, int eyeHeight) {
  display.fillRoundRect(eyeX, eyeY, eyeHeight, eyeWidth, 5, SH110X_WHITE);  // Draw rounded rectangle
}

// Define states
typedef enum {
    // STATE_CLOCK,
    STATE_AIR_QUALITY,
    STATE_AMBIENT_CONDITIONS,
    STATE_EYES_ANIMATION
} State;

State currentState = STATE_AIR_QUALITY;

// RTC time structure
struct tm timeinfo;

void loop() {
  
  if (getLocalTime(&timeinfo) == false || timeinfo.tm_year < 71) {
    // RTC clock is not set
    // initialize via NTP
    if (wifiInit()) {
      setTime();
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
    } else {
      Serial.println("WiFi not connected, cannot set time.");
    }
  }
  if (timeinfo.tm_hour == 0 && timeinfo.tm_min == 0) {
    if (wifiInit()) {
      setTime();
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
    } else {
      Serial.println("WiFi not connected, cannot set time.");
    }
  }

  // FSM implementation  
  switch (currentState) {
    case STATE_AIR_QUALITY:
      readSensorData();
      airQuality();

      if (buttonPressed)
        currentState = STATE_AMBIENT_CONDITIONS;
      
      esp_light_sleep_start();
      break;

    case STATE_AMBIENT_CONDITIONS:
      readSensorData();
      ambientConditions();
      
      if (buttonPressed)
        currentState = STATE_EYES_ANIMATION;
      
      esp_light_sleep_start();
      break;

    case STATE_EYES_ANIMATION:
      eyesAnimation();

      if (buttonPressed)
        currentState = STATE_AIR_QUALITY;
      
      // NO sleep call here - keep system awake during animation
      break;

    default:
      // currentState = STATE_AIR_QUALITY;
      break;
  }
}
