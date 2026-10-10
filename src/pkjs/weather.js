var UPDATE_INTERVAL_MS = 15 * 60 * 1000;
var updateTimer = null;

function weatherIconFromWmo(code) {
  if (code === 0) return 1;
  if (code >= 1 && code <= 3) return 2;
  if (code === 45 || code === 48) return 6;
  if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) return 3;
  if ((code >= 71 && code <= 77) || (code >= 85 && code <= 86)) return 4;
  if (code >= 95 && code <= 99) return 5;
  return 0;
}

function sendWeather(temperature, icon) {
  Pebble.sendAppMessage({
    WeatherTemperature: Math.round(temperature),
    WeatherForecastIcon: icon,
    WeatherUpdatedAt: Math.floor(Date.now() / 1000)
  });
}

function forecastIconAtThreeHours(hourly) {
  var target = Math.floor(Date.now() / 1000) + 3 * 60 * 60;
  for (var i = 0; i < hourly.time.length; i++) {
    if (hourly.time[i] >= target) return weatherIconFromWmo(hourly.weather_code[i]);
  }
  return 0;
}

function fetchWeather(position) {
  var url = 'https://api.open-meteo.com/v1/forecast?latitude=' +
      position.coords.latitude + '&longitude=' + position.coords.longitude +
      '&current=temperature_2m&hourly=weather_code&forecast_hours=5&timeformat=unixtime';
  var request = new XMLHttpRequest();
  request.onload = function() {
    if (request.status !== 200) return;
    try {
      var response = JSON.parse(request.responseText);
      sendWeather(response.current.temperature_2m,
                  forecastIconAtThreeHours(response.hourly));
    } catch (error) {
      // Keep the last valid value on the watch.
    }
  };
  request.open('GET', url);
  request.send();
}

function updateWeather() {
  navigator.geolocation.getCurrentPosition(fetchWeather, function() {}, {
    enableHighAccuracy: false,
    maximumAge: UPDATE_INTERVAL_MS,
    timeout: 10000
  });
}

exports.start = function() {
  if (updateTimer !== null) return;
  updateWeather();
  updateTimer = setInterval(updateWeather, UPDATE_INTERVAL_MS);
};

exports.stop = function() {
  if (updateTimer !== null) {
    clearInterval(updateTimer);
    updateTimer = null;
  }
};
