# pebble-packman

A Pebble watchapp/watchface written in C using the Pebble SDK.

## Building & running

```sh
pebble build --debug                   # build for all targetPlatforms
scripts/install_emulator.sh chalk      # start Chalk and install the debug bundle
pebble install --phone <ip>           # install to a paired phone
```

## Target platforms

`targetPlatforms` in `package.json` controls which watches you build for. This
project includes **aplite** (original Pebble) and **basalt** (Pebble Time)
alongside chalk, diorite, emery, flint, and gabbro. Aplite uses Diorite's
144×168 rectangular layout; its monochrome display maps the palette
accordingly. Basalt uses the same geometry while retaining its color palette.

## Project layout

```
src/c/           C source for the watchapp
src/pkjs/        PebbleKit JS (phone-side) source, if any
worker_src/c/    Background worker source, if any
resources/       Images, fonts, and other bundled resources
package.json     Project metadata (UUID, platforms, resources, message keys)
wscript          Build rules — usually no need to edit
```

By default this project is configured as a watchapp. To make it a watchface,
set `pebble.watchapp.watchface` to `true` in `package.json`.

## Documentation

Full SDK docs, tutorials, and API reference: <https://developer.repebble.com>

## Step indicator on all platforms

Pebble Round 2, Pebble Time Round, Pebble Time 2, Pebble Time, Time Steel, Pebble 2 Duo and Pebble 2 show
today's Pebble Health step count with a vector shoe icon. Steps are enabled by
default on all platforms and can be hidden with **Show steps** in configuration.
Enable Pebble Health to provide the data; `--` means it is unavailable.
The count refreshes with movement, day changes and the minute tick.
Aplite has no Pebble Health support: its enabled step indicator shows `--`.
The debug preview can still display a simulated count to verify the layout.

With all four indicators enabled, all platforms use two columns and two rows
mirrored about the dial center. Weather and steps stack their icons above the
text so both groups fit inside the white dial. Gabbro and Chalk use a tighter
grid to keep the lower groups inside the circular dial.
`scripts/screenshots.sh gabbro` (or `chalk` / `emery` / `basalt` / `flint` / `diorite` / `aplite`)
also captures all combinations with steps, using 13,947 simulated steps
and 75% battery. Screenshot builds use a non-persistent debug preview for steps
so Chalk firmware without emulator health injection shows the same value.
Release builds use Pebble Health where supported; Aplite shows `--`. See `resources/vector/README.md` for the shoe icon source.

Basalt uses the compact 19px gray shoe and Gothic 18 Bold count. The shoe is
stacked above the number so five digits fit in its narrower white playfield.
Weather also uses two rows, with its icon above the temperature, aligned with
the step indicator. Weather, Bluetooth and battery retain Basalt's compact sizes.

Aplite, Flint and Diorite share Basalt's 144×168 layout, fonts, hand lengths and indicator positions.
Their monochrome shoe has a white fill and black outline and details; only the
color palette differs from Basalt.
