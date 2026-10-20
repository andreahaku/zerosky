#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
#
# SPDX-License-Identifier: MIT
#
"""Build assets/sky/stars.tsv, the star catalog behind the sky view.

Source: the HYG database v4.1 by David Nash (https://github.com/astronexus/HYG-Database),
licensed CC BY-SA 4.0. The output is a derived work under the same license. One line per
star brighter than the magnitude limit, brightest first, so a renderer can stop early:

    hip<TAB>ra_hours<TAB>dec_deg<TAB>mag<TAB>bv<TAB>bayer<TAB>con<TAB>name

ra and dec are J2000 (epoch and equinox 2000.0). bv is the B-V colour index, empty when
unknown. bayer is the HYG Bayer letter (Alp, Bet, ...) or the Flamsteed number, con the
IAU constellation abbreviation, name the proper name. Empty fields stay empty.

    python3 tools/catalog/build_stars.py                 # download + build
    python3 tools/catalog/build_stars.py --csv FILE      # build from a local CSV
"""
import argparse
import csv
import io
import sys
import urllib.request

COMMIT = "c7f7f883fe678cc7680169a50ccd7dcc49b060ce"
URL = ("https://raw.githubusercontent.com/astronexus/HYG-Database/"
       f"{COMMIT}/hyg/CURRENT/hygdata_v41.csv")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--out", default="assets/sky/stars.tsv")
    ap.add_argument("--csv", help="local hygdata_v41.csv instead of the download")
    ap.add_argument("--mag", type=float, default=6.0, help="faintest magnitude kept")
    args = ap.parse_args()

    if args.csv:
        with open(args.csv, encoding="utf-8") as f:
            text = f.read()
    else:
        with urllib.request.urlopen(URL, timeout=120) as r:
            text = r.read().decode("utf-8")

    stars = []
    for row in csv.DictReader(io.StringIO(text)):
        if row["id"] == "0":  # the Sun
            continue
        try:
            mag = float(row["mag"])
        except ValueError:
            continue
        if mag > args.mag:
            continue
        label = row["bayer"] or row["flam"]
        stars.append((mag, row["hip"], float(row["ra"]), float(row["dec"]), row["ci"],
                      label, row["con"], row["proper"]))
    stars.sort(key=lambda s: s[0])

    with open(args.out, "w", encoding="utf-8", newline="\n") as out:
        out.write(f"# HYG v4.1 ({COMMIT[:7]}), CC BY-SA 4.0, David Nash, "
                  f"astronexus.com. Stars to mag {args.mag:g}, J2000.\n")
        for mag, hip, ra, dec, bv, label, con, name in stars:
            out.write(f"{hip}\t{ra:.6f}\t{dec:.5f}\t{mag:.2f}\t{bv}\t{label}\t{con}\t{name}\n")
    print(f"wrote {len(stars)} stars to {args.out}", file=sys.stderr)


if __name__ == "__main__":
    main()
