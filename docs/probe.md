# Field probe for orientation and magnetic calibration

`zerosky_probe` reads the actual BMI270 and BMM150 through IIO and prints one
status line every 200 ms. Acceleration and gyro are in the IIO sensor frame;
magnetic samples are scaled to microtesla but have no hard/soft-iron correction.
The output includes the Mahony quaternion, pointing azimuth/altitude and compass
status. `point[az,alt,unverified]` means the physical device axes have not been
measured, so those angles are diagnostic only.

For a first six-face axis check, run:

```sh
zerosky_probe --hardware none --seconds 60
```

Turn the device slowly and inspect the signs of `acc`, `gyro` and `mag` on each
face. The CLI never enables the GNSS cap's `ext_5v_out`; prepare the selected
hardware configuration externally. `--hardware` must describe the physical
configuration (`none`, `cap`, `usb_gps`, or a later identifier).

After the sensor-to-body mapping has been verified and the device is outdoors,
capture a profile with current latitude, longitude, WGS84 ellipsoid height and
UTC date. For example, the `--mag-to-body` value below is only appropriate if
the identity mapping was **measured** for the BMM150. The magnetometer mapping
may also be an orthogonal reflection (determinant −1) if its measured axis
signs require one; accelerometer and gyroscope frame mappings still require
proper rotations (determinant +1):

```sh
zerosky_probe --capture none --lat 36.05 --lon 14.24 --height-m 0 \
  --date 2026-10-20 --mag-to-body 1,0,0,0,1,0,0,0,1 --seconds 120
```

Rotate through all three dimensions for the entire capture, including inverted
poses. The fit needs at least 500 distinct samples, every octant, an axis ratio
below four, and a held-out RMS below 5 µT. If any check fails, the existing
profile remains in place. A successful save writes
`~/.config/zerosky/magnetometer-calibration.json` (or `$XDG_CONFIG_HOME`) and
keeps the previous complete file at `.bak`. `--calibration` and `--cof` override
the respective paths. Interrupt with Ctrl-C; an early capture may fail the
sample-count gate.

To display azimuth and altitude as verified, supply all four frame
parameters: `--accel-to-body` and `--gyro-to-body` (nine comma-separated
matrix entries each, row major), `--pointing-body` and
`--screen-up-body` (three entries each). The pointing axis and screen-up axis
must not be nearly parallel. The probe rejects an incomplete set. Compare the
output with an independent sky or geographic reference in several directions
before treating the heading as accurate.

Once these axes have been measured, save them for the app with the same four
options plus `--save-frames`. This validates the matrices and axes, writes
`~/.config/zerosky/frame-mapping.json` (or `$XDG_CONFIG_HOME`) atomically, and
exits without opening IIO. `--frame-mapping FILE` overrides the path. A previous
valid mapping is preserved at `.bak`. The magnetometer's `--mag-to-body` is
stored separately with its calibration profile; it is still required for
`--capture`.

## Measured on CardputerZero v5

The BMI270 accelerometer and gyroscope use the device body frame directly:
`+x` points toward the lower edge, `+y` toward the right edge, and `+z` out
of the screen. Both `accel_to_body` and `gyro_to_body` are the identity matrix.
A 90 degree clockwise turn while lying flat produced about -83 degrees from
the gyro sampled at 5 Hz, confirming its sign.

The BMM150 has `x` and `z` reversed relative to the BMI270. Its measured
`mag_to_body` is `diag(-1, 1, -1)`, or
`-1,0,0,0,1,0,0,0,-1` in the probe CLI. This matrix has determinant +1.
For the screen-as-sight convention, `pointing_body` is `(0, 0, -1)` and
`screen_up_body` is `(-1, 0, 0)`: the back of the device points at the sky.
When placed face down, the resulting pointing altitude was about 88 degrees.

The frame mapping has already been saved on the measured device. To capture
its magnetic profile with the cap mounted, use the installed probe and the
current outdoor position, WGS84 ellipsoid height and UTC date. The Gozo values
below are an example; replace them if the measurement site or date differs:

```sh
/usr/share/zerosky/bin/zerosky_probe --capture cap \
  --lat 36.05 --lon 14.24 --height-m 0 --date 2026-10-20 \
  --mag-to-body -1,0,0,0,1,0,0,0,-1 --seconds 120
```
