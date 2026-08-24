#include <pebble.h>

#define QUARTER_TURN (TRIG_MAX_ANGLE / 4)
#define PACMAN_RADIUS 8

static Window *s_window;
static Layer *s_face_layer;
static int s_hour;
static int s_minute;

static GPoint point_on_border(GPoint center, int radius, int hour) {
  // Hour zero starts at the bottom, then advances clockwise.
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

static void draw_pacman(GContext *ctx, GPoint center, int radius, int time_minutes) {
  GPoint pacman = point_on_24_hour_clock(center, radius, time_minutes);
  // Face clockwise toward the next dot; at 12 o'clock this is the classic right-facing pose.
  int32_t direction = (TRIG_MAX_ANGLE * time_minutes / (HOURS_PER_DAY * MINUTES_PER_HOUR))
                      + TRIG_MAX_ANGLE / 2;

  graphics_context_set_fill_color(ctx, GColorYellow);
  graphics_fill_circle(ctx, pacman, PACMAN_RADIUS);

  // Erase a small wedge in front of Pac-Man to form the animated mouth.
  int32_t mouth = TRIG_MAX_ANGLE / 12;
  int mouth_radius = PACMAN_RADIUS + 2;
  // Radial drawing uses 12 o'clock as zero, unlike the 24-hour position.
  int32_t mouth_direction = direction + QUARTER_TURN;
  int32_t mouth_start = mouth_direction - mouth;
  int32_t mouth_end = mouth_direction + mouth;
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_radial(ctx, GRect(pacman.x - mouth_radius, pacman.y - mouth_radius,
                                  mouth_radius * 2, mouth_radius * 2),
                       GOvalScaleModeFitCircle, mouth_radius, mouth_start, mouth_end);
}

static void draw_ghost(GContext *ctx, GPoint position, GColor color) {
  graphics_context_set_fill_color(ctx, color);
  graphics_fill_circle(ctx, GPoint(position.x, position.y - 1), 6);
  graphics_fill_rect(ctx, GRect(position.x - 6, position.y - 1, 13, 6), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(position.x - 6, position.y + 5, 13, 2), 0, GCornerNone);
  // Four triangular points at the bottom of the ghost.
  for (int point = -4; point <= 5; point += 3) {
    graphics_fill_rect(ctx, GRect(position.x + point - 1, position.y + 7, 2, 1),
                       0, GCornerNone);
    graphics_fill_rect(ctx, GRect(position.x + point, position.y + 8, 1, 1),
                       0, GCornerNone);
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

static void face_layer_update(Layer *layer, GContext *ctx) {
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

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Draw a thin outline around the white rim, then keep labels within it.
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, border_radius);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, border_radius - outline_width);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, inner_radius);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, white_circle_radius);

  graphics_context_set_text_color(ctx, GColorBlack);
  for (int hour = 0; hour < 24; ++hour) {
    char hour_text[3];
    GPoint position = point_on_border(center, border_text_radius, hour);
    snprintf(hour_text, sizeof(hour_text), "%d", hour);
    graphics_draw_text(ctx, hour_text, fonts_get_system_font(FONT_KEY_GOTHIC_09),
                       GRect(position.x - 6, position.y - 5, 14, 8),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }

  // Dots from midnight through Pac-Man's current 24-hour position have been eaten.
  graphics_context_set_fill_color(ctx, GColorBrilliantRose);
  int current_time_minutes = s_hour * MINUTES_PER_HOUR + s_minute;
  int current_dot = current_time_minutes * MINUTES_PER_HOUR / MINUTES_PER_DAY;
  for (int dot = current_dot + 1; dot < MINUTES_PER_HOUR; ++dot) {
    int dot_radius = 1;
    if (dot % 15 == 0) {
      dot_radius = PACMAN_RADIUS * 2 / 3; // 00:00, 06:00, 12:00 and 18:00
    } else if (dot % 5 == 0) {
      dot_radius = 1; // The other two-hour markers
    }
    int dot_time_minutes = dot * MINUTES_PER_DAY / MINUTES_PER_HOUR;
    graphics_fill_circle(ctx, point_on_24_hour_clock(center, dot_orbit_radius, dot_time_minutes),
                         dot_radius);
  }

  const int ghost_times[] = { 2 * MINUTES_PER_HOUR, 7 * MINUTES_PER_HOUR,
                              15 * MINUTES_PER_HOUR, 20 * MINUTES_PER_HOUR };
  const GColor ghost_colors[] = { GColorRed, GColorVividCerulean,
                                  GColorFolly, GColorOrange };
  for (int ghost = 0; ghost < 4; ++ghost) {
    draw_ghost(ctx, point_on_24_hour_clock(center, dot_orbit_radius, ghost_times[ghost]),
               ghost_colors[ghost]);
  }
  draw_cherries(ctx, point_on_24_hour_clock(center, dot_orbit_radius, 11 * MINUTES_PER_HOUR));

  draw_pacman(ctx, center, dot_orbit_radius, current_time_minutes);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  s_hour = tick_time->tm_hour;
  s_minute = tick_time->tm_min;
  layer_mark_dirty(s_face_layer);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_face_layer = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_face_layer, face_layer_update);
  layer_add_child(root, s_face_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_face_layer);
}

static void init(void) {
  time_t now = time(NULL);
  struct tm *time_info = localtime(&now);
  s_hour = time_info->tm_hour;
  s_minute = time_info->tm_min;

  s_window = window_create();
  window_set_background_color(s_window, GColorWhite);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
