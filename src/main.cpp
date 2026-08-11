#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// ILI9341 display driver and ESP32 2.8" 320x240 pin mapping are configured in include/User_Setup.h
TFT_eSPI tft = TFT_eSPI();

// Touchscreen pins
#define XPT2046_IRQ 36  // T_IRQ
#define XPT2046_MOSI 32 // GPIO23  // T_DIN
#define XPT2046_MISO 39 // GPIO19  // T_OUT
#define XPT2046_CLK 25  // GPIO18   // T_CLK
#define XPT2046_CS 33   // T_CS
#define FONT_SIZE 4

SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define TFT_BACKLIGHT_ON 1

#define LED_R 4
#define LED_G 16
#define LED_B 17

bool screenInverted = false;

// Touchscreen coordinates: (x, y) and pressure (z)
int x, y, z;

// Print Touchscreen info about X, Y and Pressure (Z) on the Serial Monitor
void printTouchToSerial(int touchX, int touchY, int touchZ)
{
  Serial.print("X = ");
  Serial.print(touchX);
  Serial.print(" | Y = ");
  Serial.print(touchY);
  Serial.print(" | Pressure = ");
  Serial.print(touchZ);
  Serial.println();
}

// Print Touchscreen info about X, Y and Pressure (Z) on the TFT Display
void printTouchToDisplay(int touchX, int touchY, int touchZ)
{
  // Clear TFT screen
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  int centerX = tft.width() / 2;
  int textY = 60;

  String tempText = "X = " + String(touchX);
  tft.drawCentreString(tempText, centerX, textY, FONT_SIZE);

  textY += 40;
  tempText = "Y = " + String(touchY);
  tft.drawCentreString(tempText, centerX, textY, FONT_SIZE);

  textY += 40;
  tempText = "Pressure = " + String(touchZ);
  tft.drawCentreString(tempText, centerX, textY, FONT_SIZE);
}

void setup()
{
  delay(1000);

  Serial.begin(115200);
  Serial.println("Boot OK");

  Serial.println("Starting TFT...");
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  Serial.println("Starting touch...");
  // Start the SPI for the touchscreen and init the touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  // Set the Touchscreen rotation in landscape mode
  // Note: in some displays, the touchscreen might be upside down, so you might need to set the rotation to 3: touchscreen.setRotation(3);
  touchscreen.setRotation(1);

  // Set X and Y coordinates for center of display
  int centerX = tft.width() / 2;
  int centerY = tft.height() / 2;

  tft.drawCentreString("Touch screen to test", centerX, centerY, FONT_SIZE);

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
  // bool touched = touchscreen.touched();

  // if (touched != screenInverted)
  // {
  //   screenInverted = touched;
  //   tft.invertDisplay(screenInverted);
  //   Serial.printf("Touch %s - invert %s\n", touched ? "detected" : "released", screenInverted ? "ON" : "OFF");
  // }

  // Checks if Touchscreen was touched, and prints X, Y and Pressure (Z) info on the TFT display and Serial Monitor
  if (touchscreen.tirqTouched() && touchscreen.touched())
  {
    // Get Touchscreen points
    TS_Point p = touchscreen.getPoint();
    // Calibrate Touchscreen points with map function to the correct width and height
    x = map(p.x, 200, 3700, 1, tft.width());
    y = map(p.y, 240, 3800, 1, tft.height());
    z = p.z;

    printTouchToSerial(x, y, z);
    printTouchToDisplay(x, y, z);

    delay(100);
  }
}
