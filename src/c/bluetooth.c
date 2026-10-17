#include <pebble.h>

#include "battery.h"
#include "bluetooth.h"
#include "weather.h"
#include "steps.h"

#define PERSIST_KEY_SHOW_BLUETOOTH_ICON 1
// Eight int32 settings need 89 bytes; the outgoing weather state needs 9.
// Allocating the platform maximum can exhaust aplite's application heap.
#define APP_MESSAGE_INBOX_SIZE 128
#define APP_MESSAGE_OUTBOX_SIZE 32

static Layer *s_face_layer;
static GBitmap *s_connected_bitmap;
static GBitmap *s_disconnected_bitmap;
static GSize s_icon_size;
static bool s_show_icon;
static bool s_connected;

static void mark_face_dirty(void) {
  if (s_face_layer) {
    layer_mark_dirty(s_face_layer);
  }
}

static void connection_handler(bool connected) {
  s_connected = connected;
  mark_face_dirty();
}

static void inbox_received_handler(DictionaryIterator *iterator, void *context) {
  Tuple *show_bluetooth_icon = dict_find(iterator, MESSAGE_KEY_ShowBluetoothIcon);
  Tuple *battery_mode = dict_find(iterator, MESSAGE_KEY_BatteryMode);
  Tuple *bluetooth_connected = dict_find(iterator, MESSAGE_KEY_BluetoothConnected);
  Tuple *weather_enabled = dict_find(iterator, MESSAGE_KEY_WeatherEnabled);
  Tuple *weather_temperature = dict_find(iterator, MESSAGE_KEY_WeatherTemperature);
  Tuple *weather_forecast_icon = dict_find(iterator, MESSAGE_KEY_WeatherForecastIcon);
  Tuple *weather_updated_at = dict_find(iterator, MESSAGE_KEY_WeatherUpdatedAt);
  Tuple *weather_request = dict_find(iterator, MESSAGE_KEY_WeatherRequest);
  if (show_bluetooth_icon) {
    s_show_icon = show_bluetooth_icon->value->int32 != 0;
    persist_write_bool(PERSIST_KEY_SHOW_BLUETOOTH_ICON, s_show_icon);
    mark_face_dirty();
  }
  if (battery_mode) battery_set_mode(battery_mode->value->int32);
  Tuple *steps_enabled = dict_find(iterator, MESSAGE_KEY_StepsEnabled);
  if (steps_enabled) steps_set_enabled(steps_enabled->value->int32 != 0);
#ifdef PBL_DEBUG
  Tuple *steps_preview_count = dict_find(iterator, MESSAGE_KEY_StepsPreviewCount);
  if (steps_preview_count) steps_set_preview_count(steps_preview_count->value->int32);
#endif
  if (bluetooth_connected) {
    s_connected = bluetooth_connected->value->int32 != 0;
    mark_face_dirty();
  }
  if (weather_enabled || weather_temperature || weather_forecast_icon ||
      weather_updated_at || weather_request) {
    weather_received(weather_enabled, weather_temperature, weather_forecast_icon,
                     weather_updated_at, weather_request);
  }
}

void bluetooth_initialize(Layer *face_layer, const BluetoothConfiguration *configuration) {
  s_face_layer = face_layer;
  s_connected_bitmap = gbitmap_create_with_resource(configuration->connected_resource_id);
  s_disconnected_bitmap =
      gbitmap_create_with_resource(configuration->disconnected_resource_id);
  s_icon_size = configuration->icon_size;
  s_show_icon = persist_exists(PERSIST_KEY_SHOW_BLUETOOTH_ICON)
                    ? persist_read_bool(PERSIST_KEY_SHOW_BLUETOOTH_ICON)
                    : true;
  s_connected = connection_service_peek_pebble_app_connection();

  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = connection_handler,
  });
  app_message_register_inbox_received(inbox_received_handler);
  AppMessageResult result = app_message_open(APP_MESSAGE_INBOX_SIZE, APP_MESSAGE_OUTBOX_SIZE);
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "AppMessage open failed: %d", result);
  }
}

void bluetooth_deinitialize(void) {
  connection_service_unsubscribe();
  gbitmap_destroy(s_connected_bitmap);
  gbitmap_destroy(s_disconnected_bitmap);
  s_connected_bitmap = NULL;
  s_disconnected_bitmap = NULL;
  s_face_layer = NULL;
}

void bluetooth_draw(Layer *layer, GContext *ctx, GPoint center_offset) {
  if (!s_show_icon) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);
  GPoint center = GPoint(bounds.size.w / 2 + center_offset.x,
                         bounds.size.h / 2 + center_offset.y);
  bluetooth_draw_at(ctx, center);
}

void bluetooth_draw_at(GContext *ctx, GPoint center) {
  if (!s_show_icon) {
    return;
  }

  GBitmap *bitmap = s_connected ? s_connected_bitmap : s_disconnected_bitmap;
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(
      ctx, bitmap,
      GRect(center.x - s_icon_size.w / 2,
            center.y - s_icon_size.h / 2,
            s_icon_size.w, s_icon_size.h));
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
}

bool bluetooth_is_visible(void) {
  return s_show_icon;
}

int bluetooth_get_width(void) {
  return s_icon_size.w;
}
