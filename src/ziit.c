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
             1, 1, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);

// hour texts
#define LONGEST_HOUR_TEXT 6 // for debugging layout
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

// minute texts
#define NEXT_HOUR_REF_MIN 25 // first minute which refers to coming hour (rather than current hour). Like in "10 to 7" = 6:50
const char * const minuteTexts[60] = {
  "genau\n",
  "eis ab\n",
  "zwei ab\n",
  "drü ab\n",
  "vier ab\n",
  "foif ab\n",
  "sächs ab\n",
  "sibe ab\n",
  "acht ab\n",
  "nün ab\n",
  "zä ab\n",
  "elf ab\n",
  "zwölf ab\n",
  "drizä ab\n",
  "virzä ab\n",
  "viertel ab\n",
  "sächzä ab\n",
  "sibzä ab\n",
  "achzä ab\n",
  "nünzä ab\n",
  "zwänzg ab\n",
  "einezwänzg\nab ",
  "zweiezwänzg\nab ",
  "drüezwänzg\nab ",
  "vierezwänzg\nab ",
  "foif vor halbi\n",
  "vier vor halbi\n",
  "drü vor halbi\n",
  "zwei vor halbi\n",
  "eis vor halbi\n",
  "halbi\n",
  "eis ab halbi\n",
  "zwei ab halbi\n",
  "drü ab halbi\n",
  "vier ab halbi\n",
  "foif ab halbi\n",
  "sächs ab\nhalbi ",
  "sibe ab halbi\n",
  "acht ab halbi\n",
  "nün ab halbi\n",
  "zwänzg vor\n",
  "nünzä vor\n",
  "achzä vor\n",
  "sibzä vor\n",
  "sächzä vor\n",
  "viertel vor\n",
  "virzä vor\n",
  "drizä vor\n",
  "zwölf vor\n",
  "elf vor\n",
  "zä vor\n",
  "nün vor\n",
  "acht vor\n",
  "sibe vor\n",
  "sächs vor\n",
  "foif vor\n",
  "vier vor\n",
  "drü vor\n",
  "zwei vor\n",
  "eis vor\n"
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
#define TEXT_LAYER_EXTRAMARGIN_H 17
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


// set this to 1 to quickly see all minute texts, one per second, to debug layout
// It also displays the LONGEST_HOUR_TEXT instead of the current hour
#define DEBUG_SECONDS_AS_MIN 0

// handle second tick
void handle_second_tick(AppContextRef ctx, PebbleTickEvent *event)
{
  currentSecond = event->tick_time->tm_sec;
  #if DEBUG_SECONDS_AS_MIN
  textValid = 0; // update text every second
  #else
  if (currentSecond==0) textValid = 0; // new minute, needs new text
  #endif
  if (!textValid) {
    #if DEBUG_SECONDS_AS_MIN
    int16_t minTextIndex = currentSecond;
    int16_t hourTextIndex = LONGEST_HOUR_TEXT; // longest text
    #else
    int16_t minTextIndex = event->tick_time->tm_min;
    int16_t hourTextIndex = (event->tick_time->tm_hour + (minTextIndex>=NEXT_HOUR_REF_MIN ? 1 : 0)) % 12;
    #endif
    strcpy(ziit_str_buffer, "es isch\n");
    strcat(ziit_str_buffer, minuteTexts[minTextIndex]); // includes needed line break (usually at end, sometimes in the middle)
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

  // init the engine
  textValid = 0; // text not valid
  PblTm t;
  get_time(&t);
  currentSecond = t.tm_sec; // start with correct second position

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
  strcpy(ziit_str_buffer, "Ziit\n©2013\nplan44.ch");
  text_layer_set_text(&ziit_display_textlayer, ziit_str_buffer);
  layer_add_child(&window.layer, &ziit_display_textlayer.layer);

  // the layer for displaying the seconds dot
  layer_init(&second_display_layer, window.layer.frame);
  second_display_layer.update_proc = &second_display_layer_callback;
  layer_add_child(&window.layer, &second_display_layer);
  layer_mark_dirty(&second_display_layer); // draw immediately

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
