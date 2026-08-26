#include <pebble.h>

#include "packman_internal.h"

#if defined(PBL_ROUND)
#define QUARTER_TURN (TRIG_MAX_ANGLE / 4)

static GPoint point_on_border(GPoint center, int radius, int hour) {
  int32_t angle = (TRIG_MAX_ANGLE * hour / 24) + QUARTER_TURN;
  return GPoint(center.x + (int)(cos_lookup(angle) * radius / TRIG_MAX_RATIO),
                center.y + (int)(sin_lookup(angle) * radius / TRIG_MAX_RATIO));
}

static GPoint point_on_24_hour_clock(GPoint center, int radius, int time_minutes) {
  int32_t angle = (TRIG_MAX_ANGLE * time_minutes / (HOURS_PER_DAY * MINUTES_PER_HOUR))
                  + QUARTER_TURN;
  return GPoint(center.x + (int)(cos_lookup(angle) * radius / TRIG_MAX_RATIO),
                center.y + (int)(sin_lookup(angle) * radius / TRIG_MAX_RATIO));
}

void packman_draw_round_layout(Layer *layer, GContext *ctx, const ClockTime *clock_time,
                               int pacman_time_minutes, bool mouth_open,
                               const PackmanRoundLayout *layout) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
  int smaller_side = bounds.size.w < bounds.size.h ? bounds.size.w : bounds.size.h;
  int border_radius = smaller_side / 2 - layout->border_inset;
  int inner_radius = border_radius - layout->inner_border_inset;
  int white_circle_radius = smaller_side / layout->clock_radius_divisor;
  int dot_orbit_radius = (inner_radius + white_circle_radius) / 2 + 1;
  int border_text_radius = border_radius - layout->hour_text_inset;
  int current_time_minutes = clock_time_in_minutes(clock_time);

  packman_initialize_decorations(current_time_minutes);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, inner_radius);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, white_circle_radius);
  packman_draw_clock_hands(ctx, center, white_circle_radius, clock_time);

  graphics_context_set_text_color(ctx, GColorBlack);
  for (int hour = 0; hour < 24; ++hour) {
    char hour_text[3];
    GPoint position = point_on_border(center, border_text_radius, hour);
    snprintf(hour_text, sizeof(hour_text), "%d", hour);
    graphics_draw_text(ctx, hour_text, fonts_get_system_font(FONT_KEY_GOTHIC_09),
                       GRect(position.x - 6, position.y - 5, 14, 8),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }

  graphics_context_set_fill_color(ctx, GColorBrilliantRose);
  int current_dot = pacman_time_minutes * MINUTES_PER_HOUR / MINUTES_PER_DAY;
  for (int dot = current_dot + 1; dot < MINUTES_PER_HOUR; ++dot) {
    int dot_radius = dot % 15 == 0 ? PACKMAN_RADIUS * 2 / 3 : 1;
    int dot_time_minutes = dot * MINUTES_PER_DAY / MINUTES_PER_HOUR;
    graphics_fill_circle(ctx, point_on_24_hour_clock(center, dot_orbit_radius, dot_time_minutes), dot_radius);
  }

  const GColor ghost_colors[] = { GColorRed, GColorVividCerulean, GColorFolly, GColorOrange };
  for (int ghost = 0; ghost < 4; ++ghost) {
    int ghost_time = packman_ghost_time(ghost);
    packman_draw_ghost(ctx, point_on_24_hour_clock(center, dot_orbit_radius, ghost_time),
                       ghost_colors[ghost], ghost_time > pacman_time_minutes);
  }
  int cherries_time = packman_cherries_time();
  if (cherries_time > pacman_time_minutes) {
    packman_draw_cherries(ctx, point_on_24_hour_clock(center, dot_orbit_radius, cherries_time));
  } else {
    packman_draw_fruit_bonus(ctx, point_on_24_hour_clock(center, dot_orbit_radius, cherries_time),
                             fonts_get_system_font(layout->fruit_bonus_font_key));
  }
  packman_draw_pacman(ctx, point_on_24_hour_clock(center, dot_orbit_radius, pacman_time_minutes),
                      pacman_time_minutes, mouth_open);
}
#endif
