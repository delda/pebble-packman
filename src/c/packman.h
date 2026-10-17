#pragma once

#include <pebble.h>

#include "clock.h"

void packman_draw(Layer *layer, GContext *ctx, const ClockTime *clock_time,
                  int pacman_time_minutes, bool mouth_open);

// Draws the hands once, above all indicators, with a thin white outline.
void packman_draw_hands_on_top(Layer *layer, GContext *ctx, const ClockTime *clock_time);
