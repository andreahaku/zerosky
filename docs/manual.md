# ZeroSky user manual

ZeroSky is a pocket planetarium for the M5Stack CardputerZero. It draws the sky for the device's current time and your selected location, with red night and blue day themes. The IMU can steer the view even before magnetic calibration, with an arbitrary north direction. After calibration, the compass provides a true-north heading. You can also explore with the keys or show the live camera image beneath the calculated sky.

## Start here

1. Open ZeroSky from the CardputerZero launcher. On first use, the Location dialog appears. Choose a city, enter coordinates or wait for a GPS fix, then press Enter.
2. Check the device clock. The sky positions use the system time; selecting a GPS location does not set the clock.
3. Use the arrows, or F/X/Z/C, to explore manually. Press 4 to cycle through the Sky, View, Cam and Set pages of the bottom bar.
4. To follow the device, measure and save its sensor axes. Without magnetic calibration, `tilt only` follows altitude and roll, while left/right arrows align the heading. Calibrate the magnetometer outdoors to enable true-north steering. The [field probe guide](probe.md) gives the commands.

## Reading the sky

The centre of the display is the current pointing direction. Azimuth is measured clockwise from true north: 0° north, 90° east, 180° south and 270° west. Altitude is 0° on the horizon, positive above it. The status line shows `Az`, `Alt`, horizontal `FOV` and the selected location.

Stars are sized by brightness. Lines join the western constellation figures; they are illustrative rather than official IAU boundaries. Their Latin names appear at label points from d3-celestial. Deep-sky objects have chart symbols and catalog names. The Sun, Moon and planets have labels. The dim ground and horizon line show where the sky meets the local horizontal plane. The display uses calculated directions and a simple horizon; nearby buildings and terrain are not modelled.

With daylight filtering on, the star limit changes smoothly as the Sun descends through civil, nautical and astronomical twilight. This is a display aid, not a measurement of local sky brightness. Clouds, city lights and moonlight can change what you actually see. Turn daylight filtering off on the View page to show the night catalog during the day. Constellation lines and solar-system objects remain drawn by the current view.

## Keys

| Key | Sky page | View page | Cam page | Set page |
| --- | --- | --- | --- | --- |
| 4 | Next page; hold for previous | Next page; hold for previous | Next page; hold for previous | Next page; hold for previous |
| 5 | Open Location | Switch between night and day themes | Camera background On/Off | Choose the language |
| 6 | Zoom out (camera off) | Change daylight filtering: half star (Dim) or full star (All) | Align/Done | Choose `none`, `cap` or `usb_gps` magnetic hardware |
| 7 | Zoom in (camera off) | Cycle constellations: hidden, lines, lines with Latin names | Narrow lens FOV in Align | Open About, manual QR and Credits |
| 8 | Open Find (magnifying glass); while guiding, stop (stop sign) | Show/hide deep-sky symbols and names | Widen lens FOV in Align | Open compass calibration |

Outside Align, arrows turn the manual view by 5° per press. In `tilt only`, left/right arrows adjust the heading offset; altitude and roll follow the IMU. On the CardputerZero, F/X/Z/C are also up/down/left/right outside a text field. TAB switches between IMU steering and manual control on the device; the desktop simulator is always manual. H opens a shorter help page. ESC closes an open dialog or help page, or leaves the app when no overlay is open. In help and About, F/X scroll and Z/C change pages where available.

The Cam page uses a camera icon for **5** when the image is off and a crossed camera when it is on. **6** shows a crosshair for Align and a check mark for Done. The key numbers stay the same even when the icon changes.

On the View page, **7** shows a crossed eye when constellations are hidden, connected lines for figures alone, and a star list for figures with Latin names. **8** shows circles when deep-sky symbols and names are on, or a dashed circle when they are off. Names and symbols may be suppressed where they would crowd another label. The defaults are constellation lines with names and deep sky on. Both choices persist in `~/.config/zerosky/settings`. The Find list and its guidance still include deep-sky objects when their display is off.

With the camera off, the horizontal field of view runs from 15° to 120°. A narrower field magnifies the sky and shows fainter catalog stars when daylight filtering permits them. The Sky zoom keys are disabled while the camera is on. Sky zoom is saved when you leave the app, not after each key press.

## Camera and lens alignment

On the **Cam** page, press **5** to show the IMX219 image behind the calculated stars, constellation lines, horizon and guidance. The first image can take a few seconds. Lines and labels use an outline or dark backing so they remain readable over a bright scene. If the camera is missing or its frame becomes stale, the computed sky background remains usable. Press **5** again to release the camera. While it is on, Sky zoom is disabled and the view uses the lens FOV instead. The starting value, 62.2° horizontally, is an estimate from the Pi Camera v2, not a measurement of this unit. Until you save an alignment, the status line shows `FOV estimated` when the camera is on and Align is closed.

Measure the alignment at night with two or more known stars spread across the image:

1. Choose an open patch of sky with a recognisable star near the centre and another near the edge. Set the correct location and time, then turn on the camera with **Cam: 5**. A calibrated compass gives the most reliable starting direction.
2. Hold the device steady and press **Cam: 6** to enter **Align**. Use the arrows to push the drawn sky toward the photographed star until the central star coincides: right moves the drawing right, left moves it left, and up/down move it in their direction. Each press changes the screen-right or screen-up offset by 0.5°; the status line shows the offsets.
3. Keep the central star matched. Press **Cam: 7** to narrow or **Cam: 8** to widen the lens FOV in 1° steps until the star near the edge also coincides. Recheck the centre and repeat as needed.
4. Press **Cam: 6** again when key 6 shows the check mark (**Done**). If you changed a value, this saves the FOV and screen-right/screen-up pointing offsets in `~/.config/zerosky/camera-alignment` (or `$XDG_CONFIG_HOME/zerosky/camera-alignment`). These offsets apply only while the camera is on. **Done** without changes does not mark the estimated FOV as measured.

To discard unfinished changes, switch the camera off with **Cam: 5**, or leave the app with ESC while Align is open. Neither action saves the unfinished alignment. Once an alignment has been saved, `FOV estimated` disappears from the camera status line.

The camera alignment is separate from the IMU frame mapping and magnetic calibration. It corrects the photographed image's field and centre; it does not fix an incorrect clock, location or north reference. `ZEROSKY_CAMERA_FOV` overrides the stored FOV for a controlled run, so leave it unset while measuring. The optical alignment and nighttime pointing accuracy have not yet been verified on the device.

## Saved view settings

ZeroSky restores the selected night/day theme, Dim/All daylight filtering, camera On/Off and ordinary Sky zoom after a restart. These choices live in `~/.config/zerosky/settings` (or `$XDG_CONFIG_HOME/zerosky/settings`). Sky zoom is saved when you leave the app rather than on every zoom key press. Camera lens alignment has its own file as described above; the magnetic hardware choice stays in `~/.config/zerosky/hardware` and the language in `~/.config/zeroradio/lang`. Environment overrides such as `ZEROSKY_THEME`, `ZEROSKY_DAYLIGHT`, `ZEROSKY_CAMERA`, `ZEROSKY_CAMERA_FOV` and `ZEROSKY_HARDWARE` take precedence where supplied at startup. A saved camera On choice may start capture automatically on the next launch.

## Language

The interface, the in-app help and this manual exist in English, Italian, French, German, Spanish, Japanese and Simplified Chinese. Japanese and Chinese text uses a system CJK fallback font.

Press **Set: 5** to choose the language. Each press moves to the next choice: Auto, EN, IT, FR, DE, ES, JA, ZH, then Auto again. The key shows the two-letter code, or a translate icon for Auto. A note confirms the change. Restart ZeroSky to apply it: the interface keeps the old language until then. Auto follows the system locale.

The choice is saved in `~/.config/zeroradio/lang` (or `$XDG_CONFIG_HOME/zeroradio/lang`). The file is meant for the whole ZeroRadio suite, but for now only ZeroSky reads it. At startup ZeroSky reads `ZERORADIO_LANG` first, then this file, then `LC_ALL`, `LC_MESSAGES` and `LANG`. The first two letters choose the translation; an unsupported language falls back to English. The About page QR code opens this manual in the selected language.

## Location

Press **Sky: 5** to open Location. Type a city name from the offline city list or coordinates such as `36.05, 14.24` (latitude north positive, longitude east positive). Move with up/down and press Enter on the desired row. A USB GPS receiver or the Cap LoRa-1262-GPS can offer a fix; the GPS row is selectable only after it has a position. You can still use a city or typed coordinates without GNSS hardware.

ZeroSky saves the selected position in the location file shared with ZeroRadio, so the two apps can use the same place. The location source and the magnetic hardware setting are independent: choosing a city does not mean the cap or USB GPS is absent. Position, current date and WMM2025 determine the magnetic reference used by the compass.

## Find an object

Press **Sky: 8** to open Find. Type part of a name or catalog designation, then select a row and press Enter. The list includes the Sun, Moon, planets, named stars and the bundled Messier, NGC and IC objects; objects below the horizon are marked. While guiding, 8 shows a stop sign: press it to stop the guidance and the beeps. If the target is on screen, a ring marks it. If it is outside the field, an arrow on the edge shows which way to turn, and the label gives angular separation. A target below the horizon can be selected but gives no beep. Otherwise the audio cue becomes quicker and higher as you approach; within about 1° it becomes continuous. Sound requires a working audio device.

The search uses the app's computed positions. It is only as accurate as the device clock, location, frame mapping and magnetic calibration. For a target chosen at startup during a demo, `ZEROSKY_TARGET` can name a supported object.

## Pointing with the IMU

The BMI270 measures acceleration and rotation; the BMM150 supplies a magnetic direction. ZeroSky combines them with a Mahony filter and uses the World Magnetic Model 2025 to refer the heading to true north. The device's sensor axes and the axis represented by the screen centre must be measured on the physical unit. A factory identity matrix is not proof of alignment.

With a valid frame mapping but no usable magnetic calibration or reference, the status is `tilt only`. Altitude and roll follow the IMU. Azimuth follows the gyroscope from an arbitrary starting direction and can drift. Aim at a known object or landmark and use the left/right arrows to align the view; up/down arrows do not steer while the IMU is active. TAB switches to full manual control. Once the selected hardware setup has a valid outdoor calibration and the compass is healthy, ZeroSky uses the magnetic field for true-north steering and `tilt only` disappears.

The setup has two separate files under `~/.config/zerosky/` (or `$XDG_CONFIG_HOME/zerosky/`):

- `frame-mapping.json` records the verified accelerometer, gyroscope and magnetometer rotations, pointing axis and top-of-screen axis. Save schema 2 with `zerosky_probe --save-frames` and all five frame options after measuring the axes. Existing schema-1 files still load; if they have no magnetic axis, the app can reuse the measured `sensor_to_body` from an existing magnetic profile.
- `magnetometer-calibration.json` stores an outdoor hard-iron/soft-iron fit for each mounted hardware configuration. Capture with `zerosky_probe --capture none`, `--capture cap` or `--capture usb_gps`, using current position, UTC date and a verified magnetometer rotation.

The installed probe is `/usr/share/zerosky/bin/zerosky_probe`. Run it with `--help` for its options, and follow [docs/probe.md](probe.md) for the measurement procedure. Do not copy example rotation matrices from the guide unless they match axes measured on your device. The probe refuses a fit without sufficient three-dimensional coverage and preserves an existing valid profile if the new fit fails.

For magnetic calibration in the app, first set a position and select the physically attached hardware profile on **Set: 6**. Outdoors, away from metal, press the compass icon on **Set: 8**. Follow the large moving figure 8 while rotating the device through all sides, including face down. A flat figure 8 alone does not cover the sphere. Eight dots show covered octants, and the bar shows independent samples and time left. Capture stops at 200 independent samples with at least 15 in each octant, or after 120 seconds. Two ascending tones announce a successful fit; a low tone signals failure. The fit checks three-dimensional coverage and the held-out residual. A large check mark then appears with the RMS and expected WMM field. Press Enter to save the selected profile and reload it immediately, or Esc to cancel without writing. A failed fit gives a reason; the previous profile is kept. If no measured magnetic frame is available from schema 2 or an earlier profile, use the probe to save the axes first.

On the **Set** page, key 6 cycles through `none` (a prohibition sign), `cap` (an antenna) and `usb_gps` (a USB plug). Choose the physical configuration actually attached, even when the position came from a saved city. The selection is kept in `~/.config/zerosky/hardware`. `ZEROSKY_HARDWARE` selects an override at app startup for controlled runs. A missing profile for the selected configuration is never replaced silently by another one; the app uses `tilt only` while the IMU is available. Automatic accessory detection is not yet enabled.

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

The camera background and speaker work on the CardputerZero. The `cap` magnetic profile has been calibrated outdoors and the Sun used for a first alignment check; other hardware profiles, lens alignment and nighttime pointing accuracy still need validation. Long exposure and plate solving are not implemented. The [README](../README.md) has build instructions and development status.
