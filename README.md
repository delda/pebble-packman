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

## Step count on Gabbro, Chalk, Emery and Basalt

Pebble Round 2, Pebble Time Round, Pebble Time 2, Pebble Time and Time Steel show
today's Pebble Health step count with a vector shoe icon. Steps are enabled by
default on Gabbro, Chalk, Emery and Basalt and can be hidden with **Show steps** in configuration.
Enable Pebble Health to provide the data; `--` means it is unavailable.
The count refreshes with movement, day changes and the minute tick.
Other platforms do not display the step indicator.

With Bluetooth, battery, weather and steps enabled, the four highest layout
slots are used, leaving the bottom slot empty. The two side slots are inset on
these platforms to keep the wider groups inside the dial.
`scripts/screenshots.sh gabbro` (or `chalk` / `emery` / `basalt`)
also captures all combinations with steps, using 13,947 simulated steps
and 75% battery. Screenshot builds use a non-persistent debug preview for steps
so Chalk firmware without emulator health injection shows the same value.
Release builds always use Pebble Health. See `resources/vector/README.md` for the shoe icon source.

Basalt uses the compact 19px gray shoe and Gothic 18 Bold count. The shoe is
stacked above the number so five digits fit in its narrower white playfield.
Weather also uses two rows, with its icon above the temperature, aligned with
the step indicator. Weather, Bluetooth and battery retain Basalt's compact sizes.
