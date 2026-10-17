#include <pebble.h>

#include "layout.h"

#define LAYOUT_POINT_COUNT 5
#define LAYOUT_START_ANGLE (TRIG_MAX_ANGLE / 4)
#define LAYOUT_MAX_ELEMENTS 5
#define LAYOUT_RADIUS_DIVISOR 4

static const LayoutPoint s_layout_slots[LAYOUT_MAX_ELEMENTS][LAYOUT_MAX_ELEMENTS] = {
  { LAYOUT_POINT_1 },
  { LAYOUT_POINT_3, LAYOUT_POINT_4 },
  { LAYOUT_POINT_3, LAYOUT_POINT_4, LAYOUT_POINT_1 },
  // Four indicators occupy the four highest points; the bottom stays empty.
  { LAYOUT_POINT_3, LAYOUT_POINT_4, LAYOUT_POINT_2, LAYOUT_POINT_5 },
  { LAYOUT_POINT_1, LAYOUT_POINT_2, LAYOUT_POINT_3, LAYOUT_POINT_4, LAYOUT_POINT_5 },
};

GPoint layout_get_point(Layer *layer, int element_count, int element_index) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
  if (element_count < 1) element_count = 1;
  if (element_count > LAYOUT_MAX_ELEMENTS) element_count = LAYOUT_MAX_ELEMENTS;
  if (element_index < 0) element_index = 0;
  if (element_index >= element_count) element_index = element_count - 1;

  LayoutPoint point = s_layout_slots[element_count - 1][element_index];
  int point_index = (int)point - 1;
  int smaller_side = bounds.size.w < bounds.size.h ? bounds.size.w : bounds.size.h;
  // Keep the indicator pentagon inside the white clock face, away from
  // Pac-Man's black outer band.
  int radius = smaller_side / LAYOUT_RADIUS_DIVISOR;

  // 90 degrees is below the center; each point advances by one fifth of a turn.
  int32_t angle = LAYOUT_START_ANGLE +
                  (TRIG_MAX_ANGLE * point_index / LAYOUT_POINT_COUNT);
  GPoint result = GPoint(center.x + cos_lookup(angle) * radius / TRIG_MAX_RATIO,
                         center.y + sin_lookup(angle) * radius / TRIG_MAX_RATIO);
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_BASALT) || \
    defined(PBL_PLATFORM_FLINT) || defined(PBL_PLATFORM_APLITE) || \
    defined(PBL_PLATFORM_DIORITE)
  if (point == LAYOUT_POINT_3 || point == LAYOUT_POINT_4) {
    result.y -= 10;
  }
#endif

#if defined(PBL_PLATFORM_GABBRO) || defined(PBL_PLATFORM_CHALK) || \
    defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_BASALT) || \
    defined(PBL_PLATFORM_FLINT) || defined(PBL_PLATFORM_DIORITE)
  // Inset the two side slots when all four indicators are shown so the weather
  // and five-digit step count stay inside the white dial.
  if (element_count == 4) {
#if defined(PBL_PLATFORM_BASALT) || defined(PBL_PLATFORM_FLINT) || \
    defined(PBL_PLATFORM_DIORITE)
    // Two compact stacked groups share the 80px-wide white playfield.
    if (point == LAYOUT_POINT_2) result.x += 13;
    if (point == LAYOUT_POINT_5) result.x -= 13;
#else
#if defined(PBL_PLATFORM_CHALK)
    const int inset = 16;
#else
    const int inset = 20;
#endif
    if (point == LAYOUT_POINT_2) result.x += inset;
    if (point == LAYOUT_POINT_5) result.x -= inset;
#endif
  }
#endif

  // Keep each element centered on its assigned layout point. The individual
  // draw functions account for their own dimensions around this center.
  return result;
}
