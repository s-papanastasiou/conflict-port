#!/usr/bin/env python3
"""Decode CONFLICT data files to PNG, following the EGA driver in segment 1f39.
usage: assets.py GAMEDIR OUTDIR"""
import os, struct, sys
from PIL import Image

# Default EGA 16-colour palette (mode 10h with BIOS default palette registers).
EGA = [(0,0,0),(0,0,170),(0,170,0),(0,170,170),(170,0,0),(170,0,170),(170,85,0),(170,170,170),
       (85,85,85),(85,85,255),(85,255,85),(85,255,255),(255,85,85),(255,85,255),(255,255,85),(255,255,255)]
W, H = 640, 350

def decode_mp(data):
    """1f39:0937 — rows bottom-up; runs of [colour u8][x_end u16][y u16]; fill from cursor to (x_end-1, y)."""
    px = bytearray(W * H)
    si, cx, cy = 0, 0, 0
    while si + 5 <= len(data):
        col, xe, ye = data[si], *struct.unpack_from('<HH', data, si + 1); si += 5
        while cy <= 0x15d and cy != ye:          # finish current row, advance
            px[cy*W + cx: cy*W + W] = bytes([col]) * (W - cx); cy += 1; cx = 0
        if cy > 0x15d: break
        px[cy*W + cx: cy*W + xe] = bytes([col]) * max(0, xe - cx); cx = xe
        if ye == 0x15d and xe == 0x27f:
            break
    # 1f39:03d7 draws span rows at 0x15d - y: files are stored bottom-up.
    flipped = bytearray(W * H)
    for y in range(H):
        flipped[(H-1-y)*W:(H-y)*W] = px[y*W:(y+1)*W]
    return flipped, si

def img_from_indices(px, w, h):
    im = Image.new('P', (w, h)); im.putpalette(sum(EGA, ())); im.putdata(px); return im

def decode_dat(data):
    """1f39:0256 — 160x200 1bpp, 20 bytes/row, ANDed onto screen (0 bit = black)."""
    im = Image.new('1', (160, 200))
    im.putdata([255 if (data[y*20 + x//8] >> (7 - x % 8)) & 1 else 0 for y in range(200) for x in range(160)])
    return im

def font_sheet(data, rec, rows, bpr):
    """Fonts: rec bytes/char; rows x bpr bitmap bytes then width byte(s)."""
    n = len(data) // rec; cw = bpr * 8 + 2
    im = Image.new('L', (16 * cw, ((n + 15) // 16) * (rows + 2)), 64)
    for i in range(n):
        c = data[i*rec:(i+1)*rec]
        ox, oy = (i % 16) * cw, (i // 16) * (rows + 2)
        for r in range(rows):
            for b in range(bpr):
                byte = c[r*bpr + b]
                for k in range(8):
                    if (byte >> (7-k)) & 1: im.putpixel((ox + b*8 + k, oy + r), 255)
    return im, n

def main(src, out):
    os.makedirs(out, exist_ok=True)
    for f in sorted(os.listdir(src)):
        p, ext = os.path.join(src, f), f.rsplit('.', 1)[-1].upper()
        data = open(p, 'rb').read()
        if ext == 'MP':
            px, used = decode_mp(data)
            img_from_indices(px, W, H).save(os.path.join(out, f + '.png'))
            print(f'{f}: {len(data)} bytes, consumed {used}')
        elif ext == 'DAT':
            decode_dat(data).save(os.path.join(out, f + '.png'))
        elif ext == 'FNT':
            rec, rows, bpr = (9, 8, 1) if len(data) % 9 == 0 and f.upper() in ('ETECH.FNT','EXTRA.FNT','SGUARD.FNT','SINI.FNT') else (36, 16, 2)
            im, n = font_sheet(data, rec, rows, bpr)
            im.resize((im.width*3, im.height*3), Image.NEAREST).save(os.path.join(out, f + '.png'))
            print(f'{f}: {len(data)} bytes = {n} chars x {rec} (+{len(data)%rec} spare)')

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
