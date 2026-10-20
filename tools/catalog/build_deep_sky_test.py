#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
#
# SPDX-License-Identifier: MIT
#
"""Host tests for the OpenNGC converter, using synthetic rows only."""

import csv
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))
import build_deep_sky as catalog


FIELDS = ("Name", "Type", "RA", "Dec", "M", "V-Mag", "MajAx", "MinAx",
          "Const", "Common names")


def row(name, messier="", *, type_="G", ra="05:35:17.3", dec="-05:23:28",
        mag="8.50", major="4.00", minor="2.00", common=""):
    return {"Name": name, "Type": type_, "RA": ra, "Dec": dec,
            "M": messier, "V-Mag": mag, "MajAx": major, "MinAx": minor,
            "Const": "Ori", "Common names": common}


def fixtures():
    ngc = [row(f"NGC{n:04d}", f"{n:03d}")
           for n in range(1, 111) if n not in {40, 45, 102}]
    ngc.append(row("NGC5866", common="Spindle Galaxy"))
    addendum = [row("M040", "040", type_="**"),
                row("Mel022", "045", type_="OCl", common="Pleiades"),
                row("M102", "101", type_="Dup")]
    return ngc, addendum


class DeepSkyTest(unittest.TestCase):
    def test_messier_requires_complete_unique_list(self):
        ngc, addendum = fixtures()
        selected, skipped = catalog.select_rows(ngc, addendum, scope="messier",
                                                  m102="ngc5866")
        self.assertEqual(len(selected), 110)
        self.assertEqual(skipped, 1)
        self.assertEqual({record[1] for record in selected},
                         {f"M{number}" for number in range(1, 111)})
        self.assertEqual(selected[39][0], "M040")
        self.assertEqual(selected[44][0], "Mel022")
        self.assertEqual(selected[101][0], "NGC5866")
        self.assertEqual(selected[101][1], "M102")
        self.assertEqual(selected[0][3:5], ("5.588139", "-5.391111"))
        selected, _ = catalog.select_rows(ngc, addendum, scope="messier", m102="omit")
        self.assertEqual(len(selected), 109)
        with self.assertRaisesRegex(ValueError, "M45"):
            catalog.select_rows(ngc, addendum[:1], scope="messier", m102="ngc5866")
        with self.assertRaisesRegex(ValueError, "M1 occurs"):
            catalog.select_rows(ngc + [row("NGC9999", "001")], addendum,
                                scope="messier", m102="ngc5866")

    def test_all_and_coordinates(self):
        ngc, addendum = fixtures()
        ngc += [row("NGC8888", type_="NonEx"), row("NGC7777", ra="bad")]
        selected, skipped = catalog.select_rows(ngc, addendum, scope="all", m102=None)
        self.assertEqual(len(selected), 110)
        self.assertEqual(skipped, 3)
        self.assertEqual(catalog.sexagesimal("+90:00:00", hours=False), 90)
        for coord in ("24:00:00", "12:60:00", "NaN:00:00"):
            with self.assertRaises(ValueError):
                catalog.sexagesimal(coord, hours=True)

    def test_bright_scope(self):
        ngc, addendum = fixtures()
        ngc.extend([
            row("NGC9991", mag="9.90"),
            row("NGC9992", mag="10.10"),
            row("NGC9993", mag="12.00", common="Named Nebula"),
            row("NGC9994", mag="5.00", type_="*", common="Bright Star"),
        ])
        selected, _ = catalog.select_bright(ngc, addendum,
                                             m102="ngc5866", mag_limit=10.0)
        self.assertEqual(len(selected), 112)
        self.assertEqual([record[0] for record in selected[-2:]],
                         ["NGC9991", "NGC9993"])
        self.assertEqual(selected[0][1], "M1")
        self.assertEqual(selected[109][1], "M110")

    def test_local_cli_and_provenance(self):
        ngc, addendum = fixtures()
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for name, rows in (("NGC.csv", ngc), ("addendum.csv", addendum)):
                with (root / name).open("w", encoding="utf-8", newline="") as out:
                    writer = csv.DictWriter(out, fieldnames=FIELDS, delimiter=";")
                    writer.writeheader()
                    writer.writerows(rows)
            output = root / "deep_sky.tsv"
            args = [sys.executable, str(Path(catalog.__file__)),
                    "--scope", "messier", "--m102", "ngc5866",
                    "--ngc", str(root / "NGC.csv"),
                    "--addendum", str(root / "addendum.csv"),
                    "--source-commit", catalog.COMMIT, "--out", str(output)]
            result = subprocess.run(args, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            lines = output.read_text(encoding="utf-8").splitlines()
            self.assertIn(catalog.COMMIT, lines[1])
            self.assertEqual(len(lines), 114)  # four headers plus 110 objects
            bad = subprocess.run(args[:args.index("--source-commit")] +
                                 ["--out", str(output)], capture_output=True, text=True)
            self.assertNotEqual(bad.returncode, 0)


if __name__ == "__main__":
    unittest.main()
