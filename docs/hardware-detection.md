# Detecting the magnetic hardware configuration

Proposal, 20 October 2026. This describes a later implementation; the current
app still selects `none`, `cap`, or `usb_gps` through `ZEROSKY_HARDWARE` (default
`none`). The choice controls which magnetometer calibration profile is loaded.
The source of location coordinates is a separate choice: a saved city does not
prove that no GNSS accessory is attached.

## Evidence available today

| Signal | What it proves | What it does not prove |
| --- | --- | --- |
| A `/dev/ttyACM*` or `/dev/ttyUSB*` node | A USB serial interface exists. | That it is a GPS, or that a GPS is close enough to affect the BMM150. |
| Valid NMEA from a USB serial port | A GNSS receiver is responding, even without a position fix. | That no cap is also attached. |
| `/dev/ttyS0` exists | The built-in UART is available. | That the GNSS cap is mounted or powered. |
| NMEA from the cap UART while `ext_5v_out` is already on | A cap receiver is responding. | That silence means the physical cap is absent. |
| `ext_5v_out` is on | The 5 V rail is powered. | Which accessory, if any, is attached. |

The vendored `GnssReader` in `third_party/radio_toolkit/src/geo/location.cpp`
already listens to USB ports for up to 3 seconds each, then may power
`ext_5v_out`, wait 1.5 seconds, send a CASIC wake query on the shield UART, and
listen for up to 5 seconds. It restores the rail if it turned it on. This is a
useful GNSS discovery path, but using it again merely to choose a magnetic
profile would add startup delay, power cycling and possible magnetometer
disturbance. It may also contend for a serial port with the location dialog.
The reader currently labels an explicit `ZERORADIO_GPS_DEVICE` as `USB` even if
it points at the cap UART, so that label alone is not a sound profile key.

## Proposed policy

1. Keep a user-selected hardware configuration, and expose it in settings and
   in `zerosky_probe`. Keep `ZEROSKY_HARDWARE` as an explicit override. Record
   whether the choice came from the user, an override, or detection. Recheck
   the choice when USB devices change; do not silently reuse another profile.
2. Passively inspect USB serial devices through sysfs or udev. A device node
   alone is only a candidate. Confirm GNSS with bounded, read-only NMEA
   listening, sharing the existing `GnssReader` result where possible. Do not
   write commands to an unknown USB serial device. A valid sentence is enough;
   a satellite fix is not required to identify the receiver.
3. For the cap, use an already running GNSS reader's shield result, or listen
   when the 5 V rail is already on. Do not switch the rail on at every app
   launch solely to classify the cap. A silent or unpowered UART leaves the
   configuration *unconfirmed*, not `none`. Offer an explicit "check cap"
   action if active probing is useful; serialize it with the GNSS reader and
   restore the previous rail state on every exit path.
4. If both accessories are detected, or physical presence is ambiguous, ask
   for the configuration and permit a future combined profile key. Until a
   matching, valid profile is loaded, mark the compass `unreliable` and keep
   manual camera control available. If hardware changes while running,
   invalidate the previous profile immediately before selecting the new one.

Automatic cap identification may need a dedicated presence signal from the
device. GNSS silence cannot prove absence: the module can be asleep, have a
bad serial connection, or be mounted but unpowered. Even an unpowered cap may
change the local magnetic field. Therefore the first device release should
offer a manual choice instead of treating failed GNSS discovery as `none`.

## Validation before implementation

- On the physical CardputerZero, measure BMM150 readings with `none`, mounted
  cap off/on, and the actual USB GPS on the USB-A port. Decide whether cap
  power state or specific USB devices need separate profile identifiers.
- Measure GNSS discovery delay and the magnetic transient when `ext_5v_out`
  switches. Confirm the rail state is restored after success, timeout and
  interruption.
- Test with a non-GPS USB serial adapter, no satellite fix, a sleeping cap,
  both accessories present, and USB hotplug. Verify that every uncertain
  transition remains `unreliable` until the user confirms a matching profile.

See [the phase 1 plan](plan-phase1.md) for the calibration policy and
[the probe guide](probe.md) for field measurement commands.
