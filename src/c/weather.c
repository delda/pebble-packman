#include <pebble.h>

#include "weather.h"

#if defined(PBL_PLATFORM_GABBRO) || defined(PBL_PLATFORM_CHALK) || \
    defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_BASALT) || \
    defined(PBL_PLATFORM_FLINT) || defined(PBL_PLATFORM_APLITE) || \
    defined(PBL_PLATFORM_DIORITE)

// Small-screen platforms share Basalt's compact weather text and vector icons.
#define WEATHER_ICON_WIDTH 19
#define WEATHER_TEXT_GAP 2
#if defined(PBL_PLATFORM_GABBRO)
#define WEATHER_FONT FONT_KEY_GOTHIC_24_BOLD
#define WEATHER_TEXT_WIDTH 50
#define WEATHER_TEXT_HEIGHT 28
#define WEATHER_TEXT_OFFSET_Y -5
#else
#define WEATHER_FONT FONT_KEY_GOTHIC_18_BOLD
#define WEATHER_TEXT_WIDTH 38
#define WEATHER_TEXT_HEIGHT 24
#define WEATHER_TEXT_OFFSET_Y -3
#endif
#define WEATHER_WIDTH (WEATHER_ICON_WIDTH + WEATHER_TEXT_GAP + WEATHER_TEXT_WIDTH)
#define WEATHER_HEIGHT 24

enum WeatherPersistKey {
  WEATHER_ENABLED_PERSIST_KEY = 0xB,
  WEATHER_TEMPERATURE_PERSIST_KEY = 0xC,
  WEATHER_ICON_PERSIST_KEY = 0xD,
  WEATHER_UPDATED_AT_PERSIST_KEY = 0xE,
};

static bool s_enabled;
static WeatherData s_data;

static void send_enabled_state(void) {
  DictionaryIterator *iterator;
  if (app_message_outbox_begin(&iterator) != APP_MSG_OK) return;
  dict_write_uint8(iterator, MESSAGE_KEY_WeatherEnabled, s_enabled);
  dict_write_end(iterator);
  app_message_outbox_send();
}

void weather_init(void) {
  s_enabled = persist_exists(WEATHER_ENABLED_PERSIST_KEY) &&
              persist_read_bool(WEATHER_ENABLED_PERSIST_KEY);
  s_data.temperature = 0;
  s_data.forecast_icon = WEATHER_ICON_UNAVAILABLE;
  s_data.updated_at = 0;
  s_data.has_data = false;
  if (persist_exists(WEATHER_TEMPERATURE_PERSIST_KEY) &&
      persist_exists(WEATHER_ICON_PERSIST_KEY) &&
      persist_exists(WEATHER_UPDATED_AT_PERSIST_KEY)) {
    s_data.temperature = persist_read_int(WEATHER_TEMPERATURE_PERSIST_KEY);
    s_data.forecast_icon = (WeatherIcon)persist_read_int(WEATHER_ICON_PERSIST_KEY);
    s_data.updated_at = persist_read_int(WEATHER_UPDATED_AT_PERSIST_KEY);
    s_data.has_data = s_data.forecast_icon > WEATHER_ICON_UNAVAILABLE &&
                      s_data.forecast_icon <= WEATHER_ICON_FOG;
  }
}

void weather_deinit(void) {}

void weather_received(Tuple *weather_enabled, Tuple *weather_temperature,
                      Tuple *weather_forecast_icon, Tuple *weather_updated_at,
                      Tuple *weather_request) {
  if (weather_enabled) {
    s_enabled = weather_enabled->value->int32 != 0;
    persist_write_bool(WEATHER_ENABLED_PERSIST_KEY, s_enabled);
  }
  if (weather_temperature) {
    int temperature = weather_temperature->value->int32;
    if (temperature >= -99 && temperature <= 99) {
      s_data.temperature = temperature;
      persist_write_int(WEATHER_TEMPERATURE_PERSIST_KEY, temperature);
    }
  }
  if (weather_forecast_icon) {
    int icon = weather_forecast_icon->value->int32;
    if (icon < WEATHER_ICON_UNAVAILABLE || icon > WEATHER_ICON_FOG) {
      icon = WEATHER_ICON_UNAVAILABLE;
    }
    s_data.forecast_icon = (WeatherIcon)icon;
    persist_write_int(WEATHER_ICON_PERSIST_KEY, icon);
  }
  if (weather_updated_at && weather_updated_at->value->int32 > 0) {
    s_data.updated_at = weather_updated_at->value->int32;
    s_data.has_data = s_data.forecast_icon != WEATHER_ICON_UNAVAILABLE;
    persist_write_int(WEATHER_UPDATED_AT_PERSIST_KEY, s_data.updated_at);
  }
  if (weather_request) send_enabled_state();
}

bool weather_is_enabled(void) {
  return s_enabled;
}

WeatherData weather_get_data(void) {
  return s_data;
}

static void draw_cloud(GContext *ctx, GPoint origin, GColor color) {
#ifndef PBL_COLOR
  color = GColorBlack;
#endif
  graphics_context_set_fill_color(ctx, color);
  graphics_fill_circle(ctx, GPoint(origin.x + 6, origin.y + 10), 4);
  graphics_fill_circle(ctx, GPoint(origin.x + 11, origin.y + 8), 6);
  graphics_fill_rect(ctx, GRect(origin.x + 3, origin.y + 10, 14, 6),
                     0, GCornerNone);
}

static void draw_drop(GContext *ctx, GPoint origin, int x, int y,
                      GColor color) {
  GPoint points[] = {{origin.x + x, origin.y + y},
                     {origin.x + x - 2, origin.y + y + 4},
                     {origin.x + x, origin.y + y + 6},
                     {origin.x + x + 2, origin.y + y + 4}};
  GPathInfo path_info = {.num_points = ARRAY_LENGTH(points), .points = points};
  GPath *path = gpath_create(&path_info);
  graphics_context_set_fill_color(ctx, color);
  if (path != NULL) {
    gpath_draw_filled(ctx, path);
    gpath_destroy(path);
  }
}

static void draw_sun(GContext *ctx, GPoint origin, GColor color) {
  graphics_context_set_fill_color(ctx, color);
  graphics_fill_circle(ctx, GPoint(origin.x + 9, origin.y + 9), 4);
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_line(ctx, GPoint(origin.x + 9, origin.y),
                     GPoint(origin.x + 9, origin.y + 3));
  graphics_draw_line(ctx, GPoint(origin.x + 9, origin.y + 15),
                     GPoint(origin.x + 9, origin.y + 18));
  graphics_draw_line(ctx, GPoint(origin.x, origin.y + 9),
                     GPoint(origin.x + 3, origin.y + 9));
  graphics_draw_line(ctx, GPoint(origin.x + 15, origin.y + 9),
                     GPoint(origin.x + 18, origin.y + 9));
  graphics_draw_line(ctx, GPoint(origin.x + 2, origin.y + 2),
                     GPoint(origin.x + 4, origin.y + 4));
  graphics_draw_line(ctx, GPoint(origin.x + 14, origin.y + 14),
                     GPoint(origin.x + 16, origin.y + 16));
  graphics_draw_line(ctx, GPoint(origin.x + 14, origin.y + 4),
                     GPoint(origin.x + 16, origin.y + 2));
  graphics_draw_line(ctx, GPoint(origin.x + 2, origin.y + 16),
                     GPoint(origin.x + 4, origin.y + 14));
}

static GColor icon_color(WeatherIcon icon) {
#ifdef PBL_COLOR
  switch (icon) {
    case WEATHER_ICON_CLEAR:
      return GColorChromeYellow;
    case WEATHER_ICON_THUNDER:
      return GColorRed;
    case WEATHER_ICON_CLOUDY:
    case WEATHER_ICON_FOG:
      return GColorLightGray;
    case WEATHER_ICON_RAIN:
      return GColorPictonBlue;
    case WEATHER_ICON_SNOW:
      return GColorCyan;
    case WEATHER_ICON_UNAVAILABLE:
    default:
      return GColorDarkGray;
  }
#else
  (void)icon;
  return GColorBlack;
#endif
}

static void draw_icon(GContext *ctx, WeatherIcon icon, GPoint origin) {
  GColor color = icon_color(icon);
  if (icon == WEATHER_ICON_CLEAR) {
    draw_sun(ctx, origin, color);
    return;
  }
  if (icon == WEATHER_ICON_UNAVAILABLE) {
    graphics_draw_circle(ctx, GPoint(origin.x + 9, origin.y + 9), 8);
    graphics_draw_text(ctx, "?", fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                       GRect(origin.x + 3, origin.y - 3, 13, 18),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    return;
  }
  if (icon == WEATHER_ICON_FOG) {
    graphics_context_set_stroke_color(ctx, color);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_line(ctx, GPoint(origin.x + 2, origin.y + 5), GPoint(origin.x + 16, origin.y + 5));
    graphics_draw_line(ctx, GPoint(origin.x, origin.y + 10), GPoint(origin.x + 18, origin.y + 10));
    graphics_draw_line(ctx, GPoint(origin.x + 3, origin.y + 15), GPoint(origin.x + 17, origin.y + 15));
    return;
  }
  if (icon == WEATHER_ICON_SNOW) {
    graphics_context_set_stroke_color(ctx, color);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, GPoint(origin.x + 9, origin.y + 5),
                       GPoint(origin.x + 9, origin.y + 13));
    graphics_draw_line(ctx, GPoint(origin.x + 9, origin.y + 5),
                       GPoint(origin.x + 7, origin.y));
    graphics_draw_line(ctx, GPoint(origin.x + 9, origin.y + 5),
                       GPoint(origin.x + 11, origin.y));
    graphics_draw_line(ctx, GPoint(origin.x + 9, origin.y + 13),
                       GPoint(origin.x + 7, origin.y + 18));
    graphics_draw_line(ctx, GPoint(origin.x + 9, origin.y + 13),
                       GPoint(origin.x + 11, origin.y + 18));
    graphics_draw_line(ctx, GPoint(origin.x + 5, origin.y + 7),
                       GPoint(origin.x + 13, origin.y + 11));
    graphics_draw_line(ctx, GPoint(origin.x + 5, origin.y + 7),
                       GPoint(origin.x + 3, origin.y + 3));
    graphics_draw_line(ctx, GPoint(origin.x + 5, origin.y + 7),
                       GPoint(origin.x + 1, origin.y + 8));
    graphics_draw_line(ctx, GPoint(origin.x + 13, origin.y + 11),
                       GPoint(origin.x + 15, origin.y + 15));
    graphics_draw_line(ctx, GPoint(origin.x + 13, origin.y + 11),
                       GPoint(origin.x + 17, origin.y + 10));
    graphics_draw_line(ctx, GPoint(origin.x + 13, origin.y + 7),
                       GPoint(origin.x + 5, origin.y + 11));
    graphics_draw_line(ctx, GPoint(origin.x + 13, origin.y + 7),
                       GPoint(origin.x + 15, origin.y + 3));
    graphics_draw_line(ctx, GPoint(origin.x + 13, origin.y + 7),
                       GPoint(origin.x + 17, origin.y + 8));
    graphics_draw_line(ctx, GPoint(origin.x + 5, origin.y + 11),
                       GPoint(origin.x + 3, origin.y + 15));
    graphics_draw_line(ctx, GPoint(origin.x + 5, origin.y + 11),
                       GPoint(origin.x + 1, origin.y + 10));
    return;
  }
  if (icon == WEATHER_ICON_THUNDER) {
    // Thunder is represented by the original red lightning shape alone.
    GPoint points[] = {{origin.x + 12, origin.y}, {origin.x + 3, origin.y + 11},
                       {origin.x + 8, origin.y + 11}, {origin.x + 5, origin.y + 21},
                       {origin.x + 17, origin.y + 9}, {origin.x + 12, origin.y + 9}};
    GPathInfo path_info = {.num_points = ARRAY_LENGTH(points), .points = points};
    GPath *path = gpath_create(&path_info);
    if (path != NULL) {
      graphics_context_set_fill_color(ctx, color);
      gpath_draw_filled(ctx, path);
      gpath_destroy(path);
    }
    return;
  }
  if (icon == WEATHER_ICON_RAIN) {
    // Rain is represented by a dense field of independent vector drops.
    draw_drop(ctx, origin, 2, 1, color);
    draw_drop(ctx, origin, 9, 1, color);
    draw_drop(ctx, origin, 16, 1, color);
    draw_drop(ctx, origin, 5, 8, color);
    draw_drop(ctx, origin, 12, 8, color);
    draw_drop(ctx, origin, 2, 15, color);
    draw_drop(ctx, origin, 9, 15, color);
    draw_drop(ctx, origin, 16, 15, color);
    return;
  }
  if (icon == WEATHER_ICON_CLOUDY) {
    draw_sun(ctx, origin, icon_color(WEATHER_ICON_CLEAR));
    draw_cloud(ctx, origin, GColorLightGray);
    return;
  }
  draw_cloud(ctx, origin, GColorLightGray);
}

void weather_draw(GContext *ctx, GPoint origin) {
  if (!weather_is_enabled()) return;
  WeatherData data = weather_get_data();
  char temperature[8];
  snprintf(temperature, sizeof(temperature), data.has_data ? "%d\xC2\xB0" : "--\xC2\xB0",
           data.has_data ? data.temperature : 0);

  GFont font = fonts_get_system_font(WEATHER_FONT);
  GSize temperature_size = graphics_text_layout_get_content_size(
      temperature, font, GRect(0, 0, WEATHER_TEXT_WIDTH, WEATHER_TEXT_HEIGHT),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
  int content_width = WEATHER_ICON_WIDTH + WEATHER_TEXT_GAP + temperature_size.w;
  origin.x -= content_width / 2;
  origin.y -= WEATHER_HEIGHT / 2;

  draw_icon(ctx, data.has_data ? data.forecast_icon : WEATHER_ICON_UNAVAILABLE, origin);
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, temperature, font,
                     GRect(origin.x + WEATHER_ICON_WIDTH + WEATHER_TEXT_GAP,
                           origin.y + WEATHER_TEXT_OFFSET_Y, temperature_size.w,
                           WEATHER_TEXT_HEIGHT),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

int weather_get_width(void) {
  return WEATHER_WIDTH;
}

#else

void weather_init(void) {}
void weather_deinit(void) {}
void weather_received(Tuple *a, Tuple *b, Tuple *c, Tuple *d, Tuple *e) {
  (void)a; (void)b; (void)c; (void)d; (void)e;
}
bool weather_is_enabled(void) { return false; }
WeatherData weather_get_data(void) { return (WeatherData){0}; }
void weather_draw(GContext *ctx, GPoint origin) { (void)ctx; (void)origin; }
int weather_get_width(void) { return 0; }

#endif
