# ZeroSky help

The sky view follows your position and time. On the device, it can also follow the IMU. H opens this page; F/X scroll and Esc closes it.

## Pages and movement

- 4 cycles Sky, View, Cam and Set. Hold 4 for the previous page.
- On the device, TAB switches between IMU steering and manual camera. The desktop is always manual.
- Outside Align, arrows move the manual view by 5 degrees. In tilt only mode, left/right arrows align the gyro's arbitrary north with the real sky; up/down follow the IMU. F/X/Z/C also act as up/down/left/right outside text entry.

## Sky page

- 5 opens Location. Enter a city, latitude and longitude, or choose a GPS fix. Use up/down and Enter to select. Location opens automatically on first use.
- 6 zooms out; 7 zooms in. Both keys are disabled while the camera is on. Sky zoom is saved when you leave the app.
- 8, marked by a magnifying glass, opens Find. Type part of an object name, choose a row and press Enter. While guiding, 8 shows a stop sign: press it to stop the guidance and the beeps. A ring marks an object on screen; an edge arrow points to one outside the view. Faster, higher beeps mean closer.

## View page

- 5 switches between red night and blue day themes.
- 6 cycles daylight filtering. A half star (Dim) hides faint stars by day; a full star (All) shows them.
- 7 cycles constellation figures: hidden, lines only, then lines with Latin names. The icons change from a crossed eye to connected lines to a star list.
- 8 shows or hides the 605 deep-sky symbols and names. Find can still locate a hidden object. Both View choices are saved between launches.

## Cam page

- 5, marked by a camera or crossed camera, switches the live camera background on or off. The first frame may take a few seconds. Lines, stars and labels remain visible over the photo.
- 6, marked by a crosshair or check mark, enters Align or finishes it. At night, point at known stars. Push a drawn star onto its photographed match: right moves the drawing right, left moves it left, and up/down move it the same way. Match near the centre first, then use 7 and 8 to narrow or widen the lens field until another star near an edge matches.
- In Align, each arrow changes the pointing offset by 0.5 degrees; 7 and 8 change the lens field by 1 degree. Press 6 again on Done to save only if you changed something. Switching the camera off or leaving with Esc discards unfinished Align changes.
- Until an alignment is saved, the status line says FOV estimated when the camera is on outside Align. The initial 62.2 degree lens field is an estimate.
- Night/day theme, daylight filtering, camera on/off and Sky zoom are remembered between launches. Sky zoom is saved on exit; Done saves lens alignment separately.

## Set page

- 5 chooses the language: Auto, EN, IT, FR, DE, ES, JA, ZH. The translate icon means Auto, which follows the system. Restart ZeroSky to apply the new language.
- 6 cycles the magnetic hardware profile: a prohibition sign for none, an antenna for cap, a USB plug for USB GPS. Use the configuration actually attached.
- 7 opens About, the full manual QR code and Credits.
- 8, marked by a compass, opens full-screen magnetic calibration for the selected hardware profile. Outdoors, away from metal, move the device in a figure 8 while turning every side. Eight dots show direction coverage; the bar shows progress to 200 independent samples and time left. It stops when each octant has at least 15 samples, or after 120 seconds. A check mark and two rising tones mean the fit is ready: Enter saves, Esc cancels. A low tone and a reason mean the fit failed.

## Language

- The interface, this help and the manual exist in English, Italian, French, German, Spanish, Japanese and Simplified Chinese. A system CJK font completes the Japanese and Chinese text.
- The Set page saves the choice in ~/.config/zeroradio/lang, a file for the whole ZeroRadio suite. For now only ZeroSky reads it. ZERORADIO_LANG overrides it; with Auto, LC_ALL, LC_MESSAGES and LANG decide.

## Status line

- Az and Alt are the view direction; FOV is the horizontal field of view. The location name follows.
- manual: IMU steering is off; the arrows control the camera.
- axes not measured: save the device frame mapping with zerosky_probe.
- no IMU: sensor discovery failed.
- tilt only: altitude and roll follow the IMU. Azimuth follows the gyro but north is arbitrary and can drift. Use left/right arrows to align it. A valid outdoor calibration for the selected hardware profile enables true-north steering.
- compass disturbed: the measured magnetic field does not match the expected field. The IMU can still steer, so move away from metal or electronics or press TAB for manual control.

## Calibration

- Measure the sensor axes, including the magnetometer, first. Save them with zerosky_probe --save-frames.
- Set a position, select the hardware profile on Set: 6, then press the compass on Set: 8 outdoors, away from metal. Move the device in a figure 8 while turning every side. The dialog shows distinct samples and covered octants; it stops at complete coverage or after 120 seconds.
- A successful fit shows RMS and the WMM field. Enter saves; Esc cancels without changing the old profile. If the fit fails, follow the reason shown and repeat. The source repository's docs/probe.md has the axis-measurement procedure.
