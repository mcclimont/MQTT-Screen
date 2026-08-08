# MQTT Screen Project Notes

## Device
- Display hardware: Cheap Yellow Display
- Model: ESP32-2432S028R
- Controller: ESP32-D0WDQ6, RAM 520KB
- Display: 2.8", 320x240, RGB 65K colour
  - Display Driver ILI9341
- Touchscreen: Resistive Touch
  - Driver: XPT2046

Resources:
- Pinouts: https://randomnerdtutorials.com/esp32-cheap-yellow-display-cyd-pinout-esp32-2432s028r/

## Project Purpose
The MQTT-Screen project will provide four on-screen buttons that connect to a target ESP32 device running PicoMQTT and control relays.

## Libraries
- TFT_eSPI (display driver)
  - Used to drive the onboard ILI9341 320x240 TFT display.
- XPT2046_Touchscreen
  - Used for the onboard resistive touch controller (XPT2046).

## Target Device MQTT Topics
The target device publishes the following MQTT topics:

- home/relay1
- home/relay2
- home/relay3
- home/relay4

Each topic accepts the payload values:

- ON
- OFF
