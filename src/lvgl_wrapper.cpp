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
static lv_obj_t *timerButtons[4] = {nullptr, nullptr, nullptr, nullptr};
static bool relayStates[4] = {false, false, false, false};
static bool relayStatesKnown[4] = {false, false, false, false};
static bool timerStatesKnown[4] = {false, false, false, false};
static uint32_t timerReportedSeconds[4] = {0, 0, 0, 0};
static uint32_t timerReportedAtMs[4] = {0, 0, 0, 0};
static bool mqttConnected = false;
static bool wifiConnected = false;
static lv_obj_t *mqttStatusIndicator = nullptr;
static lv_obj_t *wifiStatusIndicator = nullptr;
static lv_obj_t *rgbColorDots[3] = {nullptr, nullptr, nullptr};
static lv_timer_t *rgbAnimationTimer = nullptr;
static lv_timer_t *timerDisplayTimer = nullptr;
static lv_obj_t *timerModal = nullptr;
static lv_obj_t *timerModalMessage = nullptr;
static int selectedRelayIndex = -1;
static uint8_t rgbAnimationPhase = 0;
static const uint32_t STATUS_RED = 0xFF0000;
static const uint32_t STATUS_GREEN = 0x00FF00;
static const uint32_t RELAY_BUTTON_CONNECTED_ON = 0x19e84c;     // 0x438B55
static const uint32_t RELAY_BUTTON_CONNECTED_OFF = 0x500a0a;    // 0xB85B5B
static const uint32_t RELAY_BUTTON_DISCONNECTED_ON = 0x326940;  // 0x526B58
static const uint32_t RELAY_BUTTON_DISCONNECTED_OFF = 0x2e0606; // 0x725454
static const uint32_t RELAY_BUTTON_INITIAL = 0xD3D3D3;
static const uint32_t TIMER_BUTTON_IDLE = 0x315A5A;
static const uint32_t TIMER_BUTTON_ACTIVE = 0x35D0BA;

static void updateTimerButton(int relayIndex)
{
  if (relayIndex < 0 || relayIndex >= 4 || timerButtons[relayIndex] == nullptr)
  {
    return;
  }

  lv_obj_t *label = lv_obj_get_child(timerButtons[relayIndex], 0);
  if (!mqttConnected)
  {
    lv_label_set_text(label, "--");
    lv_obj_set_style_bg_color(timerButtons[relayIndex], lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(timerButtons[relayIndex], lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
    return;
  }

  if (!timerStatesKnown[relayIndex])
  {
    lv_label_set_text(label, "...");
    lv_obj_set_style_bg_color(timerButtons[relayIndex], lv_color_hex(0x666666), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(timerButtons[relayIndex], lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
    return;
  }

  const uint32_t elapsedSeconds = (millis() - timerReportedAtMs[relayIndex]) / 1000UL;
  const uint32_t remainingSeconds = elapsedSeconds >= timerReportedSeconds[relayIndex]
                                        ? 0
                                        : timerReportedSeconds[relayIndex] - elapsedSeconds;
  char remainingLabel[8];
  if (remainingSeconds == 0)
  {
    lv_label_set_text(label, "Set");
  }
  else if (remainingSeconds >= 3600)
  {
    snprintf(remainingLabel, sizeof(remainingLabel), "%luh%02lu",
             static_cast<unsigned long>(remainingSeconds / 3600),
             static_cast<unsigned long>((remainingSeconds / 60) % 60));
    lv_label_set_text(label, remainingLabel);
  }
  else if (remainingSeconds >= 60)
  {
    snprintf(remainingLabel, sizeof(remainingLabel), "%lum",
             static_cast<unsigned long>(remainingSeconds / 60));
    lv_label_set_text(label, remainingLabel);
  }
  else
  {
    snprintf(remainingLabel, sizeof(remainingLabel), "%lus",
             static_cast<unsigned long>(remainingSeconds));
    lv_label_set_text(label, remainingLabel);
  }

  const bool active = remainingSeconds > 0;
  lv_obj_set_style_bg_color(timerButtons[relayIndex],
                            lv_color_hex(active ? TIMER_BUTTON_ACTIVE : TIMER_BUTTON_IDLE),
                            LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(timerButtons[relayIndex],
                              lv_color_hex(active ? 0x073B38 : 0xFFFFFF),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void timerDisplayTimerCb(lv_timer_t *timer)
{
  (void)timer;
  for (int relayIndex = 0; relayIndex < 4; ++relayIndex)
  {
    updateTimerButton(relayIndex);
  }
}

static void closeTimerModal()
{
  if (timerModal != nullptr)
  {
    lv_obj_delete(timerModal);
    timerModal = nullptr;
  }

  timerModalMessage = nullptr;
  selectedRelayIndex = -1;
}

static void timerModalCloseEventCb(lv_event_t *event)
{
  if (lv_event_get_code(event) == LV_EVENT_CLICKED)
  {
    closeTimerModal();
  }
}

static void timerModalOverlayEventCb(lv_event_t *event)
{
  if (lv_event_get_code(event) == LV_EVENT_CLICKED && lv_event_get_target(event) == timerModal)
  {
    closeTimerModal();
  }
}

static void timerDurationEventCb(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED || selectedRelayIndex < 0)
  {
    return;
  }

  const uint32_t durationSeconds = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(event)));
  if (publishRelayTimer(selectedRelayIndex, durationSeconds))
  {
    closeTimerModal();
  }
  else
  {
    lv_label_set_text(timerModalMessage, "Timer command was not sent.");
  }
}

static void timerCancelEventCb(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED || selectedRelayIndex < 0)
  {
    return;
  }

  if (cancelRelayTimer(selectedRelayIndex))
  {
    closeTimerModal();
  }
  else
  {
    lv_label_set_text(timerModalMessage, "Timer command was not sent.");
  }
}

static lv_obj_t *createModalButton(lv_obj_t *parent, const char *text, int x, int y, int width, int height,
                                   lv_event_cb_t callback, void *userData = nullptr)
{
  lv_obj_t *button = addButton(parent, text, x, y, width, height, nullptr);
  lv_obj_set_style_radius(button, 5, LV_PART_MAIN);
  lv_obj_set_style_bg_color(button, lv_color_hex(0x35D0BA), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(button, lv_color_hex(0x073B38), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, userData);
  return button;
}

static void openTimerModal(int relayIndex)
{
  if (relayIndex < 0 || relayIndex >= 4 || timerModal != nullptr || !mqttConnected)
  {
    return;
  }

  selectedRelayIndex = relayIndex;
  lv_obj_t *screen = lv_scr_act();
  timerModal = lv_obj_create(screen);
  lv_obj_set_size(timerModal, LV_HOR_RES_MAX, LV_VER_RES_MAX);
  lv_obj_set_pos(timerModal, 0, 0);
  lv_obj_set_style_bg_color(timerModal, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(timerModal, LV_OPA_60, LV_PART_MAIN);
  lv_obj_set_style_border_width(timerModal, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(timerModal, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(timerModal, timerModalOverlayEventCb, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *panel = lv_obj_create(timerModal);
  lv_obj_set_size(panel, 276, 186);
  lv_obj_set_pos(panel, 22, 27);
  lv_obj_set_style_radius(panel, 8, LV_PART_MAIN);
  lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(panel, lv_color_hex(0x35D0BA), LV_PART_MAIN);
  lv_obj_set_style_bg_color(panel, lv_color_hex(0x173A3A), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN);

  lv_obj_t *title = lv_label_create(panel);
  lv_label_set_text_fmt(title, "Light %d timer", relayIndex + 1);
  lv_obj_set_pos(title, 14, 9);
  lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
  createModalButton(panel, "X", 242, 4, 28, 24, timerModalCloseEventCb);

  static const char *durations[] = {"1 hour", "2 hours", "4 hours", "8 hours"};
  static const uint32_t durationSeconds[] = {3600, 7200, 14400, 28800};
  const int buttonX[] = {18, 146, 18, 146};
  const int buttonY[] = {45, 45, 85, 85};
  for (uint8_t option = 0; option < 4; ++option)
  {
    createModalButton(panel, durations[option], buttonX[option], buttonY[option], 112, 34,
                      timerDurationEventCb,
                      reinterpret_cast<void *>(static_cast<uintptr_t>(durationSeconds[option])));
  }

  timerModalMessage = lv_label_create(panel);
  lv_label_set_text(timerModalMessage, "");
  lv_obj_set_pos(timerModalMessage, 12, 124);
  lv_obj_set_style_text_color(timerModalMessage, lv_color_hex(0xFFB2A8), LV_PART_MAIN);

  createModalButton(panel, "Cancel timer", 15, 153, 246, 25, timerCancelEventCb);
}

static void timerButtonEventCb(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED)
  {
    return;
  }

  const intptr_t relayIndex = reinterpret_cast<intptr_t>(lv_event_get_user_data(event)) - 1;
  openTimerModal(static_cast<int>(relayIndex));
}

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
  const int tileWidth = 130;
  const int buttonWidth = 82;
  const int timerButtonWidth = 44;
  const int buttonHeight = 60;
  const int gridColumnWidth = MAIN_CANVAS_WIDTH / 2;
  const int verticalSpace = MAIN_CANVAS_HEIGHT - (2 * buttonHeight);
  const char *buttonLabels[] = {"Light 1", "Light 2", "Light 3", "Light 4"};

  for (uint8_t i = 0; i < 4; ++i)
  {
    const int column = i % 2;
    const int row = i / 2;
    const int x = MAIN_CANVAS_X + column * gridColumnWidth + (gridColumnWidth - tileWidth) / 2;
    const int y = MAIN_CANVAS_Y + ((row + 1) * verticalSpace) / 3 + row * buttonHeight;
    relayButtons[i] = addButton(screen, buttonLabels[i], x, y, buttonWidth, buttonHeight, button_event_cb);
    timerButtons[i] = addButton(screen, "Set", x + buttonWidth + 4, y, timerButtonWidth, buttonHeight, nullptr);
    lv_obj_set_style_radius(timerButtons[i], 10, LV_PART_MAIN);
    lv_obj_add_event_cb(timerButtons[i], timerButtonEventCb, LV_EVENT_CLICKED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(i + 1)));
    updateRelayButtonColor(i);
    updateTimerButton(i);
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
  if (timerDisplayTimer != nullptr)
  {
    lv_timer_delete(timerDisplayTimer);
    timerDisplayTimer = nullptr;
  }
  timerModal = nullptr;
  timerModalMessage = nullptr;
  lv_obj_clean(screen);
  lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
  memset(relayButtons, 0, sizeof(relayButtons));
  memset(timerButtons, 0, sizeof(timerButtons));
  mqttStatusIndicator = nullptr;
  wifiStatusIndicator = nullptr;
  rgbColorDots[0] = nullptr;
  rgbColorDots[1] = nullptr;
  rgbColorDots[2] = nullptr;

  createHeader(screen);
  createMainCanvas(screen);
  createStatusBar(screen);
  timerDisplayTimer = lv_timer_create(timerDisplayTimerCb, 1000, nullptr);

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
    updateTimerButton(i);
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

void lvgl_set_timer_state(int relayIndex, uint32_t remainingSeconds)
{
  if (relayIndex < 0 || relayIndex >= 4)
  {
    return;
  }

  timerReportedSeconds[relayIndex] = remainingSeconds;
  timerReportedAtMs[relayIndex] = millis();
  timerStatesKnown[relayIndex] = true;
  updateTimerButton(relayIndex);
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
