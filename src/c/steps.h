#pragma once

#include <pebble.h>

void steps_initialize(Layer *face_layer);
void steps_deinitialize(void);
void steps_refresh(void);
void steps_set_enabled(bool enabled);
bool steps_is_visible(void);
void steps_draw(GContext *ctx, GPoint center, bool stacked);

#ifdef PBL_DEBUG
// Non-persistent screenshot value; a negative count restores Pebble Health.
void steps_set_preview_count(int count);
#endif
