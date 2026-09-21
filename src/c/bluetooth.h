#pragma once

#include <pebble.h>

// Initializes the Bluetooth indicator and subscribes to connection changes.
// The layer is redrawn whenever its state or visibility changes.
void bluetooth_initialize(Layer *face_layer);
void bluetooth_deinitialize(void);

// Draws the indicator at an offset from the center of `layer`.  Each watch
// model can choose the offset that fits its own layout.
void bluetooth_draw(Layer *layer, GContext *ctx, GPoint center_offset);
