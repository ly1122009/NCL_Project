#!/usr/bin/env bash
# Hardware-in-the-loop smoke test for the V4L2 capture demos.
#
# Unlike tests/test_v4l2_capture_thread.cpp (fast, no hardware needed,
# checks internal logic through a fake device), this script runs the REAL
# binaries against a REAL camera and checks the end-to-end result: right
# byte count for the negotiated format, and a sanity check that the frame
# isn't suspiciously flat. It only proves "the whole pipeline still works",
# not any particular internal behavior — that's what makes it a smoke
# test rather than a unit test. Meant to run on a machine with a UVC
# webcam attached (the Pi), not as part of the regular `ctest` suite.
#
# Usage: scripts/smoke_test_v4l2.sh [build_dir] [device] [width] [height]
set -euo pipefail

BUILD_DIR="${1:-build/manual}"
DEVICE="${2:-/dev/video0}"
WIDTH="${3:-640}"
HEIGHT="${4:-480}"
EXPECTED_BYTES=$((WIDTH * HEIGHT * 2)) # YUYV = 2 bytes/pixel

WORKDIR="$(mktemp -d)"
trap 'rm -rf "$WORKDIR"' EXIT

pass=0
fail=0

check_capture() {
  local name="$1"
  local binary="$2"
  local outfile="$WORKDIR/${name}.raw"

  echo "== ${name} =="

  if [ ! -x "$binary" ]; then
    echo "FAIL: ${name} — binary not found at ${binary} (build it first)"
    fail=$((fail + 1))
    return
  fi

  if ! "$binary" "$DEVICE" "$outfile" >"$WORKDIR/${name}.log" 2>&1; then
    echo "FAIL: ${name} exited non-zero"
    tail -20 "$WORKDIR/${name}.log"
    fail=$((fail + 1))
    return
  fi

  if [ ! -s "$outfile" ]; then
    echo "FAIL: ${name} produced no output file"
    fail=$((fail + 1))
    return
  fi

  local actual_bytes
  actual_bytes=$(stat -c '%s' "$outfile")
  if [ "$actual_bytes" -ne "$EXPECTED_BYTES" ]; then
    echo "FAIL: ${name} wrote ${actual_bytes} bytes, expected ${EXPECTED_BYTES} (${WIDTH}x${HEIGHT} YUYV)"
    fail=$((fail + 1))
    return
  fi

  local distinct_bytes
  distinct_bytes=$(od -An -tu1 "$outfile" | tr -s ' ' '\n' | sort -un | wc -l)
  if [ "$distinct_bytes" -lt 8 ]; then
    echo "WARN: ${name} frame has only ${distinct_bytes} distinct byte values — camera may be covered or the room may be dark (not a script failure by itself, just worth a look)"
  fi

  echo "PASS: ${name} (${actual_bytes} bytes, ${distinct_bytes} distinct byte values)"
  pass=$((pass + 1))
}

check_capture "v4l2_capture_demo" "${BUILD_DIR}/v4l2_capture_demo"
check_capture "v4l2_capture_threaded_demo" "${BUILD_DIR}/v4l2_capture_threaded_demo"

echo
echo "${pass} passed, ${fail} failed"
[ "$fail" -eq 0 ]
