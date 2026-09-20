#include <Arduino.h>
#include <cstring>
// #include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <lvgl.h>
#include "lvgl_wrapper.h"

#include "time.h"
#include "IPAddress.h"

#include <WiFi.h>
#include <PicoMQTT.h>

#include "config.h" // Private WiFi and MQTT configuration

// Defines
// ==========

// Sketch Name
#define SKETCH_NAME "MQTT-Screen"

// Prototype
// ==========

// Constants
// ==========

// Relay topics
const char *relaySetTopics[4] = {
    "home/relay1/set",
    "home/relay2/set",
    "home/relay3/set",
    "home/relay4/set"};

const char *relayStateTopics[4] = {
    "home/relay1/state",
    "home/relay2/state",
    "home/relay3/state",
    "home/relay4/state"};

bool relayStates[4] = {false, false, false, false};
unsigned long lastRelayStateRefresh = 0;
const unsigned long RELAY_STATE_REFRESH_INTERVAL_MS = 30000;

// NTP Server Details
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 38000;
const int daylightOffset_sec = 3600;

// Variables
// ==========

// MQTT client
PicoMQTT::Client mqtt(MQTT_BROKER, MQTT_PORT, DEVICE_NAME, MQTT_USER, MQTT_PASSWORD);

// Network Details
IPAddress myIP(0, 0, 0, 0);

// Display driver: ILI9341 and ESP32 2.8" 320x240 pin mapping are configured in include/User_Setup.h
TFT_eSPI tft = TFT_eSPI();

// Touch screen
SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
int x = 0;
int y = 0;
int z = 0;

// LVGL display buffer - keep the partial buffer small enough to fit the ESP32's RAM budget
// while still letting the screen refresh reliably.
static lv_color_t buf1[320 * 20];

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

void printStatus(const __FlashStringHelper *message, bool toSerial = true, bool toTFT = false, bool newLine = true)
{
  if (toSerial)
    if (newLine)
      Serial.println(message);
    else
      Serial.print(message);
  if (toTFT)
    if (newLine)
      tft.println(message);
    else
      tft.print(message);
}

void printStatus(const String &message, bool toSerial = true, bool toTFT = false, bool newLine = true)
{
  if (toSerial)
    if (newLine)
      Serial.println(message);
    else
      Serial.print(message);
  if (toTFT)
    if (newLine)
      tft.println(message);
    else
      tft.print(message);
}

void printStatus(const char *message, bool toSerial = true, bool toTFT = false, bool newLine = true)
{
  if (toSerial)
    if (newLine)
      Serial.println(message);
    else
      Serial.print(message);
  if (toTFT)
    if (newLine)
      tft.println(message);
    else
      tft.print(message);
}

void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    printStatus(F("Failed to obtain time"), true, false);
    return;
  }
  char timeBuffer[64];
  strftime(timeBuffer, sizeof(timeBuffer), "%A, %B %d %Y %H:%M:%S", &timeinfo);
  printStatus(String(timeBuffer), true, false);
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

void updateRelayButtonColor(int relayIndex)
{
  if (relayIndex < 0 || relayIndex >= 4 || relayButtons[relayIndex] == nullptr)
    return;

  lv_color_t buttonColor = relayStates[relayIndex] ? lv_color_hex(0x2ECC71) : lv_color_hex(0x7F8C8D);
  lv_obj_set_style_bg_color(relayButtons[relayIndex], buttonColor, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(relayButtons[relayIndex], LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(relayButtons[relayIndex], lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(relayButtons[relayIndex], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void handleRelayStateMessage(const char *topic, const char *payload)
{
  // Verbose debugging output for testing
  Serial.printf("[DEBUG] handleRelayStateMessage called. topic='%s' payload='%s'\r\n", topic ? topic : "(null)", payload ? payload : "(null)");

  if (topic == nullptr || payload == nullptr)
  {
    Serial.println("[DEBUG] handleRelayStateMessage: ignoring null topic or payload");
    return;
  }

  for (int i = 0; i < 4; ++i)
  {
    Serial.printf("[DEBUG] comparing topic to relayStateTopics[%d] = '%s'\r\n", i, relayStateTopics[i]);

    if (strcmp(topic, relayStateTopics[i]) == 0)
    {
      String statePayload = payload;
      statePayload.trim();
      bool newState;
      if (statePayload.equalsIgnoreCase("ON"))
      {
        newState = true;
      }
      else if (statePayload.equalsIgnoreCase("OFF"))
      {
        newState = false;
      }
      else
      {
        Serial.printf("[DEBUG] ignoring unexpected relay state payload '%s'\r\n", statePayload.c_str());
        return;
      }

      Serial.printf("[DEBUG] matched index=%d payload_trimmed='%s' newState=%s\r\n", i, statePayload.c_str(), newState ? "ON" : "OFF");

      relayStates[i] = newState;
      printStatus(String("Relay ") + (i + 1) + " state update: " + (relayStates[i] ? "ON" : "OFF"));
      updateRelayButtonColor(i);
      return;
    }
    else
    {
      Serial.printf("[DEBUG] no match for index=%d\r\n", i);
    }
  }

  Serial.println("[DEBUG] handleRelayStateMessage: topic not matched to any relayStateTopics");
}

void subscribeToRelayStateTopics()
{
  Serial.println("[DEBUG] subscribeToRelayStateTopics: subscribing to relay state topics");
  for (int i = 0; i < 4; ++i)
  {
    Serial.printf("[DEBUG] subscribeToRelayStateTopics: subscribing index=%d topic='%s'\r\n", i, relayStateTopics[i]);
    mqtt.subscribe(relayStateTopics[i], [](const char *topic, const char *payload)
                   {
                     Serial.printf("[DEBUG] subscribeToRelayStateTopics: MQTT callback topic='%s' payload='%s'\r\n", topic ? topic : "(null)", payload ? payload : "(null)");
                     handleRelayStateMessage(topic, payload); });
  }
}

void refreshRelayStates()
{
  printStatus(F("Refreshing relay states..."));
  Serial.printf("[DEBUG] refreshRelayStates: MQTT connected=%s\r\n", mqtt.connected() ? "true" : "false");

  if (!mqtt.connected())
  {
    Serial.println("[DEBUG] refreshRelayStates: skipping requests because MQTT is disconnected");
    return;
  }

  for (int i = 0; i < 4; ++i)
  {
    Serial.printf("[DEBUG] refreshRelayStates: publishing GET index=%d topic='%s'\r\n", i, relayStateTopics[i]);
    const bool published = mqtt.publish(relayStateTopics[i], "GET");
    Serial.printf("[DEBUG] refreshRelayStates: publish result=%s\r\n", published ? "success" : "failed");
    mqtt.loop();
    delay(10);
  }
  Serial.println("[DEBUG] refreshRelayStates: completed state requests");
}

// Bootstrap
// ==========

void setup()
{
  // Initialize serial communication
  delay(1000);
  Serial.begin(115200);

  setLEDColor(100, 0, 0); // Start the RGB LED with red to indicate startup

  // Sketch tagging
  printStatus(String("Sketch: [") + SKETCH_NAME + "]");

  printStatus(F("Starting TFT..."), true, false);
  tft.init();
  tft.setRotation(1);
  setBackgroundBrightness(40); // Set background brightness

  printStatus(F("Starting touch..."));
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(1);

  // Set "cursor" at top left corner of display (0,0) and select font 4
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 4, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  printStatus(F("Starting..."), false, true);

  // WiFi
  // --------------------
  printStatus(String("Connecting to WiFi: ") + WIFI_SSID, true, true);
  setLEDColor(100, 60, 0); // Change the RGB LED to yellow to indicate WiFi connection attempt
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    printStatus(".", true, false, false);
  }
  printStatus(F("WiFi connected"));
  myIP = getlocalIP();
  printStatus(String("Network IP: [") + myIP.toString() + "]", true, true);

  // NTP Client
  // --------------------
  printStatus(F("Synchronising time..."), true, true);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
  setLEDColor(0, 100, 0); // Change the RGB LED to green to indicate successful WiFi connection

  // PicoMQTT Initialisation
  // --------------------
  printStatus(F("Starting PicoMQTT interface..."), true, true);
  setLEDColor(0, 0, 100); // Change the RGB LED to blue to indicate PicoMQTT initialization
  mqtt.begin();
  subscribeToRelayStateTopics();
  lastRelayStateRefresh = millis();

  // LVGL
  // --------------------
  printStatus(F("Starting LVGL interface..."), true, true);
  lv_init();

  lv_display_t *display = lv_display_create(tft.width(), tft.height());
  lv_display_set_default(display);
  lv_display_set_flush_cb(display, my_disp_flush);
  lv_display_set_buffers(display, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);
  lv_indev_set_display(indev, display);

  lvgl_wrapper_init();
  for (int i = 0; i < 4; ++i)
  {
    if (relayButtons[i] != nullptr)
    {
      lv_obj_set_style_radius(relayButtons[i], 10, LV_PART_MAIN);
      updateRelayButtonColor(i);
    }
  }

  printStatus(F("LVGL started"));
  setLEDColor(0, 0, 0); // Turn off the LED after successful startup
}

void loop()
{
  // Provides a time slice for system tasks
  yield();

  // Check WiFi connection status
  if (WiFi.status() != WL_CONNECTED)
  {
    printStatus(F("WiFi disconnected! Reconnecting..."));
    // digitalWrite(led, HIGH); // turn the LED on
    WiFi.reconnect();

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(250);
      printStatus(".", true, false, false);
    }
    printStatus(F("Reconnected to WiFi!"));
    // digitalWrite(led, LOW); // turn the LED off
  }

  // Check if the local IP has changed
  if (myIP != getlocalIP())
  {
    myIP = getlocalIP();
    printStatus(String("Network IP: [") + myIP.toString() + "]");

    // Update the local time from the NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    printLocalTime();
  }

  // Run the MQTT server loop to handle incoming client connections and messages
  mqtt.loop();

  if (millis() - lastRelayStateRefresh >= RELAY_STATE_REFRESH_INTERVAL_MS)
  {
    lastRelayStateRefresh = millis();
    refreshRelayStates();
  }

  // Original LVGL loop code (commented out for now)
  static uint32_t last_tick = 0;
  uint32_t now = millis();
  uint32_t diff = now - last_tick;

  if (diff >= 5)
  {
    lv_tick_inc(diff);
    last_tick = now;
  }

  lv_timer_handler();
  delay(5);
}
