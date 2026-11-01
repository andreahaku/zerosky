# Changelog

All notable changes to ZeroSky. Versions follow semantic versioning.

## 1.0.0

First release: a pocket planetarium for the [M5Stack CardputerZero](https://docs.m5stack.com/en/CardputerZero). ZeroSky computes the real sky for the current time and your position, and draws it around the direction the device points. Everything runs on the device, with no network.

### What it does

- Draws 5,070 stars to magnitude 6, 605 Messier, NGC and IC objects, western constellation figures, the Sun, Moon and planets.
- Follows the device: the IMU steers altitude and roll. With an outdoor magnetic calibration for the attached hardware, WMM2025 adds true north. Without it, `tilt only` uses a gyro heading that you align with the left/right arrows. TAB switches to manual control.
- Find: type part of a name, and a ring or an edge arrow guides you to the object. Variometer-style beeps get faster and higher as you get closer. While guiding, key 8 shows a stop sign that ends the guidance.
- Constellation lines with their Latin names, and the deep-sky objects, can each be switched on or off on the View page.
- Red night theme and blue day theme. The faintest visible star follows the Sun's altitude through daylight and the three twilights. The View page can show every star by day.
- Camera background: the IMX219 image appears behind the sky marks. Align matches the drawn sky to the photographed stars and saves the lens field and pointing offset.
- Location by city (offline GeoNames index), by coordinates, or by a GNSS fix from a USB receiver or the Cap LoRa-1262-GPS. ZeroSky shares the location file with ZeroRadio.
- Compass calibration in the app: Set: 8 opens a full-screen page. Move the device in a figure 8 outdoors until the eight direction dots light up; two tones announce the fit, and Enter saves it. The package sets the BMM150 magnetometer to 30 Hz so the capture takes less time.
- Theme, daylight filtering, constellations, deep sky, camera on or off and zoom are restored at the next start.
- Interface, in-app help (H) and manual in English, Italian, French, German, Spanish, Japanese and Simplified Chinese. The Set page picks the language.
- `zerosky_probe` measures the sensor axes, saves the frame mapping and captures magnetic calibration profiles.

### Not yet validated

- Pointing accuracy against the night sky. The Sun gave a first check on one CardputerZero v5.
- Magnetic calibration: only the `cap` profile was calibrated outdoors, on one device, with the probe and with the in-app page. The `none` and `usb_gps` profiles are untested.
- Camera lens field: 62.2 degrees horizontal is an estimate until someone aligns it with known stars. The status line says `FOV estimated` until an alignment is saved.
- Sensor axes were measured on one CardputerZero v5. Other units should be checked with `zerosky_probe`.

### Data sources

- Sun, Moon and planets: Astronomy Engine v2.1.19 (MIT).
- Stars: HYG database v4.1 (CC BY-SA 4.0).
- Deep-sky objects: OpenNGC (CC BY-SA 4.0).
- Constellation figures and name positions: d3-celestial (BSD-3-Clause), from the IAU and Sky & Telescope charts (CC BY 4.0).
- Magnetic field: World Magnetic Model 2025, NOAA/NCEI and BGS (public domain).
- Cities: GeoNames cities15000 (CC BY 4.0).

[CREDITS.md](CREDITS.md) and [docs/data-sources.md](docs/data-sources.md) give the exact versions and attributions.
