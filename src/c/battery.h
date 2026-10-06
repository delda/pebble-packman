#pragma once

#include <pebble.h>

void battery_initialize(Layer *face_layer);
void battery_deinitialize(void);
void battery_draw(GContext *ctx, GPoint center);
void battery_set_mode(int mode);
