#!/usr/bin/env bash

# Build Packman, run it in a Pebble emulator, record its initial animation as
# an MP4, then create a looping GIF from that video. Optionally pass a Pebble
# platform, for example: scripts/record_initial_animation.sh flint
set -euo pipefail

readonly PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly DEFAULT_PLATFORM="gabbro"
readonly ANIMATION_TIME="23:59"
readonly CAPTURE_DURATION_SECONDS=3
readonly CAPTURE_FPS=20
readonly FRAME_COUNT=$((CAPTURE_DURATION_SECONDS * CAPTURE_FPS))
readonly FRAME_INTERVAL_SECONDS="0.05"

readonly PLATFORM="${1:-$DEFAULT_PLATFORM}"
readonly OUTPUT_DIR="$PROJECT_DIR/resources/images/screenshots/$PLATFORM/initial-animation"
readonly OUTPUT_VIDEO="$OUTPUT_DIR/initial-animation.mp4"
readonly OUTPUT_GIF="$OUTPUT_DIR/initial-animation.gif"
readonly RELEASE_BUILD="$PROJECT_DIR/build/pebble-packman.pbw"
readonly DEBUG_BUILD="$PROJECT_DIR/build/pebble-packman_debug.pbw"

emulator_logs_pid=""
emulator_logs_file=""
frames_dir=""

require_command() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "$1 is required to record the animation." >&2
    exit 1
  fi
}

stop_emulator() {
  if [[ -n "$emulator_logs_pid" ]]; then
    kill "$emulator_logs_pid" 2>/dev/null || true
    wait "$emulator_logs_pid" 2>/dev/null || true
  fi
  [[ -z "$emulator_logs_file" ]] || rm -f "$emulator_logs_file"
  [[ -z "$frames_dir" ]] || rm -rf "$frames_dir"
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

capture_frames() {
  local frame frame_path monitor_port
  monitor_port="$(emulator_value qemu monitor)"

  for ((frame = 1; frame <= FRAME_COUNT; frame++)); do
    printf -v frame_path '%s/frame-%05d.ppm' "$frames_dir" "$frame"
    printf 'screendump %s\n' "$frame_path" | nc -N 127.0.0.1 "$monitor_port" >/dev/null
    test -s "$frame_path"
    sleep "$FRAME_INTERVAL_SECONDS"
  done
}

require_command ffmpeg
require_command nc
require_command pebble
require_command rg

trap stop_emulator EXIT
cd "$PROJECT_DIR"

# A dummy video driver lets QEMU run without an emulator window. Override it
# when an interactive window is useful.
export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-dummy}"

# Remove both bundles so the script never installs a stale build.
rm -f "$RELEASE_BUILD" "$DEBUG_BUILD"

# The Pebble SDK build requires this Node version; a login shell loads nvm.
bash -lic 'nvm use v24.14.0 && pebble build --debug'

if [[ -f "$RELEASE_BUILD" ]]; then
  readonly APP_BUILD="$RELEASE_BUILD"
elif [[ -f "$DEBUG_BUILD" ]]; then
  readonly APP_BUILD="$DEBUG_BUILD"
else
  echo "Pebble build did not produce a .pbw bundle." >&2
  exit 1
fi

mkdir -p "$OUTPUT_DIR"
rm -f "$OUTPUT_VIDEO" "$OUTPUT_GIF"
frames_dir="$(mktemp -d)"

start_emulator

# `emu-button` synchronises QEMU to the host clock when it connects. Wake the
# display first, then set the intended time immediately before the first app
# launch so its opening animation still starts at 23:59.
pebble emu-button --emulator "$PLATFORM" click back
pebble emu-set-time --emulator "$PLATFORM" "${ANIMATION_TIME}:00"
#sleep 1

readonly PYPKJS_PORT="$(emulator_value pypkjs port)"
pebble install --phone "localhost:$PYPKJS_PORT" "$APP_BUILD"
#sleep "$FRAME_INTERVAL_SECONDS"
capture_frames

# Encode the captured frames first, then derive the GIF from the MP4 so the
# video is the single source for both deliverables.
ffmpeg -y -loglevel error -framerate "$CAPTURE_FPS" \
  -i "$frames_dir/frame-%05d.ppm" \
  -c:v libx264 -pix_fmt yuv420p -movflags +faststart "$OUTPUT_VIDEO"

ffmpeg -y -loglevel error -i "$OUTPUT_VIDEO" \
  -filter_complex '[0:v]split[frames][palette];[palette]palettegen[colors];[frames][colors]paletteuse' \
  -loop 0 "$OUTPUT_GIF"

echo "Created $OUTPUT_VIDEO and $OUTPUT_GIF"
