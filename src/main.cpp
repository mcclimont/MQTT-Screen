#include <SPI.h>

#include <TFT_eSPI.h>
// ILI9341 display driver and ESP32 2.8" 320x240 pin mapping are configured in include/User_Setup.h
TFT_eSPI tft = TFT_eSPI();

#define TFT_BACKLIGHT_ON 1

#define LED_R 4
#define LED_G 16
#define LED_B 17

// static void scanI2CDevices()
// {
//   Wire.begin();
//   Serial.println("I2C bus scan:");
//   bool found = false;

//   for (uint8_t address = 1; address < 127; ++address)
//   {
//     Wire.beginTransmission(address);
//     uint8_t error = Wire.endTransmission();

//     if (error == 0)
//     {
//       Serial.printf("  Found I2C device at 0x%02X\n", address);
//       found = true;
//     }
//     else if (error == 4)
//     {
//       Serial.printf("  Unknown error at 0x%02X\n", address);
//     }
//   }

//   if (!found)
//   {
//     Serial.println("  No I2C devices found");
//   }
// }

// static void scanSPIDevices()
// {
//   Serial.println("SPI device scan:");
//   SPIClass spi(HSPI);
//   spi.begin(TFT_SCLK, TFT_MISO, TFT_MOSI);

//   pinMode(TFT_CS, OUTPUT);
//   digitalWrite(TFT_CS, HIGH);
//   pinMode(TFT_DC, OUTPUT);
//   digitalWrite(TFT_DC, HIGH);
//   pinMode(TOUCH_CS, OUTPUT);
//   digitalWrite(TOUCH_CS, HIGH);

//   // Probe ILI9341 display by reading its ID register.
//   Serial.print("  Probing ILI9341 TFT (CS=");
//   Serial.print(TFT_CS);
//   Serial.println(")...");

//   spi.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
//   digitalWrite(TFT_CS, LOW);
//   digitalWrite(TFT_DC, LOW);
//   spi.transfer(0x04);
//   uint8_t id1 = spi.transfer(0x00);
//   uint8_t id2 = spi.transfer(0x00);
//   uint8_t id3 = spi.transfer(0x00);
//   digitalWrite(TFT_CS, HIGH);
//   spi.endTransaction();

//   Serial.printf("    ILI9341 ID bytes: %02X %02X %02X\n", id1, id2, id3);

//   Serial.println("  Reading ILI9341 command registers:");
// // If a hardware reset line is available, pulse it before reads
// #if defined(TFT_RST)
//   if (TFT_RST != -1)
//   {
//     pinMode(TFT_RST, OUTPUT);
//     digitalWrite(TFT_RST, LOW);
//     delay(20);
//     digitalWrite(TFT_RST, HIGH);
//     delay(50);
//     Serial.printf("    Pulsed TFT_RST=%d\n", TFT_RST);
//   }
//   else
//   {
//     Serial.println("    TFT_RST not defined or -1; skipping hardware reset");
//   }
// #endif

//   Serial.printf("    0xD3 Read ID1 = 0x%02X\n", tft.readcommand8(0xD3, 1));
//   Serial.printf("    0xD3 Read ID2 = 0x%02X\n", tft.readcommand8(0xD3, 2));
//   Serial.printf("    0xD3 Read ID3 = 0x%02X\n", tft.readcommand8(0xD3, 3));
//   Serial.printf("    0xD3 Read ID4 = 0x%02X\n", tft.readcommand8(0xD3, 4));
//   Serial.printf("    0x0A Read Power Mode = 0x%02X\n", tft.readcommand8(0x0A, 1));
//   Serial.printf("    0x0B Read MADCTL = 0x%02X\n", tft.readcommand8(0x0B, 1));
//   Serial.printf("    0x0C Read Pixel Format = 0x%02X\n", tft.readcommand8(0x0C, 1));
//   Serial.printf("    0x09 Read Status = 0x%02X\n", tft.readcommand8(0x09, 1));

//   // Probe XPT2046 touch controller briefly by toggling CS and reading a single response byte.
//   Serial.print("  Probing XPT2046 touch controller (CS=");
//   Serial.print(TOUCH_CS);
//   Serial.println(")...");

//   digitalWrite(TOUCH_CS, LOW);
//   spi.transfer(0x90);
//   uint8_t resp = spi.transfer(0x00);
//   digitalWrite(TOUCH_CS, HIGH);

//   Serial.printf("    XPT2046 sample response: %02X\n", resp);

//   spi.end();
// }

void setup()
{
  delay(1000);

  Serial.begin(115200);
  Serial.println("Boot OK");

  // Run I2C/SPI diagnostic scans after TFT initialization
  // Ensure backlight is enabled so we can see the display
  // pinMode(TFT_BL, OUTPUT);
  // digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
  // analogWrite(TFT_BL, 128); // Set backlight brightness (0-255) if using PWM control

  // Serial.println("I2C/SPI device scan starting...");
  // scanI2CDevices();
  // scanSPIDevices();
  // Serial.println("I2C/SPI device scan complete.");

  Serial.println("Starting TFT...");
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // tft.setRotation(1);
  Serial.println("TFT init done");

  Serial.println("TFT test content display starting...");
  tft.drawRect(0, 0, tft.width(), tft.height(), TFT_GREEN);
  tft.drawRect(2, 2, tft.width() - 4, tft.height() - 4, TFT_GREEN);

  // Set "cursor" at top left corner of display (0,0) and select font 4
  tft.setCursor(0, 4, 4);

  // Set the font colour to be white with a black background
  tft.setTextColor(TFT_WHITE);

  // We can now plot text on screen using the "print" class
  tft.println(" Initialised default\n");
  tft.println(" White text");

  tft.setTextColor(TFT_RED);
  tft.println(" Red text");

  tft.setTextColor(TFT_GREEN);
  tft.println(" Green text");

  tft.setTextColor(TFT_BLUE);
  tft.println(" Blue text");

  Serial.println("TFT test done");
}

void loop()
{
  delay(5000);
  tft.invertDisplay(false); // Where i is true or false
}
