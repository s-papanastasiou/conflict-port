#!/usr/bin/env python3
"""Split decompiled.c into per-segment-group files (game / driver / runtime)."""
import re, sys, os
d = sys.argv[1]
src = open(os.path.join(d, 'decompiled.c')).read()
# Ghidra re-assembles doubles that passed through AX:BX:CX:DX piecewise; collapse the identity.
ident = re.compile(r'\(double\)\(\(qword\)(\w+) & 0xffff000000000000\s*\|\s*\(\(qword\)\1 >> 0x20 & 0xffff\) << 0x20\s*\|\s*'
                   r'\(\(qword\)\1 >> 0x10 & 0xffff\) << 0x10\s*\|\s*\(qword\)\1 & 0xffff\)')
src, n = ident.subn(lambda m: m.group(1), src)
print('collapsed', n, 'double re-pack expressions')
open(os.path.join(d, 'decompiled_clean.c'), 'w').write(src)
groups = {}
for p in re.split(r'(?m)^(?=// ===== )', src):
    m = re.match(r'// ===== \S+ @ ([0-9a-f]{4}):', p)
    if not m: continue
    seg = m.group(1)
    g = 'game_' + seg if seg in ('1000', '1b92', '1d93') else ('driver_1f39' if seg == '1f39' else 'runtime')
    groups.setdefault(g, []).append(p)
for g, ps in groups.items():
    open(os.path.join(d, g + '.c'), 'w').write(''.join(ps))
    print(g, len(ps), 'functions')
