#pragma once

#include <pebble.h>

typedef enum {
  WEATHER_ICON_UNAVAILABLE = 0,
  WEATHER_ICON_CLEAR,
  WEATHER_ICON_CLOUDY,
  WEATHER_ICON_RAIN,
  WEATHER_ICON_SNOW,
  WEATHER_ICON_THUNDER,
  WEATHER_ICON_FOG,
} WeatherIcon;

typedef struct {
  int temperature;
  WeatherIcon forecast_icon;
  int32_t updated_at;
  bool has_data;
} WeatherData;

void weather_init(void);
void weather_deinit(void);
void weather_received(Tuple *weather_enabled, Tuple *weather_temperature,
                      Tuple *weather_forecast_icon, Tuple *weather_updated_at,
                      Tuple *weather_request);
bool weather_is_enabled(void);
WeatherData weather_get_data(void);
void weather_draw(GContext *ctx, GPoint origin);
int weather_get_width(void);
