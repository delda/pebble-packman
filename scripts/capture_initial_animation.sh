#!/usr/bin/env bash

# Build Packman, run it in a Pebble emulator, and capture its start-up
# animation as an endlessly looping GIF. Pass a platform name to override the
# default, for example: scripts/capture_initial_animation.sh flint
set -euo pipefail

readonly PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly DEFAULT_PLATFORM="gabbro"
readonly ANIMATION_TIME="23:59"
readonly CAPTURE_DURATION_SECONDS=5
readonly CAPTURE_FPS=20
readonly FRAME_COUNT=$((CAPTURE_DURATION_SECONDS * CAPTURE_FPS))
readonly FRAME_INTERVAL_SECONDS="0.05"

emulator_logs_pid=""
emulator_logs_file=""

if ! command -v ffmpeg >/dev/null 2>&1; then
  echo "ffmpeg is required to create the GIF." >&2
  exit 1
fi

readonly PLATFORM="${1:-$DEFAULT_PLATFORM}"
readonly OUTPUT_DIR="$PROJECT_DIR/resources/images/screenshots/$PLATFORM/initial-animation"
readonly OUTPUT_GIF="$OUTPUT_DIR/initial-animation.gif"

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

capture_animation() {
  local frame frame_path monitor_port

  # QEMU's monitor channel avoids the VNC display :1 hard-coded by
  # pebble-tool, which often conflicts with a local desktop session.
  monitor_port="$(python3 -c '
import json
import sys

with open("/tmp/pb-emulator.json", encoding="utf-8") as state_file:
    state = json.load(state_file)
print(state[sys.argv[1]][next(iter(state[sys.argv[1]]))]["qemu"]["monitor"])
' "$PLATFORM")"

  for ((frame = 1; frame <= FRAME_COUNT; frame++)); do
    printf -v frame_path '%s/frame-%05d.ppm' "$OUTPUT_DIR" "$frame"
    printf 'screendump %s\n' "$frame_path" | nc -N 127.0.0.1 "$monitor_port" >/dev/null
    test -s "$frame_path"
    sleep "$FRAME_INTERVAL_SECONDS"
  done
}

emulator_pypkjs_port() {
  python3 -c '
import json
import sys

with open("/tmp/pb-emulator.json", encoding="utf-8") as state_file:
    state = json.load(state_file)
print(state[sys.argv[1]][next(iter(state[sys.argv[1]]))]["pypkjs"]["port"])
' "$PLATFORM"
}

trap stop_emulator EXIT

cd "$PROJECT_DIR"

# Allow QEMU to run headlessly; callers can override this for an interactive
# emulator window.
export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-dummy}"

# The Pebble SDK build requires this Node version. A login shell provides nvm.
bash -lic 'nvm use v24.14.0 && pebble build'

mkdir -p "$OUTPUT_DIR"
rm -f "$OUTPUT_DIR"/frame-*.png "$OUTPUT_DIR"/frame-*.ppm "$OUTPUT_GIF"

start_emulator

# Commands using --emulator synchronise the emulator to the host's current
# time when they connect. Set ANIMATION_TIME first, then install through the
# pypkjs proxy so the installation does not overwrite it.
readonly PYPKJS_PORT="$(emulator_pypkjs_port)"
readonly PYPKJS_ADDRESS="localhost:$PYPKJS_PORT"
pebble emu-set-time --emulator "$PLATFORM" "${ANIMATION_TIME}:00"
pebble install --phone "$PYPKJS_ADDRESS" build/pebble-packman.pbw
sleep "$FRAME_INTERVAL_SECONDS"

capture_animation

ffmpeg -y -loglevel error -framerate "$CAPTURE_FPS" \
  -pattern_type glob -i "$OUTPUT_DIR/frame-*.ppm" \
  -filter_complex '[0:v]split[frames][palette];[palette]palettegen[colors];[frames][colors]paletteuse' \
  -loop 0 "$OUTPUT_GIF"

echo "Created $OUTPUT_GIF"
