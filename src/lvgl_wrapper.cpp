#include "lvgl_wrapper.h"
#include <Arduino.h>
#include <lvgl.h>

#define LV_HOR_RES_MAX 320
#define LV_VER_RES_MAX 240

lv_obj_t *relayButtons[4] = {nullptr, nullptr, nullptr, nullptr};

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

  // If this initializer is called again, clear the previous screen state first.
  lv_obj_clean(screen);
  memset(relayButtons, 0, sizeof(relayButtons));

  // Create the initial demo button.
  // addButton(screen, "Button 1", (LV_HOR_RES_MAX / 2) - 80 - 60, (LV_VER_RES_MAX / 2) - 30, 120, 60, button_event_cb);
  // addButton(screen, "Button 2", (LV_HOR_RES_MAX / 2) + 80 - 60, (LV_VER_RES_MAX / 2) - 30, 120, 60, button_event_cb);

  // Create the four relay buttons.
  relayButtons[0] = addButton(screen, "Relay 1", 20, 20, 120, 60, button_event_cb);
  relayButtons[1] = addButton(screen, "Relay 2", 180, 20, 120, 60, button_event_cb);
  relayButtons[2] = addButton(screen, "Relay 3", 20, 120, 120, 60, button_event_cb);
  relayButtons[3] = addButton(screen, "Relay 4", 180, 120, 120, 60, button_event_cb);

  // Force a full redraw after all objects have been created so a later refresh does not drop the later widgets.
  lv_obj_invalidate(screen);
  lv_refr_now(NULL);
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