#include "lvgl.h"

// initializes your UI (entry point of your application)
static void ui_init(void)
{
  // Create a simple button
  lv_obj_t *btn = lv_btn_create(lv_scr_act());
  lv_obj_set_size(btn, 120, 50);
  lv_obj_center(btn);

  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, "Hello World!");
  lv_obj_center(label);
}

#ifdef LVGL_LIVE_PREVIEW
// gives the live preview tool a way to initialize your UI
void lvgl_live_preview_init(void)
{
  ui_init();
}
#endif