# radio_toolkit (vendored)

This directory is a copy of part of the ZeroRadio toolkit, under the MIT license in `LICENSE`.

- Source: https://github.com/andreahaku/zeroradio, directory `toolkit/`
- Commit: `90e5897` (2026-10-20)
- Copied modules: `src/app`, `src/reactive`, `src/view`, `src/platform`, `src/geo`, `src/logger`,
  `src/config`, and the tests `location_test` and `i18n_test`.
- Not copied: `src/map`, `src/model`, `src/net` and `src/app/memories_store.*` (radio only).
- `src/platform/remote_fb_proto.h` comes from ZeroRadio `tools/remote-fb/`.

## Local changes

Every change is marked with a `zerosky:` comment. Find them with `grep -rn "zerosky:" src`.

- `app/run_app.cpp`: the SDL window title is `APP_DISPLAY_NAME` and the logger tag is `APP_NAME`.
- `app/nav_hints.h`: the ESC hint names the launcher instead of the ZeroRadio menu.
- `view/theme.{h,cpp}`: `set_dark_palette_override()` replaces the dark palette (ZeroSky's red
  night theme), and `overlay_foreground()` gives the icon colour of the overlay NavBar.
- `view/widgets/navbar.cpp`: the overlay NavBar icons and page tab use `overlay_foreground()`
  instead of plain white.

The shared position stays in `~/.config/zeroradio/location` on purpose, so ZeroSky and ZeroRadio use
the same one.

## Sync from ZeroRadio

1. Diff the ZeroRadio toolkit against this copy:
   `diff -ru ../zeroradio/toolkit/src third_party/radio_toolkit/src`
2. Copy the upstream changes, keep the `zerosky:` changes, and update the commit above.
3. Build the simulator and run `ctest`.
