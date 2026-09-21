#include <pebble.h>

#include "bluetooth.h"
#include "clock.h"
#include "packman.h"

static Window *s_window;
static Layer *s_face_layer;
static ClockTime s_clock_time;
static AppTimer *s_animation_timer;
static int s_animation_elapsed_ms;
static int s_animation_duration_ms;

#define FULL_TURN_DURATION_MS 3000
#define ANIMATION_FRAME_MS 33
#define MOUTH_ANIMATION_FRAME_MS 120

#if defined(PBL_PLATFORM_GABBRO) || defined(PBL_PLATFORM_CHALK) || defined(PBL_PLATFORM_EMERY)
#define PBL_BLUETOOTH_INDICATOR_SUPPORTED
#if defined(PBL_PLATFORM_EMERY)
#define BLUETOOTH_ICON_CENTER_OFFSET GPoint(-47, -65)
#else
#define BLUETOOTH_ICON_CENTER_OFFSET GPoint(-30, -30)
#endif

#if defined(PBL_PLATFORM_CHALK)
static const BluetoothConfiguration s_bluetooth_configuration = {
  .connected_resource_id = RESOURCE_ID_IMAGE_BLUETOOTH_CONNECTED_SMALL,
  .disconnected_resource_id = RESOURCE_ID_IMAGE_BLUETOOTH_DISCONNECTED_SMALL,
  .icon_size = GSize(10, 16),
};
#else
static const BluetoothConfiguration s_bluetooth_configuration = {
  .connected_resource_id = RESOURCE_ID_IMAGE_BLUETOOTH_CONNECTED,
  .disconnected_resource_id = RESOURCE_ID_IMAGE_BLUETOOTH_DISCONNECTED,
  .icon_size = GSize(14, 23),
};
#endif
#endif

static int animation_time_minutes(void) {
  int current_time_minutes = clock_time_in_minutes(&s_clock_time);
  int animated_time_minutes =
      s_animation_elapsed_ms * MINUTES_PER_DAY / FULL_TURN_DURATION_MS;
  return animated_time_minutes < current_time_minutes ? animated_time_minutes : current_time_minutes;
}

static bool animation_mouth_open(void) {
  if (s_animation_elapsed_ms >= s_animation_duration_ms) {
    return true;
  }
  return (s_animation_elapsed_ms / MOUTH_ANIMATION_FRAME_MS) % 2 == 0;
}

static void face_layer_update(Layer *layer, GContext *ctx) {
  packman_draw(layer, ctx, &s_clock_time, animation_time_minutes(), animation_mouth_open());
#if defined(PBL_BLUETOOTH_INDICATOR_SUPPORTED)
  // Keep the icon within the upper-left area of the inner white clock face.
  bluetooth_draw(layer, ctx, BLUETOOTH_ICON_CENTER_OFFSET);
#endif
}

static void animation_timer_handler(void *context) {
  s_animation_elapsed_ms += ANIMATION_FRAME_MS;
  if (s_animation_elapsed_ms < s_animation_duration_ms) {
    s_animation_timer = app_timer_register(ANIMATION_FRAME_MS, animation_timer_handler, NULL);
  } else {
    s_animation_elapsed_ms = s_animation_duration_ms;
    s_animation_timer = NULL;
  }
  layer_mark_dirty(s_face_layer);
}

static void start_animation(void) {
  s_animation_elapsed_ms = 0;
  // Keep the angular speed fixed: a full revolution takes the same time
  // regardless of the clock time where Pac-Man stops.
  s_animation_duration_ms =
      (clock_time_in_minutes(&s_clock_time) * FULL_TURN_DURATION_MS + MINUTES_PER_DAY - 1)
      / MINUTES_PER_DAY;
  if (s_animation_duration_ms > 0) {
    s_animation_timer = app_timer_register(ANIMATION_FRAME_MS, animation_timer_handler, NULL);
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  clock_time_set(&s_clock_time, tick_time->tm_hour, tick_time->tm_min);
  layer_mark_dirty(s_face_layer);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_face_layer = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_face_layer, face_layer_update);
  layer_add_child(root, s_face_layer);
#if defined(PBL_BLUETOOTH_INDICATOR_SUPPORTED)
  bluetooth_initialize(s_face_layer, &s_bluetooth_configuration);
#endif
  start_animation();
}

static void window_unload(Window *window) {
  if (s_animation_timer) {
    app_timer_cancel(s_animation_timer);
    s_animation_timer = NULL;
  }
  layer_destroy(s_face_layer);
  s_face_layer = NULL;
}

static void init(void) {
  time_t now = time(NULL);
  struct tm *time_info = localtime(&now);
  clock_time_set(&s_clock_time, time_info->tm_hour, time_info->tm_min);

#ifdef PBL_DEBUG
  // Keep the backlight on while debugging; release builds use automatic control.
  light_enable(true);
#endif

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
#if defined(PBL_BLUETOOTH_INDICATOR_SUPPORTED)
  bluetooth_deinitialize();
#endif
#ifdef PBL_DEBUG
  light_enable(false);
#endif
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
