var config = require('./config.json');

function configuredValue() {
  var storedValue = localStorage.getItem('showBluetoothIcon');
  return storedValue === null ? config.showBluetoothIcon.default : storedValue === 'true';
}

function configurationPage(showBluetoothIcon) {
  var checked = showBluetoothIcon ? ' checked' : '';
  return 'data:text/html,' + encodeURIComponent(
      '<!doctype html><html><head><meta name="viewport" content="width=device-width, initial-scale=1">' +
      '<style>body{font-family:sans-serif;margin:24px}label{font-size:18px}</style></head><body>' +
      '<label><input id="bluetooth" type="checkbox"' + checked + '> ' +
      config.showBluetoothIcon.label + '</label>' +
      '<p><button id="save">Save</button></p><script>' +
      'document.getElementById("save").onclick=function(){var value=document.getElementById("bluetooth").checked;' +
      'location.href="pebblejs://close#"+encodeURIComponent(JSON.stringify({showBluetoothIcon:value}));};</script>' +
      '</body></html>');
}

Pebble.addEventListener('ready', function() {
  Pebble.sendAppMessage({ ShowBluetoothIcon: configuredValue() ? 1 : 0 });
});

Pebble.addEventListener('showConfiguration', function() {
  Pebble.openURL(configurationPage(configuredValue()));
});

Pebble.addEventListener('webviewclosed', function(event) {
  if (!event.response) {
    return;
  }
  var settings = JSON.parse(decodeURIComponent(event.response));
  localStorage.setItem('showBluetoothIcon', settings.showBluetoothIcon ? 'true' : 'false');
  Pebble.sendAppMessage({ ShowBluetoothIcon: settings.showBluetoothIcon ? 1 : 0 });
});
