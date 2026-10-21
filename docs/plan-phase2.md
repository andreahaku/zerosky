# Phase 2: camera background

## Goal and current evidence

Show the IMX219 image behind the calculated sky on the CardputerZero's 320 x 170
canvas. Keep sensor fusion, star positions and search guidance independent of
the camera: if capture fails, the sky view remains usable.

The local `CameraApp/src/services/camera_service.cpp` acquires the IMX219 with
libcamera, requests an RGB565 preview, and turns the preview image by 180
degrees in its pixel conversion. Its README requires libcamera 0.7+ and an
arm64 development package or target sysroot. The template's
`cardputerzero-peripherals/references/media-and-sensors.md` recommends a
background preview session and copying only complete RGB565 frames into LVGL.
The handover records `dtoverlay=imx219`, `rpicam-hello` and `/dev/video0`, but
these have not been checked during this phase because the device is offline.

## Capture backend

Use `rpicam-vid` as a child process for the first device implementation.
Raspberry Pi documents its uncompressed YUV420 video output, `--nopreview`,
`--timeout 0`, `--rotation 180`, and `--output -` for stdout. The process is
launched without a shell using the vendored toolkit's `Subprocess`; its stderr
is left visible for diagnostics. See the [official camera software guide](https://www.raspberrypi.com/documentation/computers/camera_software.html)
and [rpicam video options](https://github.com/raspberrypi/documentation/blob/master/documentation/asciidoc/computers/camera/rpicam_options_vid.adoc).

The requested stream is 320 x 176, YUV420, 10 fps and a 180 degree rotation.
The height is a multiple of 16; the worker crops three rows from each end and
converts to RGB565 at 320 x 170, assuming full-range BT.601 YUV as used by the
rpicam YUV420 path. A raw frame is 84,480 bytes. Confirm the colour range on
the device.
The YUV pipe carries no per-frame header, so the worker accumulates exactly
one complete frame before publishing it. A separate thread owns the process,
conversion and a mutex-protected latest frame with a monotonic receipt time.
The UI copies that frame and should reject it when older than 500 ms. This
timestamp is when the bytes were received, not the sensor exposure time.

At 10 fps the raw pipe carries about 0.85 MB/s and conversion touches about
0.54 million output pixels/s. These are estimates; CPU use and latency must be
measured on the A53. The desktop uses `TestPatternSource`, which never opens
camera hardware. The process backend retries after an EOF or timeout, reports
an error and keeps the app alive if the camera or executable is missing.

The tradeoff is one external runtime dependency, `rpicam-vid`, and a fixed raw
frame layout. It avoids a new libcamera C++ link and the associated ABI and
DMA buffer lifecycle in ZeroSky. `ChildService` is for status-only processes;
this module uses `Subprocess` directly because it must read frame bytes. If
the target's raw output is padded or has an unexpected layout, use the proven
libcamera RGB565 preview path in CameraApp instead of guessing at pipe offsets.

## Device validation before view integration

1. Confirm `rpicam-vid --help` exists, reports YUV420 and rotation 180, and
   the IMX219 is listed. `rpicam-hello` alone does not prove the video command
   is installed.
2. Capture a short 320 x 176 YUV420 file using the same arguments as the
   module. Its byte count must be a multiple of 84,480. Check image colours,
   crop and the direction of a printed letter. The local CameraApp's 180
   degree turn is evidence to test, not a substitute for this visual check.
3. Run the app with the camera missing or held by CameraApp. The worker must
   report an error, stop promptly and leave the calculated sky usable. Verify
   that switching camera mode off releases the IMX219.
4. Measure delivered fps, CPU use, age of the latest frame and perceived lag
   while rotating the device. A moving camera frame and a current IMU pose can
   disagree; exposure timestamps or attitude history may be needed later.

The package needs `rpicam-vid` as a runtime dependency and the app-builder
camera permission set to true before this mode is shipped. The camera should
start only while its background mode is enabled. In that mode the view can copy
a fresh frame to the canvas, then draw sky marks and guidance over it; an old
or missing frame falls back to the existing sky fill. The overlay should avoid
opaque ground and sky fills on top of the photograph. Keep a way to return to
the dark red sky view.

## Optical alignment and field of view

The FOV of the fitted lens is not documented in this repository. Measure it
with the exact stream mode, crop and rotation used above. A pair of stars with
a known angular separation of roughly 10 to 30 degrees is more useful than the
Moon at this resolution. Place one star near the frame centre and a second
near a horizontal edge; if their angular separation from the optical axis is
`theta` and their horizontal pixel displacement is `dx`, estimate focal length
`f_px = dx / tan(theta)`. Then the horizontal FOV is
`2 atan(320 / (2 f_px))`. Repeat with several star pairs and positions, fit
the optical centre as well as focal length, and check radial distortion near
the edges. A symmetric pair around the centre can use
`f_px = separation_px / (2 tan(separation_angle / 2))`.

The Moon's diameter is only about half a degree. If the lens FOV is near 60
degrees, it would span only about 2 to 3 pixels at 320 x 170, so use a
higher-resolution still if measuring with the Moon. In either method, verify
that the optical axis matches the saved `pointing_body` and that screen right
and up agree with the projection. Store the measured horizontal FOV and
optical-centre offset as camera calibration values. Do not treat the app's
current manual zoom as the physical lens FOV while the image is visible.
