#include <pebble.h>

#include "bluetooth.h"

#define PERSIST_KEY_SHOW_BLUETOOTH_ICON 1

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
  if (!show_bluetooth_icon) {
    return;
  }

  s_show_icon = show_bluetooth_icon->value->int32 != 0;
  persist_write_bool(PERSIST_KEY_SHOW_BLUETOOTH_ICON, s_show_icon);
  mark_face_dirty();
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
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
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
  GBitmap *bitmap = s_connected ? s_connected_bitmap : s_disconnected_bitmap;
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(
      ctx, bitmap,
      GRect(center.x - s_icon_size.w / 2,
            center.y - s_icon_size.h / 2,
            s_icon_size.w, s_icon_size.h));
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
}
