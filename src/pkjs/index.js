var config = require('./config.json');
var weather = require('./weather');

function configuredValue() {
  var storedValue = localStorage.getItem('showBluetoothIcon');
  return storedValue === null ? config.showBluetoothIcon.default : storedValue === 'true';
}

function configuredBatteryMode() {
  var storedValue = localStorage.getItem('batteryMode');
  return storedValue === null ? config.batteryMode.default : storedValue;
}

function configuredWeather() {
  var storedValue = localStorage.getItem('showWeather');
  return storedValue === null ? config.showWeather.default : storedValue === 'true';
}

function supportsSteps() {
  if (typeof Pebble.getActiveWatchInfo !== 'function') return false;
  var platform = Pebble.getActiveWatchInfo().platform;
  return platform === 'gabbro' || platform === 'chalk' ||
      platform === 'emery' || platform === 'basalt';
}

function configuredSteps() {
  var storedValue = localStorage.getItem('showSteps');
  return storedValue === null ? config.showSteps.default : storedValue === 'true';
}

function configurationPage(showBluetoothIcon, batteryMode) {
  var checked = showBluetoothIcon ? ' checked' : '';
  var weatherChecked = configuredWeather() ? ' checked' : '';
  var stepsChecked = configuredSteps() ? ' checked' : '';
  var stepsOption = supportsSteps()
      ? '<p><label><input id="steps" type="checkbox"' + stepsChecked + '> ' +
        config.showSteps.label + '</label></p>' : '';
  var batteryOptions = config.batteryMode.options.map(function(option) {
    var selected = option.value === batteryMode ? ' selected' : '';
    return '<option value="' + option.value + '"' + selected + '>' + option.label + '</option>';
  }).join('');
  return 'data:text/html,' + encodeURIComponent(
      '<!doctype html><html><head><meta name="viewport" content="width=device-width, initial-scale=1">' +
      '<style>body{font-family:sans-serif;margin:24px}label{font-size:18px}</style></head><body>' +
      '<label><input id="bluetooth" type="checkbox"' + checked + '> ' +
      config.showBluetoothIcon.label + '</label>' +
      '<p><label for="battery">' + config.batteryMode.label + '</label><br>' +
      '<select id="battery">' + batteryOptions + '</select></p>' +
      '<p><label><input id="weather" type="checkbox"' + weatherChecked + '> ' +
      config.showWeather.label + '</label></p>' +
      stepsOption +
      '<p><button id="save">Save</button></p><script>' +
      'document.getElementById("save").onclick=function(){var value=document.getElementById("bluetooth").checked;' +
      'var battery=document.getElementById("battery").value;' +
      'var weather=document.getElementById("weather").checked;' +
      'location.href="pebblejs://close#"+encodeURIComponent(JSON.stringify({' +
      'showBluetoothIcon:value,batteryMode:battery,showWeather:weather,' +
      'showSteps:document.getElementById("steps")?document.getElementById("steps").checked:false}));};</script>' +
      '</body></html>');
}

Pebble.addEventListener('ready', function() {
  Pebble.sendAppMessage({ ShowBluetoothIcon: configuredValue() ? 1 : 0 });
  Pebble.sendAppMessage({ BatteryMode: parseInt(configuredBatteryMode(), 10) });
  Pebble.sendAppMessage({ WeatherEnabled: configuredWeather() ? 1 : 0 });
  if (supportsSteps()) {
    Pebble.sendAppMessage({ StepsEnabled: configuredSteps() ? 1 : 0 });
  }
});

Pebble.addEventListener('appmessage', function(event) {
  if (event.payload.WeatherEnabled !== undefined) {
    if (event.payload.WeatherEnabled !== 0) weather.start();
    else weather.stop();
  }
});

Pebble.addEventListener('showConfiguration', function() {
  Pebble.openURL(configurationPage(configuredValue(), configuredBatteryMode()));
});

Pebble.addEventListener('webviewclosed', function(event) {
  if (!event.response) {
    return;
  }
  var settings = JSON.parse(decodeURIComponent(event.response));
  localStorage.setItem('showBluetoothIcon', settings.showBluetoothIcon ? 'true' : 'false');
  localStorage.setItem('batteryMode', settings.batteryMode);
  localStorage.setItem('showWeather', settings.showWeather ? 'true' : 'false');
  if (supportsSteps()) {
    localStorage.setItem('showSteps', settings.showSteps ? 'true' : 'false');
    Pebble.sendAppMessage({ StepsEnabled: settings.showSteps ? 1 : 0 });
  }
  Pebble.sendAppMessage({ ShowBluetoothIcon: settings.showBluetoothIcon ? 1 : 0 });
  Pebble.sendAppMessage({ BatteryMode: parseInt(settings.batteryMode, 10) });
  Pebble.sendAppMessage({ WeatherEnabled: settings.showWeather ? 1 : 0 });
});
