#pragma once

#include <pebble.h>

typedef struct {
  uint32_t connected_resource_id;
  uint32_t disconnected_resource_id;
  GSize icon_size;
} BluetoothConfiguration;

// Initializes the Bluetooth indicator and subscribes to connection changes.
// The layer is redrawn whenever its state or visibility changes.
void bluetooth_initialize(Layer *face_layer, const BluetoothConfiguration *configuration);
void bluetooth_deinitialize(void);

// Draws the indicator at an offset from the center of `layer`.  Each watch
// model can choose the offset that fits its own layout.
void bluetooth_draw(Layer *layer, GContext *ctx, GPoint center_offset);
