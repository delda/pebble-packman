#pragma once

#include <pebble.h>

#include "clock.h"

void packman_draw(Layer *layer, GContext *ctx, const ClockTime *clock_time,
                  int pacman_time_minutes, bool mouth_open);
