#include "lvgl_wrapper.h"
#include <Arduino.h>
#include <lvgl.h>

#define LV_HOR_RES_MAX 320
#define LV_VER_RES_MAX 240

static void button_event_cb(lv_event_t *e)
{
  if (lv_event_get_code(e) == LV_EVENT_CLICKED)
  {
    lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    const char *text = lv_label_get_text(label);
    Serial.print("Button pressed: ");
    Serial.println(text);
  }
}

void lvgl_wrapper_init()
{
  lv_obj_t *screen = lv_scr_act();
  // Create buttons via helper to reduce duplication
  addButton(screen, "Button 1", (LV_HOR_RES_MAX / 2) - 80 - 60, (LV_VER_RES_MAX / 2) - 30, 120, 60, button_event_cb);
  addButton(screen, "Button 2", (LV_HOR_RES_MAX / 2) + 80 - 60, (LV_VER_RES_MAX / 2) - 30, 120, 60, button_event_cb);
}

lv_obj_t *addButton(lv_obj_t *parent, const char *text, int x, int y, int w, int h, lv_event_cb_t cb)
{
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, w, h);
  lv_obj_set_pos(btn, x, y);
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