# ZeroSky user manual

ZeroSky is a pocket planetarium for the M5Stack CardputerZero. It draws the sky for the device's current time and your selected location, with red night and blue day themes. The IMU can steer the view even before magnetic calibration, with an arbitrary north direction. After calibration, the compass provides a true-north heading. You can also explore with the keys. The current version draws a computed sky; it does not show a camera image.

## Start here

1. Open ZeroSky from the CardputerZero launcher. On first use, the Location dialog appears. Choose a city, enter coordinates or wait for a GPS fix, then press Enter.
2. Check the device clock. The sky positions use the system time; selecting a GPS location does not set the clock.
3. Use the arrows, or F/X/Z/C, to explore manually. Press 4 to move between the Sky and View pages of the bottom bar.
4. To follow the device, measure and save its sensor axes. Without magnetic calibration, `tilt only` follows altitude and roll, while left/right arrows align the heading. Calibrate the magnetometer outdoors to enable true-north steering. The [field probe guide](probe.md) gives the commands.

## Reading the sky

The centre of the display is the current pointing direction. Azimuth is measured clockwise from true north: 0° north, 90° east, 180° south and 270° west. Altitude is 0° on the horizon, positive above it. The status line shows `Az`, `Alt`, horizontal `FOV` and the selected location.

Stars are sized by brightness. Lines join the western constellation figures; they are illustrative rather than official IAU boundaries. The Sun, Moon and planets have labels. The dim ground and horizon line show where the sky meets the local horizontal plane. The display uses calculated directions and a simple horizon; nearby buildings and terrain are not modelled.

With daylight filtering on, the star limit changes smoothly as the Sun descends through civil, nautical and astronomical twilight. This is a display aid, not a measurement of local sky brightness. Clouds, city lights and moonlight can change what you actually see. Turn daylight filtering off on the View page to show the night catalog during the day. Constellation lines and solar-system objects remain drawn by the current view.

## Keys

| Key | Sky page | View page |
| --- | --- | --- |
| 4 | Switch to View; hold to go back | Switch to Sky; hold to go back |
| 5 | Open Location | Switch between night and day themes |
| 6 | Zoom out | Change daylight filtering: Dim or All |
| 7 | Zoom in | Choose `none`, `cap` or `usb_gps` magnetic hardware |
| 8 | Open Find; hold to stop guidance | Open About, manual QR and Credits |

Arrows turn the manual view by 5° per press. In `tilt only`, left/right arrows adjust the heading offset; altitude and roll follow the IMU. On the CardputerZero, F/X/Z/C are also up/down/left/right outside a text field. TAB switches between IMU steering and manual control on the device; the desktop simulator is always manual. H opens a shorter help page. ESC closes an open dialog or help page, or leaves the app when no overlay is open. In help and About, F/X scroll and Z/C change pages where available.

The field of view runs from 15° to 120° horizontally. A narrower field magnifies the sky and shows fainter catalog stars when daylight filtering permits them.

## Location

Press **Sky: 5** to open Location. Type a city name from the offline city list or coordinates such as `36.05, 14.24` (latitude north positive, longitude east positive). Move with up/down and press Enter on the desired row. A USB GPS receiver or the Cap LoRa-1262-GPS can offer a fix; the GPS row is selectable only after it has a position. You can still use a city or typed coordinates without GNSS hardware.

ZeroSky saves the selected position in the location file shared with ZeroRadio, so the two apps can use the same place. The location source and the magnetic hardware setting are independent: choosing a city does not mean the cap or USB GPS is absent. Position, current date and WMM2025 determine the magnetic reference used by the compass.

## Find an object

Press **Sky: 8** to open Find. Type part of a name or catalog designation, then select a row and press Enter. The list includes the Sun, Moon, planets, named stars and the bundled Messier, NGC and IC objects; objects below the horizon are marked. Hold 8 to stop guidance. If the target is on screen, a ring marks it. If it is outside the field, an arrow on the edge shows which way to turn, and the label gives angular separation. A target below the horizon can be selected but gives no beep. Otherwise the audio cue becomes quicker and higher as you approach; within about 1° it becomes continuous. Sound requires a working audio device.

The search uses the app's computed positions. It is only as accurate as the device clock, location, frame mapping and magnetic calibration. For a target chosen at startup during a demo, `ZEROSKY_TARGET` can name a supported object.

## Pointing with the IMU

The BMI270 measures acceleration and rotation; the BMM150 supplies a magnetic direction. ZeroSky combines them with a Mahony filter and uses the World Magnetic Model 2025 to refer the heading to true north. The device's sensor axes and the axis represented by the screen centre must be measured on the physical unit. A factory identity matrix is not proof of alignment.

With a valid frame mapping but no usable magnetic calibration or reference, the status is `tilt only`. Altitude and roll follow the IMU. Azimuth follows the gyroscope from an arbitrary starting direction and can drift. Aim at a known object or landmark and use the left/right arrows to align the view; up/down arrows do not steer while the IMU is active. TAB switches to full manual control. Once the selected hardware setup has a valid outdoor calibration and the compass is healthy, ZeroSky uses the magnetic field for true-north steering and `tilt only` disappears.

The setup has two separate files under `~/.config/zerosky/` (or `$XDG_CONFIG_HOME/zerosky/`):

- `frame-mapping.json` records the verified accelerometer and gyroscope rotations, pointing axis and top-of-screen axis. Save it with `zerosky_probe --save-frames` after checking the six faces and several reference directions.
- `magnetometer-calibration.json` stores an outdoor hard-iron/soft-iron fit for each mounted hardware configuration. Capture with `zerosky_probe --capture none`, `--capture cap` or `--capture usb_gps`, using current position, UTC date and a verified magnetometer rotation.

The installed probe is `/usr/share/zerosky/bin/zerosky_probe`. Run it with `--help` for its options, and follow [docs/probe.md](probe.md) for the measurement procedure. Do not copy example rotation matrices from the guide unless they match axes measured on your device. The probe refuses a fit without sufficient three-dimensional coverage and preserves an existing valid profile if the new fit fails.

On the **View** page, key 7 cycles through `none`, `cap` and `usb_gps`. Choose the physical configuration actually attached, even when the position came from a saved city. The selection is kept in `~/.config/zerosky/hardware`. `ZEROSKY_HARDWARE` selects an override at app startup for controlled runs. A missing profile for the selected configuration is never replaced silently by another one; the app uses `tilt only` while the IMU is available. Automatic accessory detection is not yet enabled.

## Status messages

| Message | Meaning and action |
| --- | --- |
| `manual` | The camera is under key control. TAB can enable IMU steering when it is available. |
| `axes not measured` | No valid frame mapping was loaded. Verify axes with the probe and save the mapping. |
| `bad axes file` | The saved frame mapping failed validation. Repeat the frame check and save a valid file. |
| `no IMU` or `IMU did not start` | The app could not discover or start the sensors. Manual control remains available. |
| `tilt only` | Altitude and roll follow the IMU; the gyro heading has arbitrary north and can drift. Align it with left/right arrows. Calibrate the selected hardware setup outdoors for true-north steering. |
| `compass disturbed` | The measured field disagrees with the expected field. IMU steering can continue with an unreliable heading; move away from nearby metal or electronics, or press TAB for manual control. |

The app updates the sky positions periodically. An inaccurate date or location moves the predicted objects even when the compass is healthy. A calibration made next to a metal railing can also pass file validation while producing a poor heading elsewhere; confirm it against an independent sky or geographic reference outdoors.

## Data, licenses and current limits

The star catalog contains 5,070 HYG v4.1 stars to magnitude 6. The app precesses their J2000 positions to the observation date, but does not yet apply individual stellar proper motion. A derived OpenNGC selection adds 605 Messier, NGC and IC objects. Astronomy Engine computes the Sun, Moon and planets. WMM2025 supplies magnetic declination and expected field strength offline. The GeoNames city list is also offline.

ZeroSky code is MIT licensed. The derived HYG star and OpenNGC deep-sky files are CC BY-SA 4.0; western constellation lines carry d3-celestial BSD-3-Clause and IAU / *Sky & Telescope* CC BY 4.0 notices. WMM code and coefficients are US Government public domain material. See [CREDITS.md](../CREDITS.md) and [data sources](data-sources.md) for revisions and attribution.

The current release has no camera background, long exposure or plate solving. The speaker works on the CardputerZero; outdoor magnetic calibration and pointing accuracy still require validation. The [README](../README.md) has build instructions and development status.
