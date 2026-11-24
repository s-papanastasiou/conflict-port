#!/bin/sh
# Re-import Conflict.exe into a fresh Ghidra project, apply symbols, export decompilation.
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
mkdir -p "$ROOT/ghidra" "$ROOT/re/ghidra_export"
flatpak run --command=/app/lib/ghidra/support/analyzeHeadless org.ghidra_sre.Ghidra \
  "$ROOT/ghidra" conflict -import "$ROOT/conflict/Conflict.exe" -overwrite \
  -scriptPath "$ROOT/tools/ghidra_scripts" \
  -preScript SetupDGroup.java -preScript ZortechFloat.java \
  -postScript ApplySymbols.java "$ROOT/re/symbols.tsv" \
  -postScript ExportAll.java "$ROOT/re/ghidra_export" > "$ROOT/ghidra/headless.log" 2>&1
grep -E "Applied|Exported|ERROR|no function" "$ROOT/ghidra/headless.log" || true
python3 -I "$ROOT/tools/split_export.py" "$ROOT/re/ghidra_export"
