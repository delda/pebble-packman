#include <pebble.h>

#include "packman.h"

#define QUARTER_TURN (TRIG_MAX_ANGLE / 4)
#define PACMAN_RADIUS 8
#define DECORATION_POSITIONS 12
#define POSITIONS_PER_QUARTER (DECORATION_POSITIONS / 4)
#define QUARTER_DURATION_MINUTES (MINUTES_PER_DAY / 4)
#define DOT_INTERVAL_MINUTES (MINUTES_PER_DAY / MINUTES_PER_HOUR)
#define GHOST_POSITIONS_PER_QUARTER (QUARTER_DURATION_MINUTES / DOT_INTERVAL_MINUTES - 1)

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

static uint32_t next_random(uint32_t *state) {
  *state = *state * 1664525u + 1013904223u;
  return *state;
}

static bool s_decoration_positions_initialized;
static int s_ghost_position_times[4];
static int s_cherries_time;

// Each ghost stays in a separate quarter of the dial and can use every inner
// dot position, leaving only the dot at the quarter boundary free.
static int ghost_time(int current_time_minutes, int ghost) {
  uint32_t random_state = (uint32_t)current_time_minutes + 0x9e3779b9u + ghost;
  int position_in_quarter = next_random(&random_state) % GHOST_POSITIONS_PER_QUARTER;
  return ghost * QUARTER_DURATION_MINUTES + (position_in_quarter + 1) * DOT_INTERVAL_MINUTES;
}

static int cherry_time(int current_time_minutes) {
  uint32_t random_state = (uint32_t)current_time_minutes + 0x85ebca6bu;
  int first_position = next_random(&random_state) % DECORATION_POSITIONS;
  for (int offset = 0; offset < DECORATION_POSITIONS; ++offset) {
    int position = (first_position + offset * 5) % DECORATION_POSITIONS;
    if (position % POSITIONS_PER_QUARTER == 0) {
      continue;
    }
    int time_minutes = position * 2 * MINUTES_PER_HOUR;
    bool occupied_by_ghost = false;
    for (int ghost = 0; ghost < 4; ++ghost) {
      if (time_minutes == ghost_time(current_time_minutes, ghost)) {
        occupied_by_ghost = true;
        break;
      }
    }
    if (!occupied_by_ghost) {
      return time_minutes;
    }
  }

  return -1;
}

// Pick the decoration positions once for this app session. Their visibility
// still changes as Pac-Man reaches them, but their position never does.
static void initialize_decoration_positions(int current_time_minutes) {
  if (s_decoration_positions_initialized) {
    return;
  }

  for (int ghost = 0; ghost < 4; ++ghost) {
    s_ghost_position_times[ghost] = ghost_time(current_time_minutes, ghost);
  }
  s_cherries_time = cherry_time(current_time_minutes);
  s_decoration_positions_initialized = true;
}

static void draw_pacman(GContext *ctx, GPoint center, int radius, int time_minutes,
                        bool mouth_open) {
  GPoint pacman = point_on_24_hour_clock(center, radius, time_minutes);
  int32_t direction = (TRIG_MAX_ANGLE * time_minutes / (HOURS_PER_DAY * MINUTES_PER_HOUR))
                      + TRIG_MAX_ANGLE / 2;
  graphics_context_set_fill_color(ctx, GColorYellow);
  graphics_fill_circle(ctx, pacman, PACMAN_RADIUS);
  if (mouth_open) {
    int32_t mouth = TRIG_MAX_ANGLE / 12;
    int mouth_radius = PACMAN_RADIUS + 2;
    int32_t mouth_direction = direction + QUARTER_TURN;
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_radial(ctx, GRect(pacman.x - mouth_radius, pacman.y - mouth_radius,
                                    mouth_radius * 2, mouth_radius * 2),
                         GOvalScaleModeFitCircle, mouth_radius,
                         mouth_direction - mouth, mouth_direction + mouth);
  }
}

static void draw_ghost(GContext *ctx, GPoint position, GColor color, bool body_visible) {
  if (body_visible) {
    graphics_context_set_fill_color(ctx, color);
    graphics_fill_circle(ctx, GPoint(position.x, position.y - 1), 6);
    graphics_fill_rect(ctx, GRect(position.x - 6, position.y - 1, 13, 6), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(position.x - 6, position.y + 5, 13, 2), 0, GCornerNone);
    for (int point = -4; point <= 5; point += 3) {
      graphics_fill_rect(ctx, GRect(position.x + point - 1, position.y + 7, 2, 1), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(position.x + point, position.y + 8, 1, 1), 0, GCornerNone);
    }
  }
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, GPoint(position.x - 3, position.y - 1), 2);
  graphics_fill_circle(ctx, GPoint(position.x + 3, position.y - 1), 2);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, GPoint(position.x - 3, position.y), 1);
  graphics_fill_circle(ctx, GPoint(position.x + 3, position.y), 1);
}

static void draw_cherries(GContext *ctx, GPoint position) {
  graphics_context_set_stroke_color(ctx, GColorGreen);
  graphics_draw_line(ctx, GPoint(position.x - 3, position.y + 2), GPoint(position.x, position.y - 5));
  graphics_draw_line(ctx, GPoint(position.x + 3, position.y + 2), GPoint(position.x, position.y - 5));
  graphics_context_set_fill_color(ctx, GColorRed);
  graphics_fill_circle(ctx, GPoint(position.x - 3, position.y + 2), 3);
  graphics_fill_circle(ctx, GPoint(position.x + 3, position.y + 2), 3);
}

static void draw_fruit_bonus(GContext *ctx, GPoint position) {
  graphics_context_set_text_color(ctx, GColorYellow);
  graphics_draw_text(ctx, "100", fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(position.x - 18, position.y - 10, 37, 20),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

static void draw_tapered_hand(GContext *ctx, GPoint center, int32_t angle, int length,
                              int tail_length, int base_width, int tip_width, GColor color) {
  int direction_x = cos_lookup(angle) / (TRIG_MAX_RATIO / 100);
  int direction_y = sin_lookup(angle) / (TRIG_MAX_RATIO / 100);
  GPoint points[] = {
    GPoint(center.x - direction_x * tail_length / 100 - direction_y * base_width / 200,
           center.y - direction_y * tail_length / 100 + direction_x * base_width / 200),
    GPoint(center.x - direction_x * tail_length / 100 + direction_y * base_width / 200,
           center.y - direction_y * tail_length / 100 - direction_x * base_width / 200),
    GPoint(center.x + direction_x * length / 100 + direction_y * tip_width / 200,
           center.y + direction_y * length / 100 - direction_x * tip_width / 200),
    GPoint(center.x + direction_x * length / 100 - direction_y * tip_width / 200,
           center.y + direction_y * length / 100 + direction_x * tip_width / 200),
  };
  GPathInfo hand = {
    .num_points = ARRAY_LENGTH(points),
    .points = points,
  };
  GPath *path = gpath_create(&hand);

  graphics_context_set_fill_color(ctx, color);
  gpath_draw_filled(ctx, path);
  gpath_destroy(path);
}

static void draw_clock_hands(GContext *ctx, GPoint center, int radius,
                             const ClockTime *clock_time) {
  int minute_length = radius - 1;
  int hour_length = radius * 2 / 3;
  int minute = clock_time->minute;
  int hour_minutes = (clock_time->hour % 12) * MINUTES_PER_HOUR + minute;
  int32_t hour_angle = TRIG_MAX_ANGLE * hour_minutes / (12 * MINUTES_PER_HOUR)
                       - QUARTER_TURN;
  int32_t minute_angle = TRIG_MAX_ANGLE * minute / MINUTES_PER_HOUR - QUARTER_TURN;

  draw_tapered_hand(ctx, center, hour_angle, hour_length, 3, 14, 4, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, 10);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, 8);
  draw_tapered_hand(ctx, center, minute_angle, minute_length, 3, 16, 8, GColorWhite);
  draw_tapered_hand(ctx, center, minute_angle, minute_length, 2, 12, 3, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, 6);
}

void packman_draw(Layer *layer, GContext *ctx, const ClockTime *clock_time,
                  int pacman_time_minutes, bool mouth_open) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
  int smaller_side = bounds.size.w < bounds.size.h ? bounds.size.w : bounds.size.h;
  int border_radius = smaller_side / 2 - 2;
  int border_width = 10;
  int outline_width = 2;
  int inner_radius = border_radius - border_width - 2;
  int white_circle_radius = smaller_side / 3;
  int dot_orbit_radius = (inner_radius + white_circle_radius) / 2 + 1;
  int border_text_radius = border_radius - border_width / 2 - 1;
  int current_time_minutes = clock_time_in_minutes(clock_time);

  initialize_decoration_positions(current_time_minutes);

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, border_radius);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, border_radius - outline_width);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, inner_radius);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, white_circle_radius);
  draw_clock_hands(ctx, center, white_circle_radius, clock_time);

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
    int dot_radius = dot % 15 == 0 ? PACMAN_RADIUS * 2 / 3 : 1;
    int dot_time_minutes = dot * MINUTES_PER_DAY / MINUTES_PER_HOUR;
    graphics_fill_circle(ctx, point_on_24_hour_clock(center, dot_orbit_radius, dot_time_minutes), dot_radius);
  }

  const GColor ghost_colors[] = { GColorRed, GColorVividCerulean, GColorFolly, GColorOrange };
  for (int ghost = 0; ghost < 4; ++ghost) {
    int ghost_position_time = s_ghost_position_times[ghost];
    bool body_visible = ghost_position_time > pacman_time_minutes;
    draw_ghost(ctx, point_on_24_hour_clock(center, dot_orbit_radius, ghost_position_time),
               ghost_colors[ghost], body_visible);
  }
  int cherries_time = s_cherries_time;
  if (cherries_time > pacman_time_minutes) {
    draw_cherries(ctx, point_on_24_hour_clock(center, dot_orbit_radius, cherries_time));
  } else {
    draw_fruit_bonus(ctx, point_on_24_hour_clock(center, dot_orbit_radius, cherries_time));
  }
  draw_pacman(ctx, center, dot_orbit_radius, pacman_time_minutes, mouth_open);
}
