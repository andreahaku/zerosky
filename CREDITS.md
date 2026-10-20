# Credits

- App shell, widgets and shared location: the ZeroRadio toolkit (MIT), vendored in
  `third_party/radio_toolkit`.
- Sun, Moon and planet positions: [Astronomy Engine](https://github.com/cosinekitty/astronomy) v2.1.19
  by Don Cross (MIT), vendored in `third_party/astronomy`.
- Magnetic field and declination: [World Magnetic Model 2025](https://www.ncei.noaa.gov/products/world-magnetic-model)
  by NOAA/NCEI and the British Geological Survey. NOAA's modular C code and WMM2025
  coefficients are in `third_party/wmm`; see `third_party/wmm/VENDORED.md` for exact
  provenance. The NOAA code is US Government public domain material, not subject to
  copyright protection.
- Star catalog (`assets/sky/stars.tsv`, built by `tools/catalog/build_stars.py`): derived from the
  [HYG database](https://github.com/astronexus/HYG-Database) v4.1 by David Nash, CC BY-SA 4.0. The
  derived file is under the same license.
- Western constellation figure lines (`assets/sky/constellations.tsv`, built by
  `tools/catalog/build_lines.py`): converted from
  [d3-celestial](https://github.com/ofrohn/d3-celestial/blob/7e720a3de062059d4c5400a379146a601d9010e0/data/constellations.lines.json)
  by Olaf Frohn, BSD-3-Clause. Its source credits the constellation charts by the IAU and
  *Sky & Telescope*, CC BY 4.0. Chart credits: Roger Sinnott and Rick Fienberg (charts),
  Alan MacRobert (figure drawings). Conversion: GeoJSON polylines split into segments;
  vertices matched to HYG v4.1 HIP identifiers when within 0.01 degrees; two unmatched
  vertices retain J2000 coordinates. The figure lines are not official IAU boundaries.
  See `assets/sky/LICENSE-d3-celestial.txt` and `assets/sky/LICENSE-CC-BY-4.0.txt`.
- Deep-sky objects (`assets/sky/deep_sky.tsv`, built by `tools/catalog/build_deep_sky.py`): the 110
  Messier objects plus named or brighter than magnitude 10, derived from
  [OpenNGC](https://github.com/mattiaverga/OpenNGC) by Mattia Verga at commit 36cb178, CC BY-SA 4.0
  (`assets/sky/LICENSE-CC-BY-SA-4.0.txt`). The derived file is under the same license.
- City list (`assets/geodata/cities.tsv`): GeoNames cities15000, CC BY 4.0, https://www.geonames.org.
- Fonts (`assets/fonts`): see `assets/fonts/LICENSE.txt`.

The sky data sources are listed in [docs/data-sources.md](docs/data-sources.md).
