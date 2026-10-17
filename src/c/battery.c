#include <pebble.h>

#include "battery.h"

#if defined(PBL_PLATFORM_GABBRO) || defined(PBL_PLATFORM_CHALK) || \
    defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_BASALT) || \
    defined(PBL_PLATFORM_FLINT) || defined(PBL_PLATFORM_APLITE) || \
    defined(PBL_PLATFORM_DIORITE)

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

int battery_get_width(void) {
#if defined(PBL_PLATFORM_CHALK) || defined(PBL_PLATFORM_BASALT) || \
    defined(PBL_PLATFORM_FLINT) || defined(PBL_PLATFORM_APLITE) || \
    defined(PBL_PLATFORM_DIORITE)
  const int width = 21;
#else
  const int width = BATTERY_WIDTH;
#endif
  // Keep three digits and the percent sign readable inside the rounded shell.
  return width + (s_battery_mode == BATTERY_MODE_PERCENTAGE ? 6 : 0);
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
  const int width = battery_get_width();
#if defined(PBL_PLATFORM_CHALK) || defined(PBL_PLATFORM_BASALT) || \
    defined(PBL_PLATFORM_FLINT) || defined(PBL_PLATFORM_APLITE) || \
    defined(PBL_PLATFORM_DIORITE)
  const int height = 13;
  const int border = 1;
  const int corner_radius = 3;
#else
  const int height = BATTERY_HEIGHT;
  const int border = 2;
  const int corner_radius = 4;
#endif
  const int x = center.x - width / 2;
  const int y = center.y - height / 2;
  const int button_height = height / 3;
  const int button_y = y + (height - button_height) / 2;
  const int inner_width = width - 2 * border;
  const int inner_height = height - 2 * border;
  const int inner_radius = corner_radius - border;

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_text_color(ctx, GColorBlack);

  // A rounded shell and a solid charge area, with the positive terminal right.
  graphics_fill_rect(ctx, GRect(x, y, width, height), corner_radius, GCornersAll);
  graphics_fill_rect(ctx, GRect(x + width, button_y, 2, button_height), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(x + border, y + border, inner_width, inner_height),
                     inner_radius, GCornersAll);
  graphics_context_set_fill_color(ctx, GColorBlack);

  if (s_battery_mode == BATTERY_MODE_PERCENTAGE) {
    char battery_text[5];
    snprintf(battery_text, sizeof(battery_text), "%d%%", s_battery_level);
#if defined(PBL_PLATFORM_CHALK) || defined(PBL_PLATFORM_BASALT) || \
    defined(PBL_PLATFORM_FLINT) || defined(PBL_PLATFORM_APLITE) || \
    defined(PBL_PLATFORM_DIORITE)
    graphics_draw_text(ctx, battery_text, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(x - 2, y - 3, width + 4, height + 2), GTextOverflowModeFill,
                       GTextAlignmentCenter, NULL);
#else
    graphics_draw_text(ctx, battery_text, fonts_get_system_font(FONT_KEY_GOTHIC_18),
                       GRect(x - 2, y - 3, width + 4, 14), GTextOverflowModeFill,
                       GTextAlignmentCenter, NULL);
#endif
    return;
  }

  int level = s_battery_level;
  if (level < 0) level = 0;
  if (level > 100) level = 100;
  const int fill_width = (inner_width * level + 50) / 100;
  if (fill_width > 0) {
    graphics_fill_rect(ctx, GRect(x + border, y + border, fill_width, inner_height),
                       inner_radius, GCornersLeft);
  }
}

#else

void battery_initialize(Layer *face_layer) { (void)face_layer; }
void battery_deinitialize(void) {}
void battery_draw(GContext *ctx, GPoint center) { (void)ctx; (void)center; }
void battery_set_mode(int mode) { (void)mode; }
bool battery_is_visible(void) { return false; }
int battery_get_width(void) { return 0; }

#endif
