#!/usr/bin/env bash

# Build Packman, run it in a Pebble emulator, and capture the requested
# screenshot sequence. The platform is required:
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 PLATFORM" >&2
  exit 2
fi

readonly PLATFORM="$1"
readonly PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly SCREENSHOT_TIME="13:25"
readonly OUTPUT_DIR="$PROJECT_DIR/resources/images/screenshots/$PLATFORM/initial-animation"
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

start_emulator_attempt() {
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

start_emulator() {
  if start_emulator_attempt; then
    return 0
  fi

  # A stale Chalk SPI flash can prevent QEMU from reaching its boot banner.
  # Screenshot runs need a deterministic emulator state, so reset the SDK data
  # and retry once rather than leaving the capture sequence blocked.
  if [[ "$PLATFORM" == "chalk" ]]; then
    echo "Resetting stale Chalk emulator data and retrying." >&2
    if [[ -n "$emulator_logs_pid" ]]; then
      kill "$emulator_logs_pid" 2>/dev/null || true
      wait "$emulator_logs_pid" 2>/dev/null || true
    fi
    pebble kill --force >/dev/null 2>&1 || true
    pebble wipe >/dev/null
    emulator_logs_pid=""
    [[ -z "$emulator_logs_file" ]] || rm -f "$emulator_logs_file"
    emulator_logs_file=""
    start_emulator_attempt
    return
  fi

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
  # --emulator synchronises QEMU to the host clock before sending the command.
  pebble emu-set-time --emulator "$PLATFORM" "${SCREENSHOT_TIME}:00"
  sleep 1
}

capture_combination() {
  local sequence="$1"
  local bluetooth_visible="$2"
  local bluetooth_state="${3:-}"
  local battery_mode="$4"
  local weather_for_capture="${5:-0}"
  local screenshot_path="$OUTPUT_DIR/${sequence}.png"

  # Visibility and the displayed Bluetooth state are controlled by app messages.
  # Keep the emulator transport connected so subsequent commands can reach it.
  local weather_enabled="$weather_for_capture"
  local app_message=("10001=$bluetooth_visible" "10002=$battery_mode" "10004=$weather_enabled")
  if [[ "$weather_enabled" -eq 1 ]]; then
    # Deterministic values for screenshot comparisons: 21 C and clear at +3h.
    app_message+=("10005=21" "10006=1" "10007=$(date +%s)")
  fi
  if [[ -n "$bluetooth_state" ]]; then
    app_message+=("10003=$([[ "$bluetooth_state" == yes ]] && echo 1 || echo 0)")
  fi
  pebble emu-battery --emulator "$PLATFORM" --percent 100
  set_screenshot_time

  # Disconnecting Chalk here prevents send-app-message from fetching watch info.
  # BluetoothConnected (10003) already selects the disconnected artwork without
  # interrupting the transport used to configure and capture the next frame.
  pebble emu-bt-connection --emulator "$PLATFORM" --connected yes

  # Send directly to the emulator after all commands that can restart or
  # resynchronise the app, so PyPKJS cannot overwrite BatteryMode afterwards.
  pebble send-app-message --emulator "$PLATFORM" --app-uuid "$APP_UUID" \
    --int "${app_message[@]}"

  # Give the app-message callback and the layer redraw time to complete before
  # taking the raw QEMU framebuffer snapshot.
  sleep 1

  # Capture through QEMU's monitor. Using `pebble screenshot` would reconnect
  # to the emulator and synchronise its clock back to the host time.
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

# Build with PBL_DEBUG so main.c keeps the emulator backlight enabled via
# light_enable(true). A login shell loads nvm.
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
# Wait for PyPKJS's ready event to finish. It sends the persisted settings once
# at startup; the capture loop must apply its requested BatteryMode afterwards.
sleep 2
# only bluetooth
#capture_combination 00-none         0 yes 2 0
#capture_combination 01-bt-active    1 yes 2 0
#capture_combination 02-bt-off       1 yes 2 0
# bluetooth + battery
#capture_combination 00-none         0 yes 2 0
#capture_combination 01-bat-img      0 yes 0 0
#capture_combination 02-bat-perc     0 yes 1 0
#capture_combination 03-bt-active    1 yes 2 0
#capture_combination 04-bt-off       1 yes 2 0
#capture_combination 05-bt-bat       1 yes 0 0
# all
capture_combination 00-none         0 yes 2 0
capture_combination 01-bat-img      0 yes 0 0
capture_combination 02-bat-perc     0 yes 1 0
capture_combination 03-bt-active    1 yes 2 0
capture_combination 04-bt-off       1 no  2 0
capture_combination 05-meteo        0 yes 2 1
capture_combination 06-bt-bat       1 yes 0 0
capture_combination 07-bt-meteo     1 yes 2 1
capture_combination 08-bat-meteo    0 yes 0 1
capture_combination 09-bt-bat-meteo 1 yes 0 1
#capture_combination 05-battery-percentage 0 2 no
