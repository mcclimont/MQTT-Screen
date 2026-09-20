// LVGL wrapper header — creates UI elements
#pragma once
#include <lvgl.h>

void lvgl_wrapper_init();
lv_obj_t *addButton(lv_obj_t *parent, const char *text, int x, int y, int w, int h, lv_event_cb_t cb);
extern lv_obj_t *relayButtons[4];

// Provide the live preview tool a way to initialize the UI
extern "C" void lvgl_live_preview_init(void);