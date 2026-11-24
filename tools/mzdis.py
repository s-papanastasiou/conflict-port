#!/usr/bin/env python3
"""Disassemble a function of Conflict.exe by Ghidra seg:off address (Ghidra adds 0x1000 to segments).
usage: mzdis.py EXE SEG:OFF [len]   (len defaults to size in functions.tsv if given via env FUNCS)"""
import sys, struct, subprocess, tempfile, os
exe, addr = sys.argv[1], sys.argv[2]
n = int(sys.argv[3], 0) if len(sys.argv) > 3 else 256
d = open(exe, 'rb').read()
hdr = struct.unpack('<H', d[8:10])[0] * 16
seg, off = (int(x, 16) for x in addr.split(':'))
lin = (seg - 0x1000) * 16 + off
with tempfile.NamedTemporaryFile(delete=False) as t:
    t.write(d[hdr + lin: hdr + lin + n]); name = t.name
out = subprocess.run(['ndisasm', '-b16', '-o', str(off), name], capture_output=True, text=True).stdout
os.unlink(name)
print(out, end='')
