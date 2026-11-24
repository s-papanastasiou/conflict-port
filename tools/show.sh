#!/bin/sh
# Print decompiled functions by name/address without their local declaration block.
# usage: tools/show.sh FUN_1000_0a7c [more...]
ROOT=$(cd "$(dirname "$0")/.." && pwd)
for f in "$@"; do
  awk -v f="$f" '
    $0 ~ "^// ===== " f " " {on=1; decl=0}
    on && /^\/\/ ===== / && $0 !~ f" " {on=0}
    on {
      if ($0 == "{") {print; decl=1; next}
      if (decl) {
        if ($0 ~ /^ *$/) {decl=0; next}
        if ($0 ~ /^  [A-Za-z_][A-Za-z0-9_ ]*[ *]+[A-Za-z_][A-Za-z0-9_]*(\[[0-9a-fx]+\])*;$/) next
        decl=0
      }
      if ($0 ~ /WARNING: Function: .* replaced with injection/) next
      if ($0 ~ /^ *$/) next
      print
    }' "$ROOT/re/ghidra_export/decompiled_clean.c"
done
