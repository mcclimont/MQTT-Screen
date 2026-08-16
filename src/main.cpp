#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <lvgl.h>
#include "lvgl_wrapper.h"

#include "time.h"
#include "IPAddress.h"

#include <WiFi.h>
#include <PicoMQTT.h>

#include "config.h" // Private WiFi and MQTT configuration

// Display driver: ILI9341 and ESP32 2.8" 320x240 pin mapping are configured in include/User_Setup.h
TFT_eSPI tft = TFT_eSPI();

// Touch screen
SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
int x = 0;
int y = 0;
int z = 0;

// LVGL display buffer
static lv_color_t buf1[320 * 20];

// MQTT topics for each relay
const char *relayTopics[4] = {
    "home/relay1",
    "home/relay2",
    "home/relay3",
    "home/relay4"};

// NTP Server Details
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 38000;
const int daylightOffset_sec = 3600;

// Network Details
IPAddress myIP(0, 0, 0, 0);

// Functions
// ==========

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
  int32_t w = area->x2 - area->x1 + 1;
  int32_t h = area->y2 - area->y1 + 1;

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)px_map, w * h, true);
  tft.endWrite();

  lv_display_flush_ready(disp);
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
  if (touchscreen.tirqTouched() && touchscreen.touched())
  {
    TS_Point p = touchscreen.getPoint();
    x = map(p.x, 200, 3700, 0, tft.width() - 1);
    y = map(p.y, 240, 3800, 0, tft.height() - 1);
    z = p.z;

    data->point.x = x;
    data->point.y = y;
    data->state = (z > 0) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
  }
  else
  {
    data->point.x = x;
    data->point.y = y;
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

// Functions
// ==========

void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

  // time_t now;
  // time(&now);
  // Serial.println(now);
}

time_t getTime()
{
  time_t now;
  time(&now);
  return now;
}

String getTimePrintable()
{
  time_t now;
  time(&now);

  String time_string = String(ctime(&now));
  return time_string;
}

IPAddress getlocalIP()
{
  return WiFi.localIP();
}

void setBackgroundBrightness(int brightness)
{
  analogWrite(TFT_BL, brightness * 2.55);
}

void setLEDColor(int r, int g, int b)
{
  analogWrite(LED_R, 255 - (r * 2.55));
  analogWrite(LED_G, 255 - (g * 2.55));
  analogWrite(LED_B, 255 - (b * 2.55));
}

// MQTT Functions
// ==========

// void readCurrentState(PicoMQTT &mqtt)
// {
//   for (int i = 0; i < 4; i++)
//   {
//     String topic = String(relayTopics[i]) + "/state";
//     mqtt.subscribe(topic.c_str());
//   }
// }

// Bootstrap
// ==========

void setup()
{
  delay(1000);
  Serial.begin(115200);

  setLEDColor(100, 0, 0); // Start the RGB LED with red to indicate startup

  // Sketch tagging
  Serial.println("Sketch: [" + String(SKETCH_NAME) + "]");

  Serial.println("Starting TFT...");
  tft.init();
  tft.setRotation(1);
  setBackgroundBrightness(40); // Set background brightness

  Serial.println("Starting touch...");
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(1);

  // Set "cursor" at top left corner of display (0,0) and select font 4
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 4, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.println("Starting...\n");

  // WiFi
  // --------------------
  Serial.printf("Connecting to WiFi: %s\n", WIFI_SSID);
  tft.printf("Connecting to WiFi: %s\n", WIFI_SSID);
  setLEDColor(100, 60, 0); // Change the RGB LED to yellow to indicate WiFi connection attempt
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  myIP = getlocalIP();
  Serial.println("Network IP: [" + myIP.toString() + "]");
  tft.println("Network IP: " + myIP.toString());

  // NTP Client
  // --------------------
  Serial.println("Synchronising time...");
  tft.println("Synchronising time...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
  setLEDColor(0, 100, 0); // Change the RGB LED to green to indicate successful WiFi connection

  // PicoMQTT Initialisation
  // --------------------
  Serial.println("Starting PicoMQTT interface...");
  setLEDColor(0, 0, 100); // Change the RGB LED to blue to indicate PicoMQTT initialization

  // LVGL
  // --------------------
  Serial.println("Starting LVGL interface...");
  // lv_init();

  // lv_display_t *display = lv_display_create(tft.width(), tft.height());
  // lv_display_set_default(display);
  // lv_display_set_flush_cb(display, my_disp_flush);
  // lv_display_set_buffers(display, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
  // lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);

  // lv_indev_t *indev = lv_indev_create();
  // lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  // lv_indev_set_read_cb(indev, my_touchpad_read);
  // lv_indev_set_display(indev, display);

  // lv_obj_t *screen = lv_scr_act();
  // lvgl_wrapper_init();

  Serial.println("LVGL started");
  setLEDColor(0, 0, 0); // Turn off the LED after successful startup
}

void loop()
{
  // Provides a time slice for system tasks
  yield();

  // Check WiFi connection status
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi disconnected! Reconnecting...");
    // digitalWrite(led, HIGH); // turn the LED on
    WiFi.reconnect();

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(250);
      Serial.print(".");
    }
    Serial.println("\nReconnected to WiFi!");
    // digitalWrite(led, LOW); // turn the LED off
  }

  // Check if the local IP has changed
  if (myIP != getlocalIP())
  {
    myIP = getlocalIP();
    Serial.println("Network IP: [" + myIP.toString() + "]");

    // Update the local time from the NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    printLocalTime();
  }

  // Run the MQTT server loop to handle incoming client connections and messages
  delay(1);
  // mqtt.loop();

  // Original LVGL loop code (commented out for now)
  // static uint32_t last_tick = 0;
  // uint32_t now = millis();
  // uint32_t diff = now - last_tick;

  // if (diff >= 5)
  // {
  //   lv_tick_inc(diff);
  //   last_tick = now;
  // }

  // lv_timer_handler();
  // delay(5);
}
