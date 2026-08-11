// TFT Configuration
#define ILI9341_2_DRIVER

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
#define TFT_BACKLIGHT_ON 1

// For some reason the TOUCH_CS pin must be defined and set to 21, otherwise the tft does not work properly.
#define TOUCH_CS 21
#define TOUCH_IRQ 36

#define SPI_FREQUENCY 40000000
#define SPI_TOUCH_FREQUENCY 2500000

// Touchscreen configuration
#define XPT2046_IRQ 36  // T_IRQ
#define XPT2046_MOSI 32 // T_DIN
#define XPT2046_MISO 39 // T_OUT
#define XPT2046_CLK 25  // T_CLK
#define XPT2046_CS 33   // T_CS

// RGB LED configuration
#define LED_R 4
#define LED_G 16
#define LED_B 17
