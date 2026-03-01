# SateLite 🌞🛰️

**SateLite** is a solar-powered ESP32-based environmental monitoring device inspired by satellite design. It is designed to be energy-efficient and fully self-sufficient.

The device measures:

- 🌫️ Air quality (TVOC)  
- 🌍 CO₂ concentration  
- 🌡️ Temperature  
- 💧 Relative Humidity (RH)  
- 📊 Atmospheric Pressure  

All data is displayed on an onboard screen.

---

## 🖥️ Display Modes

SateLite features three display screens:

### 1️⃣ Air Quality Screen
- TVOC  
- CO₂  
- Date & Time  

### 2️⃣ Environmental Data Screen
- Temperature  
- Relative Humidity  
- Atmospheric Pressure  
- Date & Time  

### 3️⃣ Animation Screen
- Blinking eyes animation  

---

## 🔋 Power Management

The device is optimized for ultra-low power consumption:

- ☀️ Powered by solar panels  
- 🔋 Li-ion battery charged during daytime  
- 😴 Enters **light sleep every minute** after updating sensor readings and time  
- 📵 Bluetooth disabled to reduce power usage  
- 📶 Wi-Fi enabled **only when required**

---

## ⏰ Time Synchronization

- RTC synchronized via Wi-Fi:
  - Once during initial setup  
  - Automatically every day at **00:00**
- Wi-Fi remains active only during synchronization to preserve power.

---

SateLite is designed to operate autonomously with minimal energy consumption while continuously monitoring environmental conditions.

# 🛠️ Hardware Components

## ESP32U – MCU

Main microcontroller responsible for:
- Sensor data acquisition  
- Display control  
- Wi-Fi time synchronization  
- Power management (light sleep mode)  

---

## ENS160 – Digital Metal-Oxide Multi-Gas Sensor  
Datasheet:  
https://www.sciosense.com/wp-content/uploads/2023/12/ENS160-Datasheet.pdf

Measures:
- TVOC  
- eCO₂  

---

## AHT21 – Humidity and Temperature Sensor  
Datasheet:  
https://www.micros.com.pl/mediaserver/UPAHT21b_0001.pdf

Measures:
- Temperature  
- Relative Humidity (RH)  

---

## BMP280 – Pressure and Temperature Sensor  
Datasheet:  
https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf

Measures:
- Atmospheric pressure  
- Temperature  

---

## Waveshare 1.3inch OLED Module Display  
Wiki:  
https://www.waveshare.com/wiki/1.3inch_OLED_Module_(C)

Used for:
- Sensor data visualization  
- Date & time display  
- Blinking eyes animation screen  

---

## 📚 Display Library

Adafruit GFX Library:  
https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives

---

## TTP223 – Capacitive Touch Sensor  
Datasheet:  
https://files.seeedstudio.com/wiki/Grove-Touch_Sensor/res/TTP223.pdf

Used for:
- Switching between display screens  
- User interaction  
---

# 🔗 Useful Links

- Display layout generator:  
  https://rickkas7.github.io/DisplayGenerator/index.html  

- Online simulator (ESP32 + sensors):  
  https://wokwi.com/  

- ESP32 eyes animation project (for improved animation):  
  https://github.com/playfultechnology/esp32-eyes  

---

# ⚠️ NOTE

To compile successfully, declare `read8()` as `public` in the ScioSense library.
