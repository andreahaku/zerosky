#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
#
# SPDX-License-Identifier: MIT
#
"""Build a compact deep-sky TSV from a fixed OpenNGC revision.

The generated dataset is a derivative of OpenNGC by Mattia Verga and remains
CC BY-SA 4.0. It is separate from ZeroSky's MIT application code. Columns:

    source_id  messier  type  ra_hours  dec_deg  v_mag  major_arcmin
    minor_arcmin  constellation  common_names

Coordinates are J2000. Empty numeric cells mean unknown, not zero. `messier`
contains M1...M110 when present. OpenNGC treats M102 as a duplicate of M101;
--m102 ngc5866 explicitly adopts the common NGC5866 identification instead.

Choose the scope explicitly. The Messier mode requires all 110 identifiers;
the complete mode includes all physical objects with coordinates in NGC.csv
and addendum.csv, excluding catalog entries marked duplicate or nonexistent.
The bright mode, the one ZeroSky ships, keeps every Messier object plus the
objects with a common name or brighter than --mag (default 10).
"""

import argparse
import csv
import io
import math
from pathlib import Path
import re
import sys
import urllib.request


COMMIT = "36cb178a0f69dba8bfc03a99c10512831edf1c6b"
SOURCE = f"https://raw.githubusercontent.com/mattiaverga/OpenNGC/{COMMIT}/database_files"
SOURCE_NAMES = ("NGC.csv", "addendum.csv")
SKIP_TYPES = {"Dup", "NonEx"}
# Single and double stars, novae and unclassified entries: the star catalog
# already has the stars, so the bright scope leaves these out.
NOT_DEEP_SKY = {"*", "**", "*Ass", "Nova", "Other"}
OUTPUT_COLUMNS = (
    "source_id", "messier", "type", "ra_hours", "dec_deg", "v_mag",
    "major_arcmin", "minor_arcmin", "constellation", "common_names",
)


def read_csv(path_or_url):
    if path_or_url.startswith("https://"):
        with urllib.request.urlopen(path_or_url, timeout=120) as response:
            source = response.read().decode("utf-8-sig")
    else:
        source = Path(path_or_url).read_text(encoding="utf-8-sig")
    reader = csv.DictReader(io.StringIO(source), delimiter=";")
    if not reader.fieldnames or not {"Name", "Type", "RA", "Dec", "M"}.issubset(reader.fieldnames):
        raise ValueError(f"unexpected OpenNGC columns in {path_or_url}")
    return list(reader)


def sexagesimal(value, *, hours):
    parts = value.split(":")
    if len(parts) != 3:
        raise ValueError(f"bad coordinate: {value!r}")
    first = parts[0]
    sign = -1 if first.startswith("-") else 1
    units = int(first.lstrip("+-"))
    minutes = int(parts[1])
    seconds = float(parts[2])
    if not all(math.isfinite(n) for n in (units, minutes, seconds)):
        raise ValueError(f"non-finite coordinate: {value!r}")
    if not (0 <= minutes < 60 and 0 <= seconds < 60):
        raise ValueError(f"bad minutes/seconds: {value!r}")
    result = sign * (units + minutes / 60 + seconds / 3600)
    if hours and (sign < 0 or not 0 <= result < 24):
        raise ValueError(f"RA outside 0..24h: {value!r}")
    if not hours and (units > 90 or abs(result) > 90):
        raise ValueError(f"Dec outside -90..90deg: {value!r}")
    return result


def optional_number(value, *, positive=False):
    if not value:
        return ""
    number = float(value)
    if not math.isfinite(number) or (positive and number <= 0):
        raise ValueError(f"invalid number: {value!r}")
    return f"{number:.2f}"


def clean(value):
    return " ".join((value or "").split())


def messier_number(raw):
    if not raw:
        return None
    if not re.fullmatch(r"\d{1,3}", raw):
        raise ValueError(f"invalid Messier field: {raw!r}")
    number = int(raw)
    if not 1 <= number <= 110:
        raise ValueError(f"Messier number outside 1..110: {raw!r}")
    return number


def select_rows(ngc_rows, addendum_rows, *, scope, m102):
    if scope not in {"messier", "all"}:
        raise ValueError("scope must be messier or all")
    if scope == "messier" and m102 not in {"ngc5866", "omit"}:
        raise ValueError("Messier mode needs an explicit M102 convention")
    selected = []
    by_messier = {}
    seen_source_ids = set()
    skipped = 0
    for row in [*ngc_rows, *addendum_rows]:
        source_id = clean(row.get("Name"))
        if not source_id or source_id in seen_source_ids:
            raise ValueError(f"missing or duplicate source ID: {source_id!r}")
        seen_source_ids.add(source_id)
        if row.get("Type") in SKIP_TYPES or not row.get("RA") or not row.get("Dec"):
            skipped += 1
            continue
        number = messier_number((row.get("M") or "").strip())
        if scope == "messier" and number is None and not (
            m102 == "ngc5866" and source_id == "NGC5866"
        ):
            continue
        if scope == "messier" and source_id == "NGC5866" and m102 == "ngc5866":
            number = 102
        if number is not None and scope == "messier":
            if number in by_messier:
                raise ValueError(f"M{number} occurs in {by_messier[number]} and {source_id}")
            by_messier[number] = source_id
        try:
            ra = sexagesimal(row["RA"], hours=True)
            dec = sexagesimal(row["Dec"], hours=False)
            values = (
                source_id,
                f"M{number}" if number is not None else "",
                clean(row.get("Type")),
                f"{ra:.6f}",
                f"{dec:.6f}",
                optional_number(row.get("V-Mag")),
                optional_number(row.get("MajAx"), positive=True),
                optional_number(row.get("MinAx"), positive=True),
                clean(row.get("Const")),
                clean(row.get("Common names")),
            )
        except (ValueError, TypeError) as exc:
            if scope == "messier":
                raise ValueError(f"invalid data for {source_id}") from exc
            skipped += 1
            continue
        selected.append(values)

    if scope == "messier":
        expected = set(range(1, 111)) - ({102} if m102 == "omit" else set())
        missing = sorted(expected - by_messier.keys())
        if missing:
            raise ValueError("missing Messier identifiers: " + ", ".join(f"M{n}" for n in missing))
        selected.sort(key=lambda record: int(record[1][1:]))
    else:
        selected.sort(key=lambda record: record[0])
    return selected, skipped


def select_bright(ngc_rows, addendum_rows, *, m102, mag_limit):
    """Messier first (by number), then named or bright objects (by magnitude)."""
    messier, _ = select_rows(ngc_rows, addendum_rows, scope="messier", m102=m102)
    complete, skipped = select_rows(ngc_rows, addendum_rows, scope="all", m102=None)
    taken = {record[0] for record in messier}

    def magnitude(record):
        return float(record[5]) if record[5] else math.inf

    extra = [record for record in complete if record[0] not in taken
             and record[2] not in NOT_DEEP_SKY
             and (record[9] or magnitude(record) <= mag_limit)]
    extra.sort(key=lambda record: (magnitude(record), record[0]))
    return messier + extra, skipped


def write_tsv(path, selected, *, scope, m102, source_commit):
    destination = Path(path)
    destination.parent.mkdir(parents=True, exist_ok=True)
    with destination.open("w", encoding="utf-8", newline="\n") as out:
        out.write("# OpenNGC by Mattia Verga, CC BY-SA 4.0; derivative selection/format for ZeroSky.\n")
        out.write(f"# Source: https://github.com/mattiaverga/OpenNGC/tree/{source_commit}\n")
        out.write(f"# Scope: {scope}; M102: {m102 or 'as in source'}; J2000.\n")
        out.write("# " + "\t".join(OUTPUT_COLUMNS) + "\n")
        for values in selected:
            out.write("\t".join(values) + "\n")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--scope", choices=("messier", "all", "bright"), required=True)
    parser.add_argument("--mag", type=float, default=10.0,
                        help="with --scope bright: faintest unnamed non-Messier object kept")
    parser.add_argument("--m102", choices=("ngc5866", "omit"),
                        help="required with --scope messier")
    parser.add_argument("--ngc", help="local NGC.csv instead of pinned download")
    parser.add_argument("--addendum", help="local addendum.csv instead of pinned download")
    parser.add_argument("--source-commit", help="OpenNGC commit of local CSVs (required with --ngc)")
    parser.add_argument("--out", default="assets/sky/deep_sky.tsv")
    args = parser.parse_args(argv)
    if bool(args.ngc) != bool(args.addendum):
        parser.error("--ngc and --addendum must be supplied together")
    if args.ngc and not args.source_commit:
        parser.error("local CSVs require --source-commit for provenance")
    if not args.ngc and args.source_commit:
        parser.error("--source-commit applies only to local CSVs")
    if args.source_commit and not re.fullmatch(r"[0-9a-f]{40}", args.source_commit):
        parser.error("--source-commit must be a full 40-digit lowercase Git SHA")
    if args.scope in ("messier", "bright") and args.m102 is None:
        parser.error(f"--scope {args.scope} requires --m102 ngc5866 or --m102 omit")
    if args.scope == "all" and args.m102 is not None:
        parser.error("--m102 applies only to --scope messier")

    paths = (args.ngc, args.addendum) if args.ngc else tuple(
        f"{SOURCE}/{name}" for name in SOURCE_NAMES
    )
    ngc_rows, addendum_rows = (read_csv(path) for path in paths)
    if args.scope == "bright":
        selected, skipped = select_bright(ngc_rows, addendum_rows,
                                          m102=args.m102, mag_limit=args.mag)
    else:
        selected, skipped = select_rows(ngc_rows, addendum_rows,
                                        scope=args.scope, m102=args.m102)
    write_tsv(args.out, selected, scope=args.scope, m102=args.m102,
              source_commit=args.source_commit or COMMIT)
    print(f"wrote {len(selected)} objects; skipped {skipped} invalid/duplicate entries", file=sys.stderr)


if __name__ == "__main__":
    main()
