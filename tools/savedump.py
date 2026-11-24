#!/usr/bin/env python3
"""Decode a CONFLICT .SAV file. Layout derived from the save routine at 1000:25b3
(a sequence of fwrite() calls of globals in DGROUP; addresses are DS offsets)."""
import struct, sys

NATIONS = ["Egypt", "Iran", "Iraq", "Jordan", "Lebanon", "Libya", "Syria", "Israel"]

# (DS offset, element size, count, provisional name)
LAYOUT = [
    (0x92f0, 2, 8, "g92f0"), (0x9300, 2, 8, "g9300"), (0x9310, 2, 64, "g9310_8x8"),
    (0x9390, 2, 8, "g9390"), (0x93a0, 2, 640, "g93a0"), (0x98a0, 2, 8, "g98a0"),
    (0x98b0, 2, 9, "g98b0"), (0x98c2, 2, 8, "g98c2"), (0x98d2, 2, 950, "g98d2"),
    (0xa03e, 2, 8, "ga03e"), (0xa04e, 2, 8, "ga04e"), (0xa05e, 2, 64, "ga05e_8x8"),
    (0xa0de, 2, 8, "ga0de"), (0xa0ee, 2, 8, "ga0ee"), (0xa0fe, 2, 8, "ga0fe"),
    (0xa10e, 2, 8, "ga10e"), (0xa11e, 2, 8, "ga11e"), (0xa12e, 2, 8, "ga12e"),
    (0xa13e, 2, 8, "ga13e"), (0xa14e, 2, 8, "ga14e"), (0xa15e, 2, 8, "ga15e"),
    (0xa16e, 2, 25, "ga16e"), (0xa1a0, 2, 25, "ga1a0"), (0xa1d2, 2, 8, "ga1d2"),
    (0xa1e2, 2, 8, "ga1e2"), (0xa4ae, 1, 2500, "ga4ae_bytes"), (0xae72, 1, 1000, "gae72_bytes"),
] + [(a, 2, 1, f"g{a:04x}") for a in range(0xb25a, 0xb278, 2)]

def load(path):
    data = open(path, "rb").read()
    out, pos = {}, 0
    for addr, size, count, name in LAYOUT:
        n = size * count
        chunk = data[pos:pos + n]; pos += n
        out[name] = list(chunk) if size == 1 else list(struct.unpack(f"<{count}h", chunk))
    assert pos == len(data), f"size mismatch: layout {pos}, file {len(data)}"
    return out

if __name__ == "__main__":
    s = load(sys.argv[1])
    for addr, size, count, name in LAYOUT:
        v = s[name]
        if count == 8:
            print(f"{name:14s}", " ".join(f"{x:6d}" for x in v))
        elif count == 64:
            print(name); [print("   ", " ".join(f"{x:6d}" for x in v[i*8:i*8+8])) for i in range(8)]
        elif count == 1:
            print(f"{name:14s} {v[0]}")
        else:
            nz = [(i, x) for i, x in enumerate(v) if x]
            print(f"{name:14s} [{count}] nonzero={len(nz)} first: {nz[:12]}")
