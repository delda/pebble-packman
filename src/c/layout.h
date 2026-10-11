#pragma once

#include <pebble.h>

// Five equally spaced indicator points around the watch face.
// Numbering starts at the bottom and continues clockwise.
typedef enum {
  LAYOUT_POINT_1 = 1, // bottom
  LAYOUT_POINT_2 = 2,
  LAYOUT_POINT_3 = 3,
  LAYOUT_POINT_4 = 4,
  LAYOUT_POINT_5 = 5,
} LayoutPoint;

// Returns the center for an element in a layout containing element_count items.
// Each element is centered on its assigned slot; its draw function accounts
// for the element's own dimensions.
GPoint layout_get_point(Layer *layer, int element_count, int element_index);
