#pragma once

#include <pebble.h>

// Five equally spaced indicator points around the Gabbro layout.
// Numbering starts at the bottom and continues clockwise.
typedef enum {
  LAYOUT_POINT_1 = 1, // bottom
  LAYOUT_POINT_2 = 2,
  LAYOUT_POINT_3 = 3,
  LAYOUT_POINT_4 = 4,
  LAYOUT_POINT_5 = 5,
} LayoutPoint;

// Returns the slot for an element in a layout containing element_count items.
// element_index is the item's zero-based index in the active-element list.
GPoint layout_get_point(Layer *layer, int element_count, int element_index);
