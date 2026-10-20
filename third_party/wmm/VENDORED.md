# World Magnetic Model 2025

This directory contains the NOAA/NCEI modular C geomagnetism library and the
standard degree-12 WMM2025 coefficients. The runtime uses the WGS84 ellipsoid;
no geoid raster is needed or bundled.

## Provenance

- `GeomagnetismLibrary.c` and `GeomagnetismHeader.h` were copied without changes
  from [dad98253/WMM2025 at c21826a20ee89c48fd21fd1f70db9b8609fe4e1d](https://github.com/dad98253/WMM2025/tree/c21826a20ee89c48fd21fd1f70db9b8609fe4e1d),
  a mirror of the NOAA modular C library. Their Git blob IDs are
  `4dacd63d754d4c7a48fc1aae3033d5b7cdbeeedb` and
  `7b09e05bbea5b93fcd5e5d2a186c3b3cc03aade1` respectively. The source
  header identifies NCEI and revision 1521 (2017-01-24).
- `WMM.COF` is the standard WMM2025 coefficient file from the official NOAA archive
  [WMM2025COF.zip](https://www.ncei.noaa.gov/sites/default/files/2024-12/WMM2025COF.zip)
  (dated 2024-12-10), copied without changes.
  SHA-256: `dfa8597825af4e0b87ff4198a5b4fb661b3c49f4cd090cd0164e0259b075582f`.
- The [official WMM2025 test table](https://www.ncei.noaa.gov/sites/default/files/2025-02/WMM2025testvalues.pdf)
  supplies the independent numerical reference used by `geomagnetic_test`.

The NOAA [WMM product page](https://www.ncei.noaa.gov/products/world-magnetic-model)
states that its source code is in the public domain and may be used freely.
NOAA/NCEI and the British Geological Survey should be credited for WMM2025
model values. This US Government material is not subject to copyright
protection. The ZeroSky wrapper in `src/orientation/geomagnetic.*` is MIT.

WMM2025 is valid through 2029. Replace the coefficient file and update the
wrapper's date guard when a newer official model is published. Run the official
test vectors after every update.
