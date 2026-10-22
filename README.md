# MQTT Screen Project Notes

## Project Purpose
The MQTT-Screen project provides four on-screen light toggles and per-light timer controls that connect to a target ESP32 device running PicoMQTT.

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

- home/relay1/set
- home/relay1/state
- home/relay2/set
- home/relay2/state
- home/relay3/set
- home/relay3/state
- home/relay4/set
- home/relay4/state

The `set` topics accept these payload values:

- ON
- OFF
- TOGGLE

The `state` topics will retrieve the current state of `ON` or `OFF`.

## Per-Light Auto-Off Timers

The Screen UI provides an independent timer button beside each light toggle. It opens fixed duration choices of 1, 2, 4, or 8 hours, plus a cancel action. It publishes the selected duration to `home/relayN/timer/set` as `3600`, `7200`, `14400`, or `28800` seconds. The topic contract accepts any positive integer duration in seconds or `CANCEL`. Starting a positive-duration timer turns that light on and starts or replaces its countdown; `CANCEL` clears the countdown without changing the light state. Manually sending `OFF` to the existing relay `set` topic cancels the timer as well. Sending `ON` leaves an active countdown unchanged.

The Screen subscribes to `home/relayN/timer/state` for each relay and requests the current value after MQTT connects and every 30 seconds. The controller reports remaining whole seconds, using `0` for no active timer, when a timer starts, is canceled, expires, or when the Screen requests `GET`. The UI decrements the reported value locally between updates. It shows `--` while MQTT is disconnected, `...` until timer state is reported, and `Set` when no timer is active.

The Screen controls and MQTT client handling, and the matching MQTT-Lights timer-topic handling, are implemented in source. Build and deploy the MQTT-Lights firmware to the controller for end-to-end operation. Timer durations are volatile and do not survive a restart of the MQTT-Lights controller.

## Build and Deployment

### Development Environment

The project is developed on Windows using Visual Studio Code and the pioarduino PlatformIO IDE extension. PlatformIO builds the Arduino framework firmware for the `esp32dev` environment and resolves the libraries declared in `platformio.ini`.

### Configure, Build, and Upload

1. Open the MQTT-Screen project folder in Visual Studio Code and allow PlatformIO to install the project dependencies.
2. In the project root, copy `include/config.example.h` to `include/config.h` and set the Wi-Fi and MQTT connection values. Keep `include/config.h` private; it is excluded from Git.
3. Connect the ESP32-2432S028R board over USB and build the firmware:

  ```powershell
  pio run -e esp32dev
  ```

4. Upload the firmware to the connected board:

  ```powershell
  pio run -e esp32dev -t upload
  ```

The same build and upload targets are available from the PlatformIO controls in Visual Studio Code. If PlatformIO does not detect the board's serial port, select or configure the upload port before uploading.

## Responsible AI

AI tools were used to assist with developing and documenting this project. Project maintainers are responsible for reviewing, testing, and deploying changes.
