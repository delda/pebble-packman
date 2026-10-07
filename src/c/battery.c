#include <pebble.h>

#include "battery.h"

#if defined(PBL_PLATFORM_GABBRO) || defined(PBL_PLATFORM_CHALK) || defined(PBL_PLATFORM_EMERY)

#define PERSIST_KEY_BATTERY_MODE 2
#define BATTERY_MODE_VISUAL 0
#define BATTERY_MODE_PERCENTAGE 1
#define BATTERY_MODE_HIDDEN 2
#define BATTERY_WIDTH 30
#define BATTERY_HEIGHT 19

static Layer *s_face_layer;
static int s_battery_level;
static int s_battery_mode;

static void battery_mark_dirty(void) {
  if (s_face_layer) layer_mark_dirty(s_face_layer);
}

static void battery_callback(BatteryChargeState state) {
  s_battery_level = state.charge_percent;
  battery_mark_dirty();
}

void battery_set_mode(int mode) {
  s_battery_mode = mode == BATTERY_MODE_PERCENTAGE
                     ? BATTERY_MODE_PERCENTAGE
                     : mode == BATTERY_MODE_HIDDEN ? BATTERY_MODE_HIDDEN
                                                   : BATTERY_MODE_VISUAL;
  persist_write_int(PERSIST_KEY_BATTERY_MODE, s_battery_mode);
  battery_mark_dirty();
}

bool battery_is_visible(void) {
  return s_battery_mode != BATTERY_MODE_HIDDEN;
}

void battery_initialize(Layer *face_layer) {
  s_face_layer = face_layer;
  s_battery_mode = persist_exists(PERSIST_KEY_BATTERY_MODE)
                     ? persist_read_int(PERSIST_KEY_BATTERY_MODE)
                     : BATTERY_MODE_VISUAL;
  if (s_battery_mode != BATTERY_MODE_PERCENTAGE &&
      s_battery_mode != BATTERY_MODE_HIDDEN) {
    s_battery_mode = BATTERY_MODE_VISUAL;
  }
  battery_callback(battery_state_service_peek());
  battery_state_service_subscribe(battery_callback);
}

void battery_deinitialize(void) {
  battery_state_service_unsubscribe();
  s_face_layer = NULL;
}

void battery_draw(GContext *ctx, GPoint center) {
  if (s_battery_mode == BATTERY_MODE_HIDDEN) {
    return;
  }
  // Same proportions as pebble-BinaryWatchFace; the center comes from layout.c.
  // Nine 2px bars with 1px spacing need 29px of inner width; leave one
  // additional pixel for the outline so the last bar stays inside the body.
  const int width = BATTERY_WIDTH;
  // Golden-ratio height: 30 / 1.618 ≈ 18.5, rounded to 19 pixels.
  const int height = BATTERY_HEIGHT;
  const int x = center.x - width / 2;
  const int y = center.y - height / 2;

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_text_color(ctx, GColorBlack);

  if (s_battery_mode == BATTERY_MODE_PERCENTAGE) {
    char battery_text[5];
    snprintf(battery_text, sizeof(battery_text), "%d%%", s_battery_level);
    graphics_draw_rect(ctx, GRect(x, y, width, height));
    graphics_fill_rect(ctx, GRect(x + width, y + 3, 2, 12), 0, GCornerNone);
    graphics_draw_text(ctx, battery_text, fonts_get_system_font(FONT_KEY_GOTHIC_18),
                       GRect(x - 2, y - 3, width + 4, 14), GTextOverflowModeFill,
                       GTextAlignmentCenter, NULL);
    return;
  }

  graphics_draw_rect(ctx, GRect(x, y, width, height));
  graphics_fill_rect(ctx, GRect(x + width, y + 3, 2, 12), 0, GCornerNone);
  int battery_blocks = s_battery_level / 10;
  if (battery_blocks > 9) battery_blocks = 9;
  for (int block = 0; block < battery_blocks; ++block) {
    graphics_fill_rect(ctx, GRect(x + 2 + block * 3, y + 2, 2, height - 4), 0, GCornerNone);
  }
}

#else

void battery_initialize(Layer *face_layer) { (void)face_layer; }
void battery_deinitialize(void) {}
void battery_draw(GContext *ctx, GPoint center) { (void)ctx; (void)center; }
void battery_set_mode(int mode) { (void)mode; }
bool battery_is_visible(void) { return false; }

#endif
