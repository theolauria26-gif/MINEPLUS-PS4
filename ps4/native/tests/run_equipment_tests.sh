#!/usr/bin/env sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
OUT="${TMPDIR:-/tmp}/mineplus-equipment-tests"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror "$ROOT/equipment.cpp" "$ROOT/tests/equipment_tests.cpp" -o "$OUT"
"$OUT"
rm -f "$OUT"
