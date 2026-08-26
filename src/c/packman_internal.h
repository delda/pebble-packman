#pragma once

#include <pebble.h>

#include "clock.h"

#define PACKMAN_RADIUS 8

void packman_initialize_decorations(int current_time_minutes);
int packman_ghost_time(int ghost);
int packman_cherries_time(void);

void packman_draw_pacman(GContext *ctx, GPoint position, int time_minutes, bool mouth_open);
void packman_draw_ghost(GContext *ctx, GPoint position, GColor color, bool body_visible);
void packman_draw_cherries(GContext *ctx, GPoint position);
void packman_draw_fruit_bonus(GContext *ctx, GPoint position);
void packman_draw_clock_hands(GContext *ctx, GPoint center, int radius,
                              const ClockTime *clock_time);

typedef struct {
  int border_inset;
  int inner_border_inset;
  int clock_radius_divisor;
  int hour_text_inset;
} PackmanRoundLayout;

void packman_draw_round_layout(Layer *layer, GContext *ctx, const ClockTime *clock_time,
                               int pacman_time_minutes, bool mouth_open,
                               const PackmanRoundLayout *layout);
void packman_draw_chalk(Layer *layer, GContext *ctx, const ClockTime *clock_time,
                        int pacman_time_minutes, bool mouth_open);
void packman_draw_gabbro(Layer *layer, GContext *ctx, const ClockTime *clock_time,
                         int pacman_time_minutes, bool mouth_open);
void packman_draw_rectangular(Layer *layer, GContext *ctx, const ClockTime *clock_time,
                              int pacman_time_minutes, bool mouth_open);
