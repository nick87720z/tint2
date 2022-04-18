This set is not static.  
Some of these ideas are alternatives for same final result (feature group) - may need cleanup.

Sections
========
- [Global](#global)
- [Executors](#executors)
- [Launcher](#launcher)
- [Taksbar](#taskbar)
- [Tooltip](#tooltip)
- [Configurator](#configurator)
- [Build system](#build_system)
- [Documentation](#documentation)

# Global

- Glib..Gtk stuff cleanup;
- Check fluxbox issues;
- External modules support;
- Generic area margin support (like padding, but outside of border)

## Executors

- Global command sinks - action commands could use global command sinks pull, thus makign it available not only for executors;
Compatible default - non-continuous shell;
- Support for RAW video streaming (ffmpeg is good candidate for streamer);
- Focus / pointer hover events;
- Ability to hold a window like taskbar task entry (feature group 1 - 2/1)
- Support sgr sequences for color, font, alignment etc - converting them into pango markup

## Launcher

- Singleton applications support (hide if presents in taskbar). Requires window-launcher association support.

## Taskbar

- Option to use minimum space for certain tasks, could be useful when turnng them into indicators (feature group 1 - 2/2);
- Support executor output - text, image and tooltip (feature group 1 - 2/2)
- Support for pinned tasks (feature group 1 - 2/2)

## Tooltip

- Markup support (be it pango or anything else);
- Graphics output;
- Wrap arbitrary window for tooltip? (final lock aganst wayland)

## Configurator

- Better GUI toolkit;

## Build system

- Better replacement for cmake (not mandatory meson);

## Documentation

- Library for executors, which are too big to inline in the Manual;
- Adopt tint2.wiki;
