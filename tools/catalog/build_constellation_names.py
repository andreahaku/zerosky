#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
# SPDX-License-Identifier: MIT
"""Convert pinned d3-celestial Latin names and label points to a compact TSV.

    python3 tools/catalog/build_constellation_names.py
    python3 tools/catalog/build_constellation_names.py --json /path/to/constellations.json
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
       f"{COMMIT}/data/constellations.json")
SHA256 = "ab4ae692027cbc042c0d6791a84456a65eb7c55656107fd00c58ff6e55d4d8b2"


def build(source: bytes) -> list[tuple[str, str, int, str, str]]:
    if hashlib.sha256(source).hexdigest() != SHA256:
        raise ValueError("source SHA-256 differs from the pinned d3-celestial file")
    data = json.loads(source)
    if data.get("type") != "FeatureCollection":
        raise ValueError("source is not a GeoJSON FeatureCollection")
    features = data.get("features", [])
    if len(features) != 89 or len({f["id"] for f in features}) != 88:
        raise ValueError("expected 89 label points for 88 constellations")
    rows = []
    for feature in features:
        abbr = feature["id"]
        props = feature["properties"]
        name = props["la"]  # Latin IAU name, including both parts of Serpens.
        rank = int(props["rank"])
        geometry = feature["geometry"]
        if (len(abbr) != 3 or not abbr.isalpha() or not name or "\t" in name or
                geometry["type"] != "Point" or rank not in (1, 2, 3)):
            raise ValueError(f"malformed constellation label: {abbr}")
        lon, dec = geometry["coordinates"]
        if not math.isfinite(lon) or not math.isfinite(dec) or not -90 <= dec <= 90:
            raise ValueError(f"invalid coordinates for {abbr}")
        rows.append((abbr, name, rank, f"{(lon % 360) / 15:.6f}", f"{dec:.5f}"))
    return rows


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--json", help="local copy of the pinned constellations.json")
    ap.add_argument("--out", default="assets/sky/constellation_names.tsv")
    args = ap.parse_args()
    if args.json:
        source = Path(args.json).read_bytes()
    else:
        with urllib.request.urlopen(URL, timeout=120) as response:
            source = response.read()
    try:
        rows = build(source)
    except (ValueError, KeyError, TypeError, IndexError) as exc:
        ap.error(str(exc))
    with open(args.out, "w", encoding="utf-8", newline="") as stream:
        stream.write(f"# Latin constellation names and J2000 label positions from d3-celestial {COMMIT} by Olaf Frohn.\n")
        stream.write(f"# Source: {URL}\n")
        stream.write(f"# Source SHA-256: {SHA256}\n")
        stream.write("# d3-celestial: BSD-3-Clause; adapted IAU / Sky & Telescope charts: CC BY 4.0.\n")
        stream.write("# Credits: IAU, Sky & Telescope, Roger Sinnott, Rick Fienberg, Alan MacRobert, Olaf Frohn.\n")
        stream.write("# Selected Latin names and GeoJSON Point positions; normalized RA to J2000 hours.\n")
        stream.write("# Serpens has two source points, Caput and Cauda, with the same Latin name.\n")
        stream.write("# abbr<TAB>latin_name<TAB>rank<TAB>ra_j2000_hours<TAB>dec_j2000_deg\n")
        csv.writer(stream, delimiter="\t", lineterminator="\n").writerows(rows)
    print(f"wrote {len(rows)} labels for 88 constellations", file=sys.stderr)


if __name__ == "__main__":
    main()
