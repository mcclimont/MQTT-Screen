# MQTT Screen Project Notes

## Project Purpose
The MQTT-Screen project will provide four on-screen buttons that connect to a target ESP32 device running PicoMQTT and control relays.

## Hardware and SPI Configuration
This project uses a single ESP32 board with two separate SPI buses:

- TFT display SPI: the display uses the default ESP32 SPI bus (VSPI) and is driven by the ILI9341 controller.
- Touchscreen SPI: the resistive touch controller uses a second SPI bus on HSPI.

### Board
- Board model: ESP32-2432S028R (Cheap Yellow Display)
- MCU: ESP32-D0WDQ6
- Display: 2.8" 320x240 TFT, ILI9341
- Touch controller: XPT2046 resistive touchscreen

### Actual SPI Pins
- TFT display (VSPI)
  - SCLK: GPIO 14
  - MOSI: GPIO 13
  - MISO: GPIO 12
  - CS: GPIO 15
  - DC: GPIO 2
  - RST: not connected (set to -1 in the TFT setup)
  - BL: GPIO 27

- Touchscreen (HSPI)
  - SCLK: GPIO 25
  - MOSI: GPIO 32
  - MISO: GPIO 39
  - CS: GPIO 33
  - IRQ: GPIO 36

References:
- Pinouts: https://randomnerdtutorials.com/esp32-cheap-yellow-display-cyd-pinout-esp32-2432s028r/

## Libraries and Versions
- bodmer/TFT_eSPI v2.5.43
  - Used to drive the onboard ILI9341 320x240 TFT display.
- PaulStoffregen/XPT2046_Touchscreen v1.4
  - Used for the onboard resistive touch controller (XPT2046).
  - Sourced from the GIT repository (https://github.com/PaulStoffregen/XPT2046_Touchscreen.git#v1.4). The platformio respository version is v1.3 as at Aug 2026.

## Target Device MQTT Topics
The target device publishes the following MQTT topics:

- home/relay1
- home/relay2
- home/relay3
- home/relay4

Each topic accepts the payload values:

- ON
- OFF
