#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
# SPDX-License-Identifier: MIT
"""Convert pinned d3-celestial western figure lines into assets/sky/constellations.tsv.

Source: Olaf Frohn, d3-celestial constellations.lines.json at COMMIT, BSD-3-Clause.
The source says its western figures adapt IAU / Sky & Telescope constellation
charts (CC BY 4.0). See CREDITS.md and docs/data-sources.md for attribution.
The generated dataset retains both notices. Coordinates are J2000 equatorial.

    python3 tools/catalog/build_lines.py
    python3 tools/catalog/build_lines.py --json /path/to/constellations.lines.json
"""

import argparse
import csv
import hashlib
import json
import math
import sys
import urllib.request
from pathlib import Path

COMMIT = "7e720a3de062059d4c5400a379146a601d9010e0"
URL = ("https://raw.githubusercontent.com/ofrohn/d3-celestial/"
       f"{COMMIT}/data/constellations.lines.json")
SHA256 = "294f66bef5d5cf50b1e17f16d2efa1d97a15131612c68dd935adef6e7373e13c"
MATCH_DEG = 0.01


def stars_from_tsv(path):
    stars = []
    with open(path, encoding="utf-8", newline="") as stream:
        for row in csv.reader(stream, delimiter="\t"):
            if not row or row[0].startswith("#") or not row[0]:
                continue
            hip = int(row[0])
            if hip <= 0:
                continue
            ra, dec = math.radians(float(row[1]) * 15), math.radians(float(row[2]))
            stars.append((hip, (math.cos(dec) * math.cos(ra),
                                math.cos(dec) * math.sin(ra), math.sin(dec))))
    return stars


def matched_hip(ra_deg, dec_deg, stars):
    ra, dec = math.radians(ra_deg), math.radians(dec_deg)
    vec = (math.cos(dec) * math.cos(ra), math.cos(dec) * math.sin(ra), math.sin(dec))
    hip, dot = max(((h, sum(a * b for a, b in zip(vec, v))) for h, v in stars),
                   key=lambda pair: pair[1])
    return hip if dot >= math.cos(math.radians(MATCH_DEG)) else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--json", help="local copy of pinned constellations.lines.json")
    ap.add_argument("--stars", default="assets/sky/stars.tsv")
    ap.add_argument("--out", default="assets/sky/constellations.tsv")
    args = ap.parse_args()

    if args.json:
        source = Path(args.json).read_bytes()
    else:
        with urllib.request.urlopen(URL, timeout=120) as response:
            source = response.read()
    digest = hashlib.sha256(source).hexdigest()
    if digest != SHA256:
        ap.error(f"source SHA-256 differs from pinned d3-celestial file: {digest}")
    data = json.loads(source)
    if data.get("type") != "FeatureCollection":
        ap.error("source is not a GeoJSON FeatureCollection")
    features = data["features"]
    if len({feature["id"] for feature in features}) != 88:
        ap.error("source does not contain 88 constellation abbreviations")

    stars = stars_from_tsv(args.stars)
    if not stars:
        ap.error("star catalog has no HIP entries")
    vertices = {tuple(point) for feature in features
                for line in feature["geometry"]["coordinates"] for point in line}
    matches = {point: matched_hip(*point, stars) for point in vertices}

    segments = []
    for feature in features:
        if feature["geometry"]["type"] != "MultiLineString":
            ap.error(f"unexpected geometry in {feature['id']}")
        con = feature["id"]
        rank = int(feature["properties"]["rank"])
        for line in feature["geometry"]["coordinates"]:
            for a, b in zip(line, line[1:]):
                def endpoint(point):
                    ra_deg, dec_deg = point
                    return (matches[tuple(point)], f"{(ra_deg % 360) / 15:.6f}",
                            f"{dec_deg:.5f}")
                segments.append((con, rank, *endpoint(a), *endpoint(b)))

    with open(args.out, "w", encoding="utf-8", newline="") as stream:
        stream.write(f"# Western constellation figure lines from d3-celestial {COMMIT} by Olaf Frohn.\n")
        stream.write(f"# Source: {URL}\n")
        stream.write(f"# Source SHA-256: {SHA256}\n")
        stream.write("# d3-celestial: BSD-3-Clause; adapted IAU / Sky & Telescope charts: CC BY 4.0.\n")
        stream.write("# Chart credits: IAU, Sky & Telescope, Roger Sinnott, Rick Fienberg, Alan MacRobert.\n")
        stream.write("# Converted GeoJSON polylines to segments; matched vertices within 0.01 deg to HYG v4.1 HIP IDs.\n")
        stream.write("# Unmatched vertices have HIP 0 and retain J2000 RA/Dec. Details: CREDITS.md.\n")
        stream.write("# con<TAB>rank<TAB>hip_a<TAB>ra_a_hours<TAB>dec_a_deg<TAB>hip_b<TAB>ra_b_hours<TAB>dec_b_deg\n")
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerows(segments)
    print(f"wrote {len(segments)} segments in 88 constellations; "
          f"{sum(hip == 0 for hip in matches.values())} unmatched vertices", file=sys.stderr)


if __name__ == "__main__":
    main()
