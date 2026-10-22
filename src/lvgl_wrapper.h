// LVGL wrapper header — creates UI elements
#pragma once
#include <stdint.h>
#include <lvgl.h>

void lvgl_wrapper_init();
void lvgl_set_mqtt_connected(bool connected);
void lvgl_set_relay_state(int relayIndex, bool isOn);
void lvgl_set_timer_state(int relayIndex, uint32_t remainingSeconds);
void lvgl_set_wifi_connected(bool connected);
void publishRelayToggle(int relayIndex);
bool publishRelayTimer(int relayIndex, uint32_t durationSeconds);
bool cancelRelayTimer(int relayIndex);
lv_obj_t *addButton(lv_obj_t *parent, const char *text, int x, int y, int w, int h, lv_event_cb_t cb);
extern lv_obj_t *relayButtons[4];
