/*

  Ziit watch
  (c) 2010-2013 by Lukas Zeller + Frank Hürlimann, mixwerk.ch, plan44.ch

 */

#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0xFF, 0xA3, 0x15, 0x94, 0x1D, 0x30, 0x44, 0x99, 0xBF, 0xA1, 0x00, 0x74, 0x48, 0xF1, 0xE5, 0xCD }
PBL_APP_INFO(MY_UUID,
             "Ziit", "plan44.ch",
             1, 0, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);

// texts
const char * const hourTexts[12] = {
  "zwölfi",
  "eis",
  "zwei",
  "drü",
  "vieri",
  "foifi",
  "sächsi",
  "sibni",
  "achti",
  "nüni",
  "zäni",
  "elfi"
};

#define NEXT_HOUR_REF_MIN 25
const char * const minuteTexts[60] = {
  "genau",
  "eis ab",
  "zwei ab",
  "drü ab",
  "vier ab",
  "foif ab",
  "sächs ab",
  "sibe ab",
  "acht ab",
  "nün ab",
  "zä ab",
  "elf ab",
  "zwölf ab",
  "drizä ab",
  "virzä ab",
  "viertel ab",
  "sächzä ab",
  "sibzä ab",
  "achzä ab",
  "nünzä ab",
  "zwänzg ab",
  "einezwänzg ab",
  "zweiezwänzg ab",
  "drüezwänzg ab",
  "vierezwänzg ab",
  "foif vor halbi",
  "vier vor halbi",
  "drü vor halbi",
  "zwei vor halbi",
  "eis vor halbi",
  "halbi",
  "eis ab halbi",
  "zwei ab halbi",
  "drü ab halbi",
  "vier ab halbi",
  "foifezwänzg vor",
  "sächs ab halbi",
  "sibe ab halbi",
  "acht ab halbi",
  "nün ab halbi",
  "zwänzg vor",
  "nünzä vor",
  "achzä vor",
  "sibzä vor",
  "sächzä vor",
  "viertel vor",
  "virzä vor",
  "drizä vor",
  "zwölf vor",
  "elf vor",
  "zä vor",
  "nün vor",
  "acht vor",
  "sibe vor",
  "sächs vor",
  "foif vor",
  "vier vor",
  "drü vor",
  "zwei vor",
  "eis vor"
};


// Variables
// - the window
Window window;
// - layers
Layer second_display_layer;
TextLayer ziit_display_textlayer;
// - internal variables
int currentSecond;
int textValid;
// - the ziit text string
#define ZIIT_STR_BUFFER_BYTES 200
char ziit_str_buffer[ZIIT_STR_BUFFER_BYTES];

// Geometry constants
#define TEXT_LAYER_MINMARGIN 9
#define TEXT_LAYER_EXTRAMARGIN_H 11
#define DOT_MARGIN 5
#define DOT_RADIUS 4


void second_display_layer_callback(Layer *me, GContext* ctx)
{
  // unsigned int angle = currentSecond * 6;
  int32_t hexAngle = (int32_t)currentSecond * 1092;
  GPoint center = grect_center_point(&me->frame);
  int32_t r = center.x-DOT_MARGIN;
  GPoint dotPos;
  dotPos.x = center.x + sin_lookup(hexAngle)*r/0xFFFFl;
  dotPos.y = center.y - cos_lookup(hexAngle)*r/0xFFFFl;
  graphics_context_set_fill_color(ctx, GColorWhite);
  // paint the dot
  graphics_fill_circle(ctx, dotPos, DOT_RADIUS);
}


// handle second tick
void handle_second_tick(AppContextRef ctx, PebbleTickEvent *event)
{
  currentSecond = event->tick_time->tm_sec;
  if (currentSecond==0) textValid = 0; // new minute, needs new text
  if (!textValid) {
    string_format_time(ziit_str_buffer, ZIIT_STR_BUFFER_BYTES, "%H:%M", event->tick_time);
    int16_t minTextIndex = event->tick_time->tm_min;
    int16_t hourTextIndex = (event->tick_time->tm_hour + (minTextIndex>=NEXT_HOUR_REF_MIN ? 1 : 0)) % 12;
    strcpy(ziit_str_buffer, "es isch\n");
    strcat(ziit_str_buffer, minuteTexts[minTextIndex]);
    strcat(ziit_str_buffer, "\n");
    strcat(ziit_str_buffer, hourTexts[hourTextIndex]);
    text_layer_set_text(&ziit_display_textlayer, ziit_str_buffer);
    textValid = 1; // now valid
  }
  // anyway, update second
  layer_mark_dirty(&second_display_layer);
}



void handle_init(AppContextRef ctx)
{
  (void)ctx;

  // the window
  window_init(&window, "Ziit");
  window_set_background_color(&window, GColorBlack);
  window_stack_push(&window, true /* Animated */);

  // the text layer for displaying the Ziit time text
  GRect tf = window.layer.frame;
  // - centered square
  tf.origin.x += TEXT_LAYER_MINMARGIN;
  tf.size.w -= 2*TEXT_LAYER_MINMARGIN;
  tf.origin.y += (tf.size.h-tf.size.w) / 2 + TEXT_LAYER_EXTRAMARGIN_H;
  tf.size.h = tf.size.w - 2*TEXT_LAYER_EXTRAMARGIN_H;
  text_layer_init(&ziit_display_textlayer, tf);
  // - parameters
  text_layer_set_text_alignment(&ziit_display_textlayer, GTextAlignmentCenter); // centered
  text_layer_set_background_color(&ziit_display_textlayer, GColorBlack); // black background
  text_layer_set_font(&ziit_display_textlayer, fonts_get_system_font(FONT_KEY_GOTHIC_28)); // font
  text_layer_set_text_color(&ziit_display_textlayer, GColorWhite); // white text
  // - text
  strcpy(ziit_str_buffer, "???");
  text_layer_set_text(&ziit_display_textlayer, ziit_str_buffer);
  layer_add_child(&window.layer, &ziit_display_textlayer.layer);

  // the layer for displaying the seconds dot
  layer_init(&second_display_layer, window.layer.frame);
  second_display_layer.update_proc = &second_display_layer_callback;
  layer_add_child(&window.layer, &second_display_layer);

  // init the engine
  textValid = 0; // text not valid
}



void pbl_main(void *params)
{
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .tick_info = {
      .tick_handler = &handle_second_tick,
      .tick_units = SECOND_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
