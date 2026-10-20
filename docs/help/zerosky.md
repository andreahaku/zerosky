# ZeroSky help

The sky view follows your position and time. On the device, it can also follow the IMU. H opens this page; F/X scroll and Esc closes it.

## Pages and movement

- 4 switches between Sky and View. Hold 4 for the previous page.
- On the device, TAB switches between IMU steering and manual camera. The desktop is always manual.
- In manual mode, arrows move the view by 5 degrees. In tilt only mode, left/right arrows align the gyro's arbitrary north with the real sky; up/down follow the IMU. F/X/Z/C also act as up/down/left/right outside text entry.

## Sky page

- 5 opens Location. Enter a city, latitude and longitude, or choose a GPS fix. Use up/down and Enter to select. Location opens automatically on first use.
- 6 zooms out; 7 zooms in.
- 8 opens Find. Type part of an object name, choose a row and press Enter. Hold 8 to stop guiding. A ring marks an object on screen; an edge arrow points to one outside the view. Faster, higher beeps mean closer.

## View page

- 5 switches between red night and blue day themes.
- 6 cycles daylight filtering: Dim hides faint stars by day, All shows them.
- 7 cycles the magnetic hardware profile: none, cap, USB GPS. Use the configuration actually attached.
- 8 opens About, the full manual QR code and Credits.

## Status line

- Az and Alt are the view direction; FOV is the horizontal field of view. The location name follows.
- manual: IMU steering is off; the arrows control the camera.
- axes not measured: save the device frame mapping with zerosky_probe.
- no IMU: sensor discovery failed.
- tilt only: altitude and roll follow the IMU. Azimuth follows the gyro but north is arbitrary and can drift. Use left/right arrows to align it. A valid outdoor calibration for the selected hardware profile enables true-north steering.
- compass disturbed: the measured magnetic field does not match the expected field. The IMU can still steer, so move away from metal or electronics or press TAB for manual control.

## Calibration

- Measure the sensor axes and pointing direction first. Save them with zerosky_probe --save-frames.
- Outdoors, rotate the device in all directions while zerosky_probe captures a magnetic profile for the mounted hardware. Repeat for each configuration you use.
- Calibration is done with zerosky_probe, not from this screen. The full guide is docs/probe.md in the source repository.
