#include <pebble.h>

#include "steps.h"

#if (defined(PBL_PLATFORM_GABBRO) || defined(PBL_PLATFORM_CHALK) || \
     defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_BASALT) || \
     defined(PBL_PLATFORM_FLINT) || defined(PBL_PLATFORM_DIORITE) || \
     defined(PBL_PLATFORM_APLITE))

#define PERSIST_KEY_STEPS_ENABLED 0xF
#define STEPS_TEXT_GAP 2
#if defined(PBL_PLATFORM_CHALK) || defined(PBL_PLATFORM_BASALT) || defined(PBL_PLATFORM_FLINT) || \
    defined(PBL_PLATFORM_DIORITE) || defined(PBL_PLATFORM_APLITE)
#define STEPS_ICON_WIDTH 19
#define STEPS_FONT FONT_KEY_GOTHIC_18_BOLD
#define STEPS_TEXT_WIDTH 60
#define STEPS_TEXT_HEIGHT 24
#define STEPS_TEXT_OFFSET_Y -12
#define STEPS_STROKE_WIDTH 1
#else
#define STEPS_ICON_WIDTH 25
#define STEPS_FONT FONT_KEY_GOTHIC_24_BOLD
#define STEPS_TEXT_WIDTH 76
#define STEPS_TEXT_HEIGHT 28
#define STEPS_TEXT_OFFSET_Y -16
#define STEPS_STROKE_WIDTH 2
#endif

static Layer *s_face_layer;
static bool s_enabled;
static bool s_has_data;
#ifdef PBL_HEALTH
static bool s_subscribed;
#endif
static int32_t s_count;
static GPath *s_shoe;
#ifdef PBL_DEBUG
static int s_preview_count = -1;
#endif

// Adapted from pebble-dev/iconography/icons/25px_Steps.svg (Apache-2.0).
// Keep the ankle, toe and flat sole recognizable on the supported displays.
static const GPathInfo s_shoe_path = {
  .num_points = 13,
  .points = (GPoint[]) {
#if defined(PBL_PLATFORM_CHALK) || defined(PBL_PLATFORM_BASALT) || defined(PBL_PLATFORM_FLINT) || \
    defined(PBL_PLATFORM_DIORITE) || defined(PBL_PLATFORM_APLITE)
    {2, 15}, {2, 9}, {3, 8}, {2, 6}, {2, 4}, {3, 3}, {8, 3},
    {9, 4}, {9, 6}, {8, 8}, {9, 8}, {15, 11}, {17, 15},
#else
    {2, 20}, {2, 12}, {4, 10}, {2, 8}, {2, 5}, {3, 4}, {11, 4},
    {12, 5}, {12, 8}, {10, 10}, {12, 10}, {20, 14}, {23, 20},
#endif
  },
};

static void mark_dirty(void) {
  if (s_face_layer) layer_mark_dirty(s_face_layer);
}

void steps_refresh(void) {
  if (!s_enabled) return;
#ifdef PBL_DEBUG
  if (s_preview_count >= 0) {
    s_has_data = true;
    s_count = s_preview_count;
    mark_dirty();
    return;
  }
#endif
#ifdef PBL_HEALTH
  HealthServiceAccessibilityMask accessible = health_service_metric_accessible(
      HealthMetricStepCount, time_start_of_today(), time(NULL));
  s_has_data = (accessible & HealthServiceAccessibilityMaskAvailable) != 0;
  s_count = s_has_data ? health_service_sum_today(HealthMetricStepCount) : 0;
#else
  // Aplite has no Pebble Health: show unavailable rather than invent a count.
  s_has_data = false;
  s_count = 0;
#endif
  mark_dirty();
}

#ifdef PBL_HEALTH
static void health_handler(HealthEventType event, void *context) {
  (void)context;
  if (event == HealthEventMovementUpdate || event == HealthEventSignificantUpdate) {
    steps_refresh();
  }
}
#endif

void steps_initialize(Layer *face_layer) {
  s_face_layer = face_layer;
  s_enabled = !persist_exists(PERSIST_KEY_STEPS_ENABLED) ||
              persist_read_bool(PERSIST_KEY_STEPS_ENABLED);
  s_shoe = gpath_create(&s_shoe_path);
#ifdef PBL_HEALTH
  s_subscribed = health_service_events_subscribe(health_handler, NULL);
  if (!s_subscribed) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Steps: health event subscription unavailable");
  }
#endif
  steps_refresh();
}

void steps_deinitialize(void) {
#ifdef PBL_HEALTH
  if (s_subscribed) health_service_events_unsubscribe();
  s_subscribed = false;
#endif
  gpath_destroy(s_shoe);
  s_shoe = NULL;
  s_face_layer = NULL;
}

void steps_set_enabled(bool enabled) {
  s_enabled = enabled;
  persist_write_bool(PERSIST_KEY_STEPS_ENABLED, enabled);
  steps_refresh();
  mark_dirty();
}

#ifdef PBL_DEBUG
void steps_set_preview_count(int count) {
  s_preview_count = count;
  steps_refresh();
}
#endif

bool steps_is_visible(void) {
  return s_enabled;
}

void steps_draw(GContext *ctx, GPoint center, bool stacked) {
  if (!s_enabled) return;
  char text[12];
  if (s_has_data) {
    snprintf(text, sizeof(text), "%ld", (long)s_count);
  } else {
    snprintf(text, sizeof(text), "--");
  }
  GFont font = fonts_get_system_font(STEPS_FONT);
  GSize text_size = graphics_text_layout_get_content_size(
      text, font, GRect(0, 0, STEPS_TEXT_WIDTH, STEPS_TEXT_HEIGHT),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
  GPoint origin;
  GRect text_bounds;
  if (stacked) {
    origin = GPoint(center.x - STEPS_ICON_WIDTH / 2,
                    center.y - STEPS_ICON_WIDTH + 1);
    text_bounds = GRect(center.x - text_size.w / 2, center.y,
                       text_size.w, STEPS_TEXT_HEIGHT);
  } else {
    const int width = STEPS_ICON_WIDTH + STEPS_TEXT_GAP + text_size.w;
    origin = GPoint(center.x - width / 2, center.y - STEPS_ICON_WIDTH / 2);
    text_bounds = GRect(origin.x + STEPS_ICON_WIDTH + STEPS_TEXT_GAP,
                       center.y + STEPS_TEXT_OFFSET_Y,
                       text_size.w, STEPS_TEXT_HEIGHT);
  }
#ifdef PBL_COLOR
  graphics_context_set_fill_color(ctx, GColorLightGray);
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
#else
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);
#endif
  graphics_context_set_stroke_width(ctx, STEPS_STROKE_WIDTH);
  if (s_shoe) {
    gpath_move_to(s_shoe, origin);
    gpath_draw_filled(ctx, s_shoe);
    gpath_draw_outline(ctx, s_shoe);
  }
  // Cuff and shoe panels from the original vector.
#if defined(PBL_PLATFORM_CHALK) || defined(PBL_PLATFORM_BASALT) || defined(PBL_PLATFORM_FLINT) || \
    defined(PBL_PLATFORM_DIORITE) || defined(PBL_PLATFORM_APLITE)
  graphics_draw_line(ctx, GPoint(origin.x + 3, origin.y + 8),
                     GPoint(origin.x + 8, origin.y + 8));
  graphics_draw_line(ctx, GPoint(origin.x + 9, origin.y + 8),
                     GPoint(origin.x + 5, origin.y + 15));
  graphics_draw_line(ctx, GPoint(origin.x + 12, origin.y + 10),
                     GPoint(origin.x + 10, origin.y + 15));
#else
  graphics_draw_line(ctx, GPoint(origin.x + 4, origin.y + 10),
                     GPoint(origin.x + 10, origin.y + 10));
  graphics_draw_line(ctx, GPoint(origin.x + 12, origin.y + 10),
                     GPoint(origin.x + 7, origin.y + 20));
  graphics_draw_line(ctx, GPoint(origin.x + 16, origin.y + 13),
                     GPoint(origin.x + 13, origin.y + 20));
#endif
  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, text, font,
                     text_bounds,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

#else

void steps_initialize(Layer *face_layer) { (void)face_layer; }
void steps_deinitialize(void) {}
void steps_refresh(void) {}
void steps_set_enabled(bool enabled) { (void)enabled; }
#ifdef PBL_DEBUG
void steps_set_preview_count(int count) { (void)count; }
#endif
bool steps_is_visible(void) { return false; }
void steps_draw(GContext *ctx, GPoint center, bool stacked) { (void)ctx; (void)center; (void)stacked; }

#endif
