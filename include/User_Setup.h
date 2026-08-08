#define ILI9341_2_DRIVER

// #define TFT_WIDTH 320
// #define TFT_HEIGHT 240
// #define TFT_RGB

#define TFT_MOSI 13 // SDI
#define TFT_MISO 12 // SDO
#define TFT_SCLK 14 // SCK
#define TFT_CS 15   // Chip select control pin
#define TFT_DC 2    // Data Command control pin
#define TFT_RST -1  // Set TFT_RST to -1 if display RESET is connected to ESP32 board RST

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8

#define TFT_BL 27 // Backlight control

#define TOUCH_CS 21
#define TOUCH_IRQ 36

#define SPI_FREQUENCY 40000000
#define SPI_TOUCH_FREQUENCY 2500000

// https://forum.arduino.cc/t/esp32-2432s028r-all-in-one-display-touch-spi-problems/1059746/20
// https://mischianti.org/esp32-2432s028-cheap-yellow-display-high-resolution-pinout-datasheet-schema-and-specs/