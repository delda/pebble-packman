#include <pebble.h>

#include "packman_internal.h"

#if !defined(PBL_ROUND)
static GRect rect_inset(GRect rect, int inset) {
  return GRect(rect.origin.x + inset, rect.origin.y + inset,
               rect.size.w - inset * 2, rect.size.h - inset * 2);
}

static GPoint point_on_rectangle(GRect rect, int time_minutes) {
  int width = rect.size.w - 1;
  int height = rect.size.h - 1;
  int perimeter = 2 * (width + height);
  int distance = (time_minutes % MINUTES_PER_DAY) * perimeter / MINUTES_PER_DAY;
  int bottom = rect.origin.y + height;
  int right = rect.origin.x + width;
  int half_width = width / 2;

  if (distance <= half_width) return GPoint(rect.origin.x + half_width - distance, bottom);
  distance -= half_width;
  if (distance <= height) return GPoint(rect.origin.x, bottom - distance);
  distance -= height;
  if (distance <= width) return GPoint(rect.origin.x + distance, rect.origin.y);
  distance -= width;
  if (distance <= height) return GPoint(right, rect.origin.y + distance);
  return GPoint(right - (distance - height), bottom);
}

#if defined(PBL_PLATFORM_EMERY)
static int32_t direction_on_rectangle(GRect rect, int time_minutes) {
  int width = rect.size.w - 1;
  int height = rect.size.h - 1;
  int perimeter = 2 * (width + height);
  int distance = (time_minutes % MINUTES_PER_DAY) * perimeter / MINUTES_PER_DAY;
  int half_width = width / 2;

  if (distance <= half_width) return TRIG_MAX_ANGLE * 3 / 4;
  distance -= half_width;
  if (distance <= height) return 0;
  distance -= height;
  if (distance <= width) return TRIG_MAX_ANGLE / 4;
  distance -= width;
  if (distance <= height) return TRIG_MAX_ANGLE / 2;
  return TRIG_MAX_ANGLE * 3 / 4;
}
#endif

void packman_draw_rectangular(Layer *layer, GContext *ctx, const ClockTime *clock_time,
                              int pacman_time_minutes, bool mouth_open) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = GPoint(bounds.origin.x + bounds.size.w / 2,
                         bounds.origin.y + bounds.size.h / 2);
  GRect hour_track = GRect(bounds.origin.x + 6, bounds.origin.y + 6,
                           bounds.size.w - 12, bounds.size.h - 12);
  GRect game_frame = GRect(bounds.origin.x + 12, bounds.origin.y + 13,
                           bounds.size.w - 24, bounds.size.h - 26);
  GRect game_playfield = rect_inset(game_frame, 20);
  GRect game_track = rect_inset(game_frame, 10);
  GRect clock_face = GRect(center.x - bounds.size.w * 23 / 100,
                           center.y - bounds.size.h * 19 / 100,
                           bounds.size.w * 46 / 100, bounds.size.h * 38 / 100);
  int clock_radius = (clock_face.size.w < clock_face.size.h ? clock_face.size.w : clock_face.size.h) / 2 - 4;
  int hands_radius = clock_radius;
  int current_time_minutes = clock_time_in_minutes(clock_time);
  GFont fruit_bonus_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
#if defined(PBL_PLATFORM_EMERY)
  hands_radius = clock_radius * 2;
  fruit_bonus_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
#endif

  packman_initialize_decorations(current_time_minutes);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, game_frame, 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, game_playfield, 0, GCornerNone);
  packman_draw_clock_hands(ctx, center, hands_radius, clock_time);

  graphics_context_set_text_color(ctx, GColorBlack);
  for (int hour = 0; hour < 24; ++hour) {
    char hour_text[3];
    GPoint position = point_on_rectangle(hour_track, hour * MINUTES_PER_HOUR);
    snprintf(hour_text, sizeof(hour_text), "%d", hour);
    graphics_draw_text(ctx, hour_text, fonts_get_system_font(FONT_KEY_GOTHIC_09),
                       GRect(position.x - 6, position.y - 5, 14, 10),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }

  graphics_context_set_fill_color(ctx, GColorBrilliantRose);
  int current_dot = pacman_time_minutes * MINUTES_PER_HOUR / MINUTES_PER_DAY;
  for (int dot = current_dot + 1; dot < MINUTES_PER_HOUR; ++dot) {
    int dot_radius = dot % 15 == 0 ? PACKMAN_RADIUS * 2 / 3 : 1;
    int dot_time_minutes = dot * MINUTES_PER_DAY / MINUTES_PER_HOUR;
    graphics_fill_circle(ctx, point_on_rectangle(game_track, dot_time_minutes), dot_radius);
  }

  const GColor ghost_colors[] = { GColorRed, GColorVividCerulean, GColorFolly, GColorOrange };
  for (int ghost = 0; ghost < 4; ++ghost) {
    int ghost_time = packman_ghost_time(ghost);
    packman_draw_ghost(ctx, point_on_rectangle(game_track, ghost_time), ghost_colors[ghost],
                       ghost_time > pacman_time_minutes);
  }
  int cherries_time = packman_cherries_time();
  if (cherries_time > pacman_time_minutes) {
    packman_draw_cherries(ctx, point_on_rectangle(game_track, cherries_time));
  } else {
    packman_draw_fruit_bonus(ctx, point_on_rectangle(game_track, cherries_time),
                             fruit_bonus_font);
  }
#if defined(PBL_PLATFORM_EMERY)
  packman_draw_pacman_facing(ctx, point_on_rectangle(game_track, pacman_time_minutes),
                             direction_on_rectangle(game_track, pacman_time_minutes), mouth_open);
#else
  packman_draw_pacman(ctx, point_on_rectangle(game_track, pacman_time_minutes),
                      pacman_time_minutes, mouth_open);
#endif
}
#endif
