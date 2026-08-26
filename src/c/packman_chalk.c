#include <pebble.h>

#include "packman_internal.h"

#if defined(PBL_PLATFORM_CHALK)
static const PackmanRoundLayout s_chalk_layout = {
  .border_inset = 2,
  .inner_border_inset = 12,
  .clock_radius_divisor = 3,
  .hour_text_inset = 5,
};

void packman_draw_chalk(Layer *layer, GContext *ctx, const ClockTime *clock_time,
                        int pacman_time_minutes, bool mouth_open) {
  packman_draw_round_layout(layer, ctx, clock_time, pacman_time_minutes, mouth_open,
                            &s_chalk_layout);
}
#endif
