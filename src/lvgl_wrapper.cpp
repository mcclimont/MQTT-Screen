#include "lvgl_wrapper.h"
#include <Arduino.h>
#include <lvgl.h>
#include "config.h"

#define LV_HOR_RES_MAX 320
#define LV_VER_RES_MAX 240

static const int HEADER_HEIGHT = 31;
static const int HEADER_RULE_HEIGHT = 2;
static const int STATUS_BAR_HEIGHT = 20;
static const int STATUS_BAR_SEPARATOR_HEIGHT = 1;
static const int STATUS_BAR_SEPARATOR_PADDING = 3;
static const int MAIN_CANVAS_X = 0;
static const int MAIN_CANVAS_Y = HEADER_HEIGHT + HEADER_RULE_HEIGHT;
static const int MAIN_CANVAS_WIDTH = LV_HOR_RES_MAX;
static const int MAIN_CANVAS_HEIGHT = LV_VER_RES_MAX - MAIN_CANVAS_Y - STATUS_BAR_HEIGHT - STATUS_BAR_SEPARATOR_HEIGHT - STATUS_BAR_SEPARATOR_PADDING;
static const int STATUS_BAR_Y = LV_VER_RES_MAX - STATUS_BAR_HEIGHT;

lv_obj_t *relayButtons[4] = {nullptr, nullptr, nullptr, nullptr};
static bool relayStates[4] = {false, false, false, false};
static bool relayStatesKnown[4] = {false, false, false, false};
static bool mqttConnected = false;
static bool wifiConnected = false;
static lv_obj_t *mqttStatusIndicator = nullptr;
static lv_obj_t *wifiStatusIndicator = nullptr;
static lv_obj_t *rgbColorDots[3] = {nullptr, nullptr, nullptr};
static lv_timer_t *rgbAnimationTimer = nullptr;
static uint8_t rgbAnimationPhase = 0;
static const uint32_t STATUS_RED = 0xFF0000;
static const uint32_t STATUS_GREEN = 0x00FF00;
static const uint32_t RELAY_BUTTON_CONNECTED_ON = 0x19e84c;     // 0x438B55
static const uint32_t RELAY_BUTTON_CONNECTED_OFF = 0x500a0a;    // 0xB85B5B
static const uint32_t RELAY_BUTTON_DISCONNECTED_ON = 0x326940;  // 0x526B58
static const uint32_t RELAY_BUTTON_DISCONNECTED_OFF = 0x2e0606; // 0x725454
static const uint32_t RELAY_BUTTON_INITIAL = 0xD3D3D3;

static void rgbAnimationTimerCb(lv_timer_t *timer)
{
  (void)timer;
  const uint8_t highlightedDot = rgbAnimationPhase++ % 3;
  for (uint8_t i = 0; i < 3; ++i)
  {
    lv_obj_set_style_bg_opa(rgbColorDots[i], i == highlightedDot ? LV_OPA_COVER : LV_OPA_30, LV_PART_MAIN);
  }
}

static void stopRgbAnimation()
{
  if (rgbAnimationTimer != nullptr)
  {
    lv_timer_delete(rgbAnimationTimer);
    rgbAnimationTimer = nullptr;
  }

  rgbAnimationPhase = 0;
  for (uint8_t i = 0; i < 3; ++i)
  {
    if (rgbColorDots[i] != nullptr)
    {
      lv_obj_set_style_bg_opa(rgbColorDots[i], LV_OPA_COVER, LV_PART_MAIN);
    }
  }
}

static void updateRgbAnimationState()
{
  if (rgbColorDots[0] == nullptr)
  {
    return;
  }

  if (wifiConnected && mqttConnected)
  {
    stopRgbAnimation();
  }
  else if (rgbAnimationTimer == nullptr)
  {
    rgbAnimationPhase = 0;
    rgbAnimationTimer = lv_timer_create(rgbAnimationTimerCb, 180, nullptr);
    rgbAnimationTimerCb(rgbAnimationTimer);
  }
}

static void updateRelayButtonColor(int relayIndex)
{
  if (relayIndex < 0 || relayIndex >= 4 || relayButtons[relayIndex] == nullptr)
  {
    return;
  }

  uint32_t colorHex = RELAY_BUTTON_INITIAL;
  if (relayStatesKnown[relayIndex])
  {
    if (mqttConnected)
    {
      colorHex = relayStates[relayIndex] ? RELAY_BUTTON_CONNECTED_ON : RELAY_BUTTON_CONNECTED_OFF;
    }
    else
    {
      colorHex = relayStates[relayIndex] ? RELAY_BUTTON_DISCONNECTED_ON : RELAY_BUTTON_DISCONNECTED_OFF;
    }
  }

  lv_obj_set_style_bg_color(relayButtons[relayIndex], lv_color_hex(colorHex), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(relayButtons[relayIndex], LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  const lv_color_t textColor = !relayStatesKnown[relayIndex]
                                   ? lv_color_hex(0x303030)
                                   : (relayStates[relayIndex] ? lv_color_hex(0x003B16) : lv_color_hex(0xFF8B8B));
  lv_obj_set_style_text_color(relayButtons[relayIndex], textColor, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(relayButtons[relayIndex], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void button_event_cb(lv_event_t *e)
{
  if (lv_event_get_code(e) == LV_EVENT_CLICKED)
  {
    lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    const char *text = lv_label_get_text(label);
    Serial.print("Button pressed: ");
    Serial.println(text);

    for (int relayIndex = 0; relayIndex < 4; ++relayIndex)
    {
      if (relayButtons[relayIndex] == btn)
      {
        publishRelayToggle(relayIndex);
        break;
      }
    }
  }
}

static void createHeader(lv_obj_t *screen)
{
  lv_obj_t *header = lv_obj_create(screen);
  lv_obj_set_size(header, LV_HOR_RES_MAX, HEADER_HEIGHT);
  lv_obj_set_pos(header, 0, 0);
  lv_obj_set_style_radius(header, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);
  lv_obj_set_style_outline_width(header, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(header, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_color(header, lv_color_hex(0x173A3A), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(header, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_pad_all(header, 0, LV_PART_MAIN);

  lv_obj_t *title = lv_label_create(header);
  lv_label_set_text(title, SCREEN_TITLE);
  lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
  lv_obj_center(title);

  lv_obj_t *headerRule = lv_obj_create(screen);
  lv_obj_set_size(headerRule, LV_HOR_RES_MAX, HEADER_RULE_HEIGHT);
  lv_obj_set_pos(headerRule, 0, HEADER_HEIGHT);
  lv_obj_set_style_radius(headerRule, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(headerRule, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_color(headerRule, lv_color_hex(0x35D0BA), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(headerRule, LV_OPA_COVER, LV_PART_MAIN);
}

static void createMainCanvas(lv_obj_t *screen)
{
  const int buttonWidth = 130;
  const int buttonHeight = 60;
  const int gridColumnWidth = MAIN_CANVAS_WIDTH / 2;
  const int verticalSpace = MAIN_CANVAS_HEIGHT - (2 * buttonHeight);
  const char *buttonLabels[] = {"Light 1", "Light 2", "Light 3", "Light 4"};

  for (uint8_t i = 0; i < 4; ++i)
  {
    const int column = i % 2;
    const int row = i / 2;
    const int x = MAIN_CANVAS_X + column * gridColumnWidth + (gridColumnWidth - buttonWidth) / 2;
    const int y = MAIN_CANVAS_Y + ((row + 1) * verticalSpace) / 3 + row * buttonHeight;
    relayButtons[i] = addButton(screen, buttonLabels[i], x, y, buttonWidth, buttonHeight, button_event_cb);
    updateRelayButtonColor(i);
  }
}

static void createStatusBar(lv_obj_t *screen)
{
  lv_obj_t *statusBarSeparator = lv_obj_create(screen);
  lv_obj_set_size(statusBarSeparator, LV_HOR_RES_MAX, STATUS_BAR_SEPARATOR_HEIGHT);
  lv_obj_set_pos(statusBarSeparator, 0, STATUS_BAR_Y - STATUS_BAR_SEPARATOR_HEIGHT - STATUS_BAR_SEPARATOR_PADDING);
  lv_obj_set_style_radius(statusBarSeparator, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(statusBarSeparator, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_color(statusBarSeparator, lv_color_hex(0x808080), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(statusBarSeparator, LV_OPA_50, LV_PART_MAIN);

  lv_obj_t *mqttLabel = lv_label_create(screen);
  lv_label_set_text(mqttLabel, "Lights");
  lv_obj_set_pos(mqttLabel, 247, STATUS_BAR_Y);

  mqttStatusIndicator = lv_obj_create(screen);
  lv_obj_set_size(mqttStatusIndicator, 14, 14);
  lv_obj_set_pos(mqttStatusIndicator, 298, STATUS_BAR_Y);
  lv_obj_set_style_radius(mqttStatusIndicator, 7, LV_PART_MAIN);
  lv_obj_set_style_border_width(mqttStatusIndicator, 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(mqttStatusIndicator, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(mqttStatusIndicator, LV_OPA_COVER, LV_PART_MAIN);
  lvgl_set_mqtt_connected(mqttConnected);

  lv_obj_t *wifiLabel = lv_label_create(screen);
  lv_label_set_text(wifiLabel, "WiFi");
  lv_obj_set_pos(wifiLabel, 8, STATUS_BAR_Y);

  wifiStatusIndicator = lv_obj_create(screen);
  lv_obj_set_size(wifiStatusIndicator, 14, 14);
  lv_obj_set_pos(wifiStatusIndicator, 47, STATUS_BAR_Y);
  lv_obj_set_style_radius(wifiStatusIndicator, 7, LV_PART_MAIN);
  lv_obj_set_style_border_width(wifiStatusIndicator, 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(wifiStatusIndicator, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(wifiStatusIndicator, LV_OPA_COVER, LV_PART_MAIN);
  lvgl_set_wifi_connected(wifiConnected);

  const uint32_t rgbColors[] = {0xFF0000, 0x00FF00, 0x0000FF};
  for (uint8_t i = 0; i < 3; ++i)
  {
    rgbColorDots[i] = lv_obj_create(screen);
    lv_obj_set_size(rgbColorDots[i], 8, 8);
    lv_obj_set_pos(rgbColorDots[i], 145 + (i * 11), STATUS_BAR_Y + 3);
    lv_obj_set_style_radius(rgbColorDots[i], LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(rgbColorDots[i], 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(rgbColorDots[i], lv_color_hex(rgbColors[i]), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(rgbColorDots[i], LV_OPA_COVER, LV_PART_MAIN);
  }

  updateRgbAnimationState();
}

void lvgl_wrapper_init()
{
  lv_obj_t *screen = lv_scr_act();

  // If this initializer is called again, clear the previous screen state first.
  stopRgbAnimation();
  lv_obj_clean(screen);
  lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
  memset(relayButtons, 0, sizeof(relayButtons));
  mqttStatusIndicator = nullptr;
  wifiStatusIndicator = nullptr;
  rgbColorDots[0] = nullptr;
  rgbColorDots[1] = nullptr;
  rgbColorDots[2] = nullptr;

  createHeader(screen);
  createMainCanvas(screen);
  createStatusBar(screen);

  // Force a full redraw after all objects have been created so a later refresh does not drop the later widgets.
  lv_obj_invalidate(screen);
  lv_refr_now(NULL);
}

void lvgl_set_mqtt_connected(bool connected)
{
  mqttConnected = connected;
  if (mqttStatusIndicator != nullptr)
  {
    lv_color_t color = connected ? lv_color_hex(STATUS_GREEN) : lv_color_hex(STATUS_RED);
    lv_obj_set_style_bg_color(mqttStatusIndicator, color, LV_PART_MAIN);
  }

  for (int i = 0; i < 4; ++i)
  {
    updateRelayButtonColor(i);
  }
  updateRgbAnimationState();
}

void lvgl_set_relay_state(int relayIndex, bool isOn)
{
  if (relayIndex < 0 || relayIndex >= 4)
  {
    return;
  }

  relayStates[relayIndex] = isOn;
  relayStatesKnown[relayIndex] = true;
  updateRelayButtonColor(relayIndex);
}

void lvgl_set_wifi_connected(bool connected)
{
  wifiConnected = connected;
  if (wifiStatusIndicator != nullptr)
  {
    lv_color_t color = connected ? lv_color_hex(STATUS_GREEN) : lv_color_hex(STATUS_RED);
    lv_obj_set_style_bg_color(wifiStatusIndicator, color, LV_PART_MAIN);
  }

  updateRgbAnimationState();
}

lv_obj_t *addButton(lv_obj_t *parent, const char *text, int x, int y, int w, int h, lv_event_cb_t cb)
{
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, w, h);
  lv_obj_set_pos(btn, x, y);
  lv_obj_set_style_bg_color(btn, lv_color_hex(0xD3D3D3), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(btn, lv_color_hex(0x303030), LV_PART_MAIN | LV_STATE_DEFAULT);
  if (cb)
  {
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
  }
  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_center(label);
  return btn;
}

#ifdef LVGL_LIVE_PREVIEW
// Provide the live preview tool a way to initialize the UI
void lvgl_live_preview_init(void)
{
  lvgl_wrapper_init();
}
#endif