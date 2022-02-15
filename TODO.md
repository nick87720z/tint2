This set is not static.  
Some of these ideas are alternatives for same final result (feature group) - may need cleanup.

Sections
========
- [Global](#global)
- [Executors](#executors)
- [Taksbar](#taskbar)
- [Tooltip](#tooltip)
- [Configurator](#configurator)
- [Build system](#build_system)
- [Documentation](#documentation)

# Global

- Glib..Gtk stuff cleanup;
- Per-corner rounding support;
- Check fluxbox issues;
- External modules support;

## Executors

- Persistant command interpreters - command text could be fed into continuous process from configurable command.  
Compatible default - non-continuous shell;
- Support for RAW video streaming (ffmpeg is good candidate for streamer);
- Focus / pointer hover events;
- Ability to hold a window like taskbar task entry (feature group 1 - 2/1)
- Support sgr sequences for color, font, alignment etc - converting them into pango markup

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

- Library for executors, too big to inline in the Manual;
- Adopt tint2.wiki;
