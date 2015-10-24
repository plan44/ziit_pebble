/*

  Ziit watch
  (c) 2010-2014 by Lukas Zeller + Frank Hürlimann, mixwerk.ch, plan44.ch

 */

#include <pebble.h>

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
Window *window;
// - layers
Layer *second_display_layer;
TextLayer *ziit_display_textlayer;
// - internal variables
int currentSecond;
int textValid;
// - the ziit text string
#define ZIIT_STR_BUFFER_BYTES 200
char ziit_str_buffer[ZIIT_STR_BUFFER_BYTES];

// Geometry constants
#define TEXT_LAYER_MINMARGIN 9
#define TEXT_LAYER_EXTRAMARGIN_H 17
#ifdef PBL_ROUND
#define DOT_MARGIN 10
#define TEXT_SHIFTDOWN 15
#else
#define DOT_MARGIN 5
#define TEXT_SHIFTDOWN 0
#endif
#define DOT_RADIUS 4


void second_display_layer_callback(struct Layer *layer, GContext *ctx)
{
  // unsigned int angle = currentSecond * 6;
  int32_t hexAngle = (int32_t)currentSecond * 1092;
  GRect f = layer_get_frame(layer);
  GPoint center = grect_center_point(&f);
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
void handle_second_tick(struct tm *tick_time, TimeUnits units_changed)
{
  currentSecond = tick_time->tm_sec;
  #if DEBUG_SECONDS_AS_MIN
  textValid = 0; // update text every second
  #else
  if (units_changed & MINUTE_UNIT) textValid = 0; // new minute, needs new text
  #endif
  if (!textValid) {
    #if DEBUG_SECONDS_AS_MIN
    int16_t minTextIndex = currentSecond;
    int16_t hourTextIndex = LONGEST_HOUR_TEXT; // longest text
    #else
    int16_t minTextIndex = tick_time->tm_min;
    int16_t hourTextIndex = (tick_time->tm_hour + (minTextIndex>=NEXT_HOUR_REF_MIN ? 1 : 0)) % 12;
    #endif
    strcpy(ziit_str_buffer, "es isch\n");
    strcat(ziit_str_buffer, minuteTexts[minTextIndex]); // includes needed line break (usually at end, sometimes in the middle)
    strcat(ziit_str_buffer, hourTexts[hourTextIndex]);
    text_layer_set_text(ziit_display_textlayer, ziit_str_buffer);
    textValid = 1; // now valid
  }
  // anyway, update second
  layer_mark_dirty(second_display_layer);
}



void init()
{
  // the window
  window = window_create();
  window_set_background_color(window, GColorBlack);
  window_stack_push(window, true /* Animated */);

  // init the engine
  textValid = 0; // text not valid
  struct tm *t;
  time_t now = time(NULL);
  t = localtime(&now);
  currentSecond = t->tm_sec; // start with correct second position

  // get the window frame
  GRect wf = layer_get_frame(window_get_root_layer(window));
  // the text layer for displaying the Ziit time text
  GRect tf = wf; // start calculation with window frame
  // - centered square
  tf.origin.x += TEXT_LAYER_MINMARGIN;
  tf.size.w -= 2*TEXT_LAYER_MINMARGIN;
  tf.origin.y += (tf.size.h-tf.size.w) / 2 + TEXT_LAYER_EXTRAMARGIN_H + TEXT_SHIFTDOWN;
  tf.size.h = tf.size.w - 2*TEXT_LAYER_EXTRAMARGIN_H;
  ziit_display_textlayer = text_layer_create(tf);
  // - parameters
  text_layer_set_text_alignment(ziit_display_textlayer, GTextAlignmentCenter); // centered
  text_layer_set_background_color(ziit_display_textlayer, GColorBlack); // black background
  text_layer_set_font(ziit_display_textlayer, fonts_get_system_font(FONT_KEY_GOTHIC_28)); // font
  text_layer_set_text_color(ziit_display_textlayer, GColorWhite); // white text
  // - text
  strcpy(ziit_str_buffer, "Ziit\n©2013\nplan44.ch");
  text_layer_set_text(ziit_display_textlayer, ziit_str_buffer);
  layer_add_child(window_get_root_layer(window), (Layer *)ziit_display_textlayer);

  // the layer for displaying the seconds dot
  second_display_layer = layer_create(wf);
  #ifdef PBL_ROUND
  #endif
  layer_set_update_proc(second_display_layer, &second_display_layer_callback);
  layer_add_child(window_get_root_layer(window), second_display_layer);
  layer_mark_dirty(second_display_layer); // draw immediately

  // now subscribe to ticks
  tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
}


void deinit()
{
  layer_destroy(second_display_layer);
  text_layer_destroy(ziit_display_textlayer);
  window_destroy(window);
}



int main(void)
{
  init();
  app_event_loop();
  deinit();
  return 0;
}
