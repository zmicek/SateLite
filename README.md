# SateLite
SateLite is a solar-powered ESP32 environmental monitoring device inspired by satellite design. It measures air quality, CO2 , temperature, humidity, and air pressure, displaying data on an onboard screen.

## Addiditonal functionalities
Date and time, which is updated over the WiFi/internet
Blinking eyes

To save the power, device is going to light sleep every minute after updating the time/sensor readings.

Idea of this device is to self sufficient by using solar panels which will charge li-ion battery during the day. 
To preserve little bit more power, bluetooth is disabled. 
RTC time/date is synchronized once initially and every day at 00:00, and WiFi will be active only during this period of time.

# Components:
## ESP32U - MCU

## ENS160 - Digital Metal-Oxide Multi-Gas Sensor
https://www.sciosense.com/wp-content/uploads/2023/12/ENS160-Datasheet.pdf

## AHT21 - Humidity and Temperature Sensor
https://www.micros.com.pl/mediaserver/UPAHT21b_0001.pdf

## BMP280 - Presure and Temperature Sensor
https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf

## Waveshare 1.3inch OLED Module Display
https://www.waveshare.com/wiki/1.3inch_OLED_Module_(C)

## Display library:
https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives

# Usefull links
Display layout generator: https://rickkas7.github.io/DisplayGenerator/index.html
Online simulator: https://wokwi.com/
For nicer eyes animation use: https://github.com/playfultechnology/esp32-eyes


# NOTE:
To make it possible to compile please declare read8() as public in ScioSense library.
