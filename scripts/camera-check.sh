#!/usr/bin/env bash
#
# SPDX-FileCopyrightText: 2026 One Small Step Apps Ltd
#
# SPDX-License-Identifier: MIT
#
# Capture the same raw stream used by RpicamSource and inspect its framing.
# Run on the CardputerZero: scripts/camera-check.sh [output-directory]
set -euo pipefail

if (( $# > 1 )); then
    echo "Usage: $0 [output-directory]" >&2
    exit 2
fi

rpicam_vid="${ZEROSKY_RPICAM_VID:-rpicam-vid}"
if ! command -v "$rpicam_vid" >/dev/null 2>&1; then
    echo "Camera command not found: $rpicam_vid" >&2
    exit 1
fi
if ! command -v python3 >/dev/null 2>&1; then
    echo "python3 is required to write the inspection PNG" >&2
    exit 1
fi

output_dir="${1:-$(mktemp -d /tmp/zerosky-camera-check.XXXXXX)}"
mkdir -p "$output_dir"

"$rpicam_vid" --help > "$output_dir/rpicam-help.txt" 2>&1
echo "Capturing 2 seconds of 320 x 176 YUV420 at 10 fps..."
if ! "$rpicam_vid" --nopreview --timeout 2000 --codec yuv420 \
    --width 320 --height 176 --framerate 10 --rotation 180 --flush \
    --output - > "$output_dir/capture.yuv" 2> "$output_dir/rpicam-stderr.txt"; then
    echo "Capture failed. See $output_dir/rpicam-stderr.txt" >&2
    exit 1
fi

frame_bytes=$((320 * 176 * 3 / 2))
byte_count=$(wc -c < "$output_dir/capture.yuv")
if (( byte_count == 0 || byte_count % frame_bytes != 0 )); then
    echo "Unexpected stream size: $byte_count bytes; expected a nonzero multiple of $frame_bytes." >&2
    echo "Raw capture retained at $output_dir/capture.yuv" >&2
    exit 1
fi
frame_count=$((byte_count / frame_bytes))

# Convert the final frame, after auto-exposure has had time to settle. The
# 170-row PNG shows the exact centre crop used by RpicamSource; the full PNG
# makes incorrect padding or crop assumptions easier to spot.
python3 - "$output_dir/capture.yuv" "$output_dir" <<'PY'
import pathlib
import struct
import sys
import zlib

WIDTH = 320
HEIGHT = 176
FRAME_BYTES = WIDTH * HEIGHT * 3 // 2


def png_chunk(tag, payload):
    body = tag + payload
    return struct.pack('!I', len(payload)) + body + struct.pack('!I', zlib.crc32(body))


def write_png(path, rows):
    height = len(rows)
    image = b''.join(b'\x00' + row for row in rows)
    header = b'\x89PNG\r\n\x1a\n'
    header += png_chunk(b'IHDR', struct.pack('!IIBBBBB', WIDTH, height, 8, 2, 0, 0, 0))
    header += png_chunk(b'IDAT', zlib.compress(image))
    path.write_bytes(header + png_chunk(b'IEND', b''))


raw_path = pathlib.Path(sys.argv[1])
output_dir = pathlib.Path(sys.argv[2])
with raw_path.open('rb') as stream:
    stream.seek(-FRAME_BYTES, 2)
    frame = stream.read(FRAME_BYTES)
assert len(frame) == FRAME_BYTES
y_plane = frame[:WIDTH * HEIGHT]
u_plane = frame[WIDTH * HEIGHT:WIDTH * HEIGHT * 5 // 4]
v_plane = frame[WIDTH * HEIGHT * 5 // 4:]


def clip(value):
    return max(0, min(255, value))


rows = []
for y in range(HEIGHT):
    row = bytearray()
    for x in range(WIDTH):
        luma = y_plane[y * WIDTH + x]
        uv = (y // 2) * (WIDTH // 2) + x // 2
        u = u_plane[uv] - 128
        v = v_plane[uv] - 128
        # Same full-range BT.601 assumption as RpicamSource.
        row.extend((clip(luma + ((359 * v) >> 8)),
                    clip(luma - ((88 * u + 183 * v) >> 8)),
                    clip(luma + ((454 * u) >> 8))))
    rows.append(bytes(row))

write_png(output_dir / 'last-frame-full.png', rows)
write_png(output_dir / 'last-frame-cropped.png', rows[3:-3])
PY

echo "Frames: $frame_count ($byte_count bytes; $frame_bytes bytes/frame)"
echo "Inspect $output_dir/last-frame-cropped.png for colours and orientation."
echo "Full frame and rpicam diagnostics are also in $output_dir."
