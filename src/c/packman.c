#include <pebble.h>

#include "packman.h"
#include "packman_internal.h"

#define QUARTER_TURN (TRIG_MAX_ANGLE / 4)
#define DECORATION_POSITIONS 12
#define POSITIONS_PER_QUARTER (DECORATION_POSITIONS / 4)
#define QUARTER_DURATION_MINUTES (MINUTES_PER_DAY / 4)
#define DOT_INTERVAL_MINUTES (MINUTES_PER_DAY / MINUTES_PER_HOUR)
#define GHOST_POSITIONS_PER_QUARTER (QUARTER_DURATION_MINUTES / DOT_INTERVAL_MINUTES - 1)

static bool s_decorations_initialized;
static int s_ghost_times[4];
static int s_cherries_time;

static uint32_t next_random(uint32_t *state) {
  *state = *state * 1664525u + 1013904223u;
  return *state;
}

static int decoration_ghost_time(int current_time_minutes, int ghost) {
  uint32_t random_state = (uint32_t)current_time_minutes + 0x9e3779b9u + ghost;
  int position = next_random(&random_state) % GHOST_POSITIONS_PER_QUARTER;
  return ghost * QUARTER_DURATION_MINUTES + (position + 1) * DOT_INTERVAL_MINUTES;
}

static int decoration_cherries_time(int current_time_minutes) {
  uint32_t random_state = (uint32_t)current_time_minutes + 0x85ebca6bu;
  int first_position = next_random(&random_state) % DECORATION_POSITIONS;
  for (int offset = 0; offset < DECORATION_POSITIONS; ++offset) {
    int position = (first_position + offset * 5) % DECORATION_POSITIONS;
    if (position % POSITIONS_PER_QUARTER == 0) continue;
    int time_minutes = position * 2 * MINUTES_PER_HOUR;
    bool occupied = false;
    for (int ghost = 0; ghost < 4; ++ghost) {
      if (time_minutes == decoration_ghost_time(current_time_minutes, ghost)) {
        occupied = true;
        break;
      }
    }
    if (!occupied) return time_minutes;
  }
  return -1;
}

void packman_initialize_decorations(int current_time_minutes) {
  if (s_decorations_initialized) return;
  for (int ghost = 0; ghost < 4; ++ghost) {
    s_ghost_times[ghost] = decoration_ghost_time(current_time_minutes, ghost);
  }
  s_cherries_time = decoration_cherries_time(current_time_minutes);
  s_decorations_initialized = true;
}

int packman_ghost_time(int ghost) { return s_ghost_times[ghost]; }
int packman_cherries_time(void) { return s_cherries_time; }

void packman_draw_pacman(GContext *ctx, GPoint position, int time_minutes, bool mouth_open) {
  int32_t direction = (TRIG_MAX_ANGLE * time_minutes / (HOURS_PER_DAY * MINUTES_PER_HOUR))
                      + TRIG_MAX_ANGLE / 2;
  graphics_context_set_fill_color(ctx, GColorYellow);
  graphics_fill_circle(ctx, position, PACKMAN_RADIUS);
  if (!mouth_open) return;
  int32_t mouth = TRIG_MAX_ANGLE / 12;
  int mouth_radius = PACKMAN_RADIUS + 2;
  int32_t mouth_direction = direction + QUARTER_TURN;
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_radial(ctx, GRect(position.x - mouth_radius, position.y - mouth_radius,
                                  mouth_radius * 2, mouth_radius * 2),
                       GOvalScaleModeFitCircle, mouth_radius,
                       mouth_direction - mouth, mouth_direction + mouth);
}

void packman_draw_ghost(GContext *ctx, GPoint position, GColor color, bool body_visible) {
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

void packman_draw_cherries(GContext *ctx, GPoint position) {
  graphics_context_set_stroke_color(ctx, GColorGreen);
  graphics_draw_line(ctx, GPoint(position.x - 3, position.y + 2), GPoint(position.x, position.y - 5));
  graphics_draw_line(ctx, GPoint(position.x + 3, position.y + 2), GPoint(position.x, position.y - 5));
  graphics_context_set_fill_color(ctx, GColorRed);
  graphics_fill_circle(ctx, GPoint(position.x - 3, position.y + 2), 3);
  graphics_fill_circle(ctx, GPoint(position.x + 3, position.y + 2), 3);
}

void packman_draw_fruit_bonus(GContext *ctx, GPoint position) {
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
  GPathInfo hand = { .num_points = ARRAY_LENGTH(points), .points = points };
  GPath *path = gpath_create(&hand);
  graphics_context_set_fill_color(ctx, color);
  gpath_draw_filled(ctx, path);
  gpath_destroy(path);
}

void packman_draw_clock_hands(GContext *ctx, GPoint center, int radius,
                              const ClockTime *clock_time) {
  int minute = clock_time->minute;
  int hour_minutes = (clock_time->hour % 12) * MINUTES_PER_HOUR + minute;
  int32_t hour_angle = TRIG_MAX_ANGLE * hour_minutes / (12 * MINUTES_PER_HOUR) - QUARTER_TURN;
  int32_t minute_angle = TRIG_MAX_ANGLE * minute / MINUTES_PER_HOUR - QUARTER_TURN;
  draw_tapered_hand(ctx, center, hour_angle, radius * 2 / 3, 3, 14, 4, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, 10);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, 8);
  draw_tapered_hand(ctx, center, minute_angle, radius - 1, 3, 16, 8, GColorWhite);
  draw_tapered_hand(ctx, center, minute_angle, radius - 1, 2, 12, 3, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, 6);
}

void packman_draw(Layer *layer, GContext *ctx, const ClockTime *clock_time,
                  int pacman_time_minutes, bool mouth_open) {
#if defined(PBL_PLATFORM_CHALK)
  packman_draw_chalk(layer, ctx, clock_time, pacman_time_minutes, mouth_open);
#elif defined(PBL_PLATFORM_GABBRO)
  packman_draw_gabbro(layer, ctx, clock_time, pacman_time_minutes, mouth_open);
#else
  packman_draw_rectangular(layer, ctx, clock_time, pacman_time_minutes, mouth_open);
#endif
}
