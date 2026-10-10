#!/usr/bin/env bash

# Build Packman, run it in a Pebble emulator, and capture one screenshot for
# every supported weather icon. The platform is required.
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 PLATFORM" >&2
  exit 2
fi

readonly PLATFORM="$1"
if [[ "$PLATFORM" != "gabbro" ]]; then
  echo "Weather screenshots are currently supported only on gabbro." >&2
  exit 2
fi
readonly PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly SCREENSHOT_TIME="13:25"
readonly OUTPUT_DIR="$PROJECT_DIR/resources/images/screenshots/$PLATFORM/weather"
readonly DEBUG_BUNDLE="$PROJECT_DIR/build/pebble-packman_debug.pbw"
readonly APP_UUID="cca111ca-06ea-4a19-a7d7-a3bd06326f79"

emulator_logs_pid=""
emulator_logs_file=""

stop_emulator() {
  if [[ -n "$emulator_logs_pid" ]]; then
    kill "$emulator_logs_pid" 2>/dev/null || true
    wait "$emulator_logs_pid" 2>/dev/null || true
  fi
  if [[ -n "$emulator_logs_file" ]]; then
    rm -f "$emulator_logs_file"
  fi
  pebble kill --force >/dev/null 2>&1 || true
}

start_emulator() {
  local attempt
  pebble kill --force >/dev/null 2>&1 || true
  emulator_logs_file="$(mktemp)"
  pebble logs -vv --emulator "$PLATFORM" >"$emulator_logs_file" 2>&1 &
  emulator_logs_pid=$!

  for attempt in {1..30}; do
    if { rg -q 'Firmware booted\.' "$emulator_logs_file" &&
         rg -q 'pypkjs:Ready\.' "$emulator_logs_file"; } ||
       { rg -q 'QEMU is already running\.' "$emulator_logs_file" &&
         rg -q 'pypkjs is already running\.' "$emulator_logs_file"; }; then
      return 0
    fi
    if ! kill -0 "$emulator_logs_pid" 2>/dev/null; then
      break
    fi
    sleep 1
  done

  cat "$emulator_logs_file" >&2
  echo "Unable to start a ready $PLATFORM emulator." >&2
  return 1
}

emulator_value() {
  python3 -c '
import json
import sys

with open("/tmp/pb-emulator.json", encoding="utf-8") as state_file:
    state = json.load(state_file)
print(state[sys.argv[1]][next(iter(state[sys.argv[1]]))][sys.argv[2]][sys.argv[3]])
' "$PLATFORM" "$1" "$2"
}

set_screenshot_time() {
  pebble emu-set-time --emulator "$PLATFORM" "${SCREENSHOT_TIME}:00"
  sleep 1
}

capture_weather() {
  local sequence="$1"
  local name="$2"
  local icon="$3"
  local screenshot_path="$OUTPUT_DIR/${sequence}-${name}.png"
  local app_message=(
    "10001=0"  # Bluetooth hidden
    "10002=2"  # Battery hidden
    "10003=0"  # Bluetooth disconnected
    "10004=1"  # Weather enabled
    "10005=21" # Deterministic current temperature
    "10006=$icon"
    "10007=$(date +%s)"
  )

  pebble emu-battery --emulator "$PLATFORM" --percent 100
  set_screenshot_time
  pebble send-app-message --emulator "$PLATFORM" --app-uuid "$APP_UUID" \
    --int "${app_message[@]}"
  sleep 1

  local monitor_port ppm_path
  monitor_port="$(emulator_value qemu monitor)"
  ppm_path="${screenshot_path%.png}.ppm"
  printf 'screendump %s\n' "$ppm_path" |
    nc -N 127.0.0.1 "$monitor_port" >/dev/null
  test -s "$ppm_path"
  ffmpeg -y -loglevel error -i "$ppm_path" "$screenshot_path"
  rm -f "$ppm_path"
  echo "Created $screenshot_path"
}

trap stop_emulator EXIT

cd "$PROJECT_DIR"
export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-dummy}"

# Build with PBL_DEBUG so main.c keeps the emulator backlight enabled.
bash -lic 'nvm use v24.14.0 && pebble clean && pebble build --debug'

if [[ ! -f "$DEBUG_BUNDLE" ]]; then
  echo "Pebble debug build did not produce $DEBUG_BUNDLE." >&2
  exit 1
fi

mkdir -p "$OUTPUT_DIR"
rm -f "$OUTPUT_DIR"/[0-9][0-9]-*.png

start_emulator
pebble emu-button --emulator "$PLATFORM" click back
pebble install --emulator "$PLATFORM" "$DEBUG_BUNDLE"
sleep 2

capture_weather 01 clear   1
capture_weather 02 cloudy  2
capture_weather 03 rain    3
capture_weather 04 snow    4
capture_weather 05 thunder 5
capture_weather 06 fog     6
