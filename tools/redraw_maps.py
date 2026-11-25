#!/usr/bin/env python3
"""Draw the "redrawn" maps: new versions of the five CONFLICT maps (.MP) from Natural Earth data.

usage: tools/redraw_maps.py [--out assets/maps] [--preview DIR] [--compare GAMEDIR]

The maps keep the original layout (frame, scale, position of every country) and each country's
colour number, because the game draws markers at fixed positions on them and recolours countries
by colour inside fixed rectangles. Only the drawing is new: real coastlines and borders, thin border
lines, a ripple along the coasts, new labels and city names. Everything comes from:
  - Natural Earth 1:10m countries and lakes (public domain), downloaded on first use into
    ~/.cache/conflict-port/naturalearth
  - two pixel fonts drawn here (bold capitals for the country names, a smaller one for the other
    labels and the city names)
No original game data is used; --compare GAMEDIR only reports how well a new map lines up with the
original one. Needs Python 3 with numpy and Pillow.
"""
import argparse
import json
import os
import struct
import sys
import urllib.request

import numpy as np
from PIL import Image, ImageDraw

W, H = 640, 350
GREY, BLACK, SEA, RIPPLE, BORDER = 7, 0, 11, 3, 8

NE_URL = 'https://raw.githubusercontent.com/nvkelso/natural-earth-vector/master/geojson/{}.geojson'
NE_FILES = ('ne_10m_admin_0_countries', 'ne_10m_lakes')

# Territories drawn as part of another shape, and shapes whose borders are dotted.
MERGE = {'N. Cyprus': 'Cyprus', 'Dhekelia': 'Cyprus', 'Akrotiri': 'Cyprus',
         'Cyprus U.N. Buffer Zone': 'Cyprus'}
DOTTED = {'Gaza', 'West Bank'}  # Natural Earth's 'Palestine', split into its two parts

# Per map: frame (x0, y0, x1, y1 of the frame lines, line width), projection x = ax*lon + bx,
# y = ay*lat + by (fitted to the original map), colour of each country (as on the original map;
# other land is grey), labels (text, lon, lat, style) and cities (name, lon, lat, label side).
# Label styles: big (country, spaced capitals), small (country, capitals; small-white in white),
# sea and sea-vertical (blue, spaced).
MAPS = {
    'MDLEAST': dict(
        frame=(8, 7, 509, 307, 2), proj=(30.025, -901.37, -27.899, 1006.48),
        colours={'Cyprus': 2, 'Egypt': 10, 'Iran': 14, 'Iraq': 10, 'Israel': 12, 'Jordan': 2,
                 'Kuwait': 14, 'Lebanon': 10, 'West Bank': 2, 'Gaza': 12, 'Saudi Arabia': 14, 'Syria': 14,
                 'Turkey': 14},
        labels=[('EGYPT', 31.9, 27.3, 'big'), ('JORDAN', 37.3, 31.0, 'big'), ('SYRIA', 38.6, 35.2, 'big'),
                ('IRAQ', 43.4, 32.4, 'big'), ('SAUDI ARABIA', 41.6, 27.6, 'big'),
                ('ISRAEL', 34.95, 30.85, 'small-white'), ('LEBANON', 36.55, 34.5, 'small'),
                ('CYPRUS', 33.2, 35.6, 'small'),
                ('MEDITERRANEAN', 32.6, 33.6, 'sea'), ('SEA', 32.6, 33.25, 'sea'), ('RED SEA', 35.3, 25.4, 'sea')],
        cities=[('Cairo', 31.2357, 30.0444, 'r'), ('Amman', 35.9284, 31.9454, 'r'),
                ('Jerusalem', 35.2137, 31.7683, 'l'), ('Baghdad', 44.3661, 33.3152, 'r'),
                ('Damascus', 36.2913, 33.5138, 'r'), ('Beirut', 35.5018, 33.8938, 'l')]),
    'ISRAEL': dict(
        frame=(48, 7, 378, 338, 1), proj=(134.679, -4532.17, -114.253, 3822.65),
        colours={'Egypt': 10, 'Israel': 12, 'Jordan': 2, 'Lebanon': 10, 'West Bank': 2, 'Gaza': 12,
                 'Syria': 14},
        labels=[('ISRAEL', 34.75, 30.75, 'big'), ('JORDAN', 36.0, 31.35, 'big'), ('SYRIA', 36.05, 33.05, 'big'),
                ('LEBANON', 35.55, 33.35, 'small'), ('EGYPT', 34.15, 30.05, 'small'),
                ('MEDITERRANEAN', 34.55, 33.05, 'sea'), ('SEA', 34.55, 32.93, 'sea'),
                ('DEAD SEA', 35.49, 31.53, 'sea-vertical')],
        cities=[('Haifa', 34.9896, 32.7940, 'r'), ('Tel Aviv', 34.7818, 32.0853, 'l'),
                ('Jerusalem', 35.2137, 31.7683, 'l'), ('Amman', 35.9284, 31.9454, 'r'),
                ('Gaza', 34.4668, 31.5017, 'l'), ('Beersheba', 34.7913, 31.2520, 'r')]),
    'IRAQ': dict(
        frame=(8, 6, 458, 306, 2), proj=(35.743, -1345.58, -32.306, 1217.96),
        colours={'Iran': 14, 'Iraq': 10, 'Jordan': 2, 'Kuwait': 2, 'Saudi Arabia': 14, 'Syria': 14,
                 'Turkey': 2},
        labels=[('IRAQ', 42.6, 33.4, 'big'), ('IRAN', 48.0, 34.0, 'big'), ('SYRIA', 39.2, 35.3, 'small'),
                ('SAUDI ARABIA', 42.5, 29.4, 'big'), ('JORDAN', 38.9, 32.1, 'small'),
                ('TURKEY', 42.5, 37.2, 'small'), ('KUWAIT', 47.4, 29.05, 'small'), ('GULF', 49.7, 29.0, 'sea')],
        cities=[('Baghdad', 44.3661, 33.3152, 'r'), ('Basra', 47.7804, 30.5085, 'l'),
                ('Mosul', 43.1189, 36.3450, 'r'), ('Tehran', 51.4215, 35.6944, 'l')]),
    'LIBYA': dict(
        frame=(6, 4, 456, 304, 2), proj=(20.169, -203.90, -19.960, 710.06),
        colours={'Algeria': 14, 'Chad': 2, 'Egypt': 10, 'Libya': 14, 'Niger': 10, 'Sudan': 2,
                 'Tunisia': 10},
        labels=[('LIBYA', 17.5, 27.2, 'big'), ('EGYPT', 28.5, 26.0, 'big'), ('CHAD', 18.5, 21.2, 'small'),
                ('NIGER', 12.4, 22.0, 'small'), ('SUDAN', 28.0, 21.0, 'small'), ('ALGERIA', 11.3, 27.5, 'small'),
                ('MEDITERRANEAN SEA', 18.5, 34.3, 'sea')],
        cities=[('Tripoli', 13.1913, 32.8872, 'r'), ('Benghazi', 20.0868, 32.1194, 'r'),
                ('Cairo', 31.2357, 30.0444, 'l'), ('Alexandria', 29.9187, 31.2001, 'l')]),
    'SYRIA': dict(
        frame=(10, 11, 412, 262, 2), proj=(47.328, -1591.80, -52.922, 1969.46),
        colours={'Iraq': 10, 'Israel': 12, 'Jordan': 2, 'Lebanon': 10, 'West Bank': 2, 'Gaza': 12,
                 'Syria': 14, 'Turkey': 2},
        labels=[('SYRIA', 38.4, 35.2, 'big'), ('IRAQ', 41.45, 33.05, 'big'), ('JORDAN', 37.4, 32.6, 'small'),
                ('TURKEY', 37.2, 37.05, 'small'), ('LEBANON', 35.0, 34.1, 'small'), ('ISRAEL', 34.75, 32.75, 'small')],
        cities=[('Damascus', 36.2913, 33.5138, 'r'), ('Aleppo', 37.1612, 36.2021, 'r'),
                ('Homs', 36.7235, 34.7308, 'r'), ('Beirut', 35.5018, 33.8938, 'l'),
                ('Latakia', 35.7796, 35.5317, 'r')]),
}


# ---- data -----------------------------------------------------------------------------------

def natural_earth(cache):
    os.makedirs(cache, exist_ok=True)
    out = {}
    for name in NE_FILES:
        path = os.path.join(cache, name + '.geojson')
        if not os.path.exists(path):
            print(f'downloading {name} (Natural Earth, public domain)...', file=sys.stderr)
            urllib.request.urlretrieve(NE_URL.format(name), path + '.part')
            os.replace(path + '.part', path)
        out[name] = json.load(open(path, encoding='utf-8'))['features']
    return out


def polygons(geom):
    return [geom['coordinates']] if geom['type'] == 'Polygon' else geom['coordinates']


# ---- drawing ----------------------------------------------------------------------------------

class Map:
    def __init__(self, name, ne):
        self.name = name
        m = MAPS[name]
        self.m = m
        self.ax, self.bx, self.ay, self.by = m['proj']
        x0, y0, x1, y1, _ = m['frame']
        lon = sorted(((x0 - 40 - self.bx) / self.ax, (x1 + 40 - self.bx) / self.ax))
        lat = sorted(((y0 - 40 - self.by) / self.ay, (y1 + 40 - self.by) / self.ay))

        def near(ring):
            r = np.asarray(ring)
            return not (r[:, 0].max() < lon[0] or r[:, 0].min() > lon[1] or
                        r[:, 1].max() < lat[0] or r[:, 1].min() > lat[1])
        self.shapes = []  # (name, outer ring, holes)
        for f in ne['ne_10m_admin_0_countries']:
            n = MERGE.get(f['properties']['NAME'], f['properties']['NAME'])
            for p in polygons(f['geometry']):
                part = n
                if n == 'Palestine': part = 'Gaza' if min(x for x, _ in p[0]) < 34.6 else 'West Bank'
                if near(p[0]): self.shapes.append((part, p[0], p[1:]))
        self.lakes = [p[0] for f in ne['ne_10m_lakes'] for p in polygons(f['geometry']) if near(p[0])]
        self.names = sorted({n for n, _, _ in self.shapes})

    def xy(self, lon, lat):
        return self.ax * lon + self.bx, self.ay * lat + self.by

    def ids(self):
        """Shape index per pixel: 0 sea, -1 lake, i+1 = self.names[i]."""
        im = Image.new('I', (W, H), 0)
        d = ImageDraw.Draw(im)
        tr = lambda ring: [(self.ax * x + self.bx + 0.5, self.ay * y + self.by + 0.5) for x, y in ring]
        for n, outer, holes in self.shapes:
            d.polygon(tr(outer), fill=self.names.index(n) + 1)
            for h in holes: d.polygon(tr(h), fill=0)
        for ring in self.lakes: d.polygon(tr(ring), fill=-1)
        return np.array(im, dtype=np.int32)

    def draw(self):
        x0, y0, x1, y1, t = self.m['frame']
        ids = self.ids()
        land = ids > 0
        a = np.full((H, W), GREY, np.uint8)
        inside = np.zeros((H, W), bool); inside[y0 + t:y1 - t + 1, x0 + t:x1 - t + 1] = True
        fill = np.full((H, W), SEA, np.uint8)
        for i, n in enumerate(self.names):
            fill[ids == i + 1] = self.m['colours'].get(n, GREY)
        # ripple: a line in the sea 3 pixels off the coast (not in lakes)
        ripple = grow(land, 3) & ~grow(land, 2) & (ids == 0)
        fill[ripple] = RIPPLE
        # borders: thin dark line where two countries meet, dotted around DOTTED shapes
        border = np.zeros((H, W), bool)
        border[:, 1:] |= (ids[:, 1:] != ids[:, :-1]) & land[:, 1:] & land[:, :-1]
        border[1:] |= (ids[1:] != ids[:-1]) & land[1:] & land[:-1]
        dotted = np.zeros((H, W), bool)
        for n in DOTTED:
            if n in self.names: dotted |= grow(ids == self.names.index(n) + 1, 1)
        yy, xx = np.mgrid[0:H, 0:W]
        fill[border & ~dotted] = BORDER
        fill[border & dotted & ((xx + yy) % 3 == 0)] = BORDER
        a[inside] = fill[inside]
        # labels and cities (clipped to the frame)
        img = Image.fromarray(a, 'P')
        for text, lon, lat, style in self.m['labels']:
            x, y = self.xy(lon, lat)
            if style == 'big': pixel_text_into(img, (x, y), '  '.join(text), BLACK, big=True)
            elif style == 'small': pixel_text_into(img, (x, y), text, BLACK)
            elif style == 'small-white': pixel_text_into(img, (x, y), text, 15)
            elif style == 'sea-vertical': pixel_text_into(img, (x, y), ' '.join(text), 1, vertical=True)
            else: pixel_text_into(img, (x, y), ' '.join(text), 1)
        a = np.array(img)
        for name, lon, lat, side in self.m['cities']:
            x, y = (int(round(v)) for v in self.xy(lon, lat))
            if inside[y, x]:
                a[y - 1:y + 2, x - 1:x + 2] = BLACK
                a[y, x] = 15
        img = Image.fromarray(a, 'P')
        for name, lon, lat, side in self.m['cities']:
            x, y = self.xy(lon, lat)
            if inside[int(round(y)), int(round(x))]:
                pixel_text_into(img, (x + 3, y) if side == 'r' else (x - 3, y), name, BLACK,
                                'lm' if side == 'r' else 'rm')
        a = np.array(img)
        out = np.full((H, W), GREY, np.uint8)
        out[inside] = a[inside]
        frame = np.zeros((H, W), bool)
        frame[y0:y1 + 1, x0:x1 + 1] = True
        frame[y0 + t:y1 - t + 1, x0 + t:x1 - t + 1] = False
        out[frame] = BLACK
        return out


def grow(mask, n):
    m = mask.copy()
    for _ in range(n):
        s = m.copy()
        s[1:] |= m[:-1]; s[:-1] |= m[1:]; s[:, 1:] |= m[:, :-1]; s[:, :-1] |= m[:, 1:]
        m = s
    return m


# ---- pixel font ---------------------------------------------------------------------------------
# All labels use hand-drawn pixel fonts, made for the EGA screen (TrueType fonts at this size lose
# too much when drawn without anti-aliasing). Small labels and city names: capitals 7 pixels high,
# lowercase 5 (plus 2 for descenders), 1 pixel between letters. Each glyph is 9 rows: 7 down to the
# baseline, then 2 for descenders.
PIXEL_GLYPHS = {
    'A': '.###. #...# #...# ##### #...# #...# #...#', 'B': '####. #...# #...# ####. #...# #...# ####.',
    'C': '.###. #...# #.... #.... #.... #...# .###.', 'D': '####. #...# #...# #...# #...# #...# ####.',
    'E': '##### #.... #.... ####. #.... #.... #####', 'F': '##### #.... #.... ####. #.... #.... #....',
    'G': '.###. #...# #.... #.### #...# #...# .####', 'H': '#...# #...# #...# ##### #...# #...# #...#',
    'I': '### .#. .#. .#. .#. .#. ###', 'J': '..### ...#. ...#. ...#. ...#. #..#. .##..',
    'K': '#...# #..#. #.#.. ##... #.#.. #..#. #...#', 'L': '#.... #.... #.... #.... #.... #.... #####',
    'M': '#...# ##.## #.#.# #.#.# #...# #...# #...#', 'N': '#...# #...# ##..# #.#.# #..## #...# #...#',
    'O': '.###. #...# #...# #...# #...# #...# .###.', 'P': '####. #...# #...# ####. #.... #.... #....',
    'Q': '.###. #...# #...# #...# #.#.# #..#. .##.#', 'R': '####. #...# #...# ####. #.#.. #..#. #...#',
    'S': '.#### #.... #.... .###. ....# ....# ####.', 'T': '##### ..#.. ..#.. ..#.. ..#.. ..#.. ..#..',
    'U': '#...# #...# #...# #...# #...# #...# .###.', 'V': '#...# #...# #...# #...# #...# .#.#. ..#..',
    'W': '#...# #...# #...# #.#.# #.#.# ##.## #...#', 'X': '#...# #...# .#.#. ..#.. .#.#. #...# #...#',
    'Y': '#...# #...# .#.#. ..#.. ..#.. ..#.. ..#..', 'Z': '##### ....# ...#. ..#.. .#... #.... #####',
    'a': '..... ..... .###. ....# .#### #...# .####', 'b': '#.... #.... ####. #...# #...# #...# ####.',
    'c': '.... .... .### #... #... #... .###', 'd': '....# ....# .#### #...# #...# #...# .####',
    'e': '..... ..... .###. #...# ##### #.... .###.', 'f': '..## .#.. #### .#.. .#.. .#.. .#..',
    'g': '..... ..... .#### #...# #...# #...# .#### ....# .###.',
    'h': '#.... #.... ####. #...# #...# #...# #...#', 'i': '# . # # # # #',
    'j': '..# ... ..# ..# ..# ..# ..# ..# ##.', 'k': '#... #... #..# #.#. ##.. #.#. #..#',
    'l': '# # # # # # #', 'm': '..... ..... ####. #.#.# #.#.# #.#.# #.#.#',
    'n': '..... ..... ####. #...# #...# #...# #...#', 'o': '..... ..... .###. #...# #...# #...# .###.',
    'p': '..... ..... ####. #...# #...# #...# ####. #.... #....',
    'q': '..... ..... .#### #...# #...# #...# .#### ....# ....#',
    'r': '.... .... #.## ##.. #... #... #...', 's': '.... .... .### #... .##. ...# ###.',
    't': '... .#. ### .#. .#. .#. ..#', 'u': '..... ..... #...# #...# #...# #...# .####',
    'v': '..... ..... #...# #...# #...# .#.#. ..#..', 'w': '..... ..... #...# #...# #.#.# #.#.# .#.#.',
    'x': '..... ..... #...# .#.#. ..#.. .#.#. #...#',
    'y': '..... ..... #...# #...# #...# #...# .#### ....# .###.',
    'z': '..... ..... ##### ...#. ..#.. .#... #####',
    ' ': '... ... ... ... ... ... ...', '.': '. . . . . . #', '-': '... ... ... ### ... ... ...',
    "'": '# # . . . . .',
}


# Bold capitals for the big country names: 9 pixels high, strokes 2 pixels wide.
BIG_GLYPHS = {
    'A': '..###.. .##.##. ##...## ##...## ##...## ####### ##...## ##...## ##...##',
    'B': '######. ##...## ##...## ##...## ######. ##...## ##...## ##...## ######.',
    'C': '.#####. ##...## ##..... ##..... ##..... ##..... ##..... ##...## .#####.',
    'D': '#####.. ##..##. ##...## ##...## ##...## ##...## ##...## ##..##. #####..',
    'E': '###### ##.... ##.... ##.... #####. ##.... ##.... ##.... ######',
    'F': '###### ##.... ##.... ##.... #####. ##.... ##.... ##.... ##....',
    'G': '.#####. ##...## ##..... ##..... ##.#### ##...## ##...## ##...## .######',
    'H': '##...## ##...## ##...## ##...## ####### ##...## ##...## ##...## ##...##',
    'I': '#### .##. .##. .##. .##. .##. .##. .##. ####',
    'J': '...#### ....##. ....##. ....##. ....##. ....##. ##..##. ##..##. .####..',
    'K': '##...## ##..##. ##.##.. ####... ###.... ####... ##.##.. ##..##. ##...##',
    'L': '##.... ##.... ##.... ##.... ##.... ##.... ##.... ##.... ######',
    'M': '##...## ###.### ####### ##.#.## ##...## ##...## ##...## ##...## ##...##',
    'N': '##...## ###..## ###..## ####.## ##.#### ##..### ##..### ##...## ##...##',
    'O': '.#####. ##...## ##...## ##...## ##...## ##...## ##...## ##...## .#####.',
    'P': '######. ##...## ##...## ##...## ######. ##..... ##..... ##..... ##.....',
    'Q': '.#####. ##...## ##...## ##...## ##...## ##...## ##.#### ##..##. .###.##',
    'R': '######. ##...## ##...## ##...## ######. ##.##.. ##..##. ##...## ##...##',
    'S': '.#####. ##...## ##..... ##..... .#####. .....## .....## ##...## .#####.',
    'T': '###### ..##.. ..##.. ..##.. ..##.. ..##.. ..##.. ..##.. ..##..',
    'U': '##...## ##...## ##...## ##...## ##...## ##...## ##...## ##...## .#####.',
    'V': '##...## ##...## ##...## ##...## ##...## .##.##. .##.##. ..###.. ...#...',
    'W': '##...## ##...## ##...## ##...## ##...## ##.#.## ####### ###.### ##...##',
    'X': '##...## ##...## .##.##. ..###.. ..###.. ..###.. .##.##. ##...## ##...##',
    'Y': '##...## ##...## .##.##. .##.##. ..###.. ..###.. ..###.. ..###.. ..###..',
    'Z': '####### .....## ....##. ...##.. ..##... .##.... ##..... ##..... #######',
    ' ': '.. .. .. .. .. .. .. .. ..',
}


def pixel_text(text, glyphs=PIXEL_GLYPHS):
    """The text drawn in a pixel font: a bool array 9 rows high."""
    parts = []
    for ch in text:
        rows = glyphs[ch].split()
        rows += ['.' * len(rows[0])] * (9 - len(rows))
        parts += [np.array([[c == '#' for c in r] for r in rows]), np.zeros((9, 1), bool)]
    return np.hstack(parts[:-1])


def pixel_text_into(img, xy, text, colour, anchor='mm', vertical=False, big=False):
    """Pixel-font text (big: the bold capitals) written into the palette image in place, anchored
    like text_into (the middle is the middle of the capitals); vertical text reads bottom to top."""
    m = pixel_text(text, BIG_GLYPHS if big else PIXEL_GLYPHS)
    x, y = int(round(xy[0])), int(round(xy[1]))
    if vertical:
        m = np.rot90(m)
        x0, y0 = x - 3, y - m.shape[0] // 2
    else:
        w = m.shape[1]
        x0 = x - w // 2 if anchor[0] == 'm' else x if anchor[0] == 'l' else x - w + 1
        y0 = y - (4 if big else 3)
    a = np.array(img)
    ys, xs = np.nonzero(m)
    ys, xs = ys + y0, xs + x0
    ok = (ys >= 0) & (ys < a.shape[0]) & (xs >= 0) & (xs < a.shape[1])
    a[ys[ok], xs[ok]] = colour
    img.paste(Image.fromarray(a, 'P'))


# ---- .MP format -------------------------------------------------------------------------------

def encode_mp(a):
    """Encode a 640x350 picture of EGA colour numbers as .MP, like the original files: rows stored
    bottom-up as one stream of runs [colour u8][x_end u16][y u16] (a run ends just before
    (x_end, y) and may continue across rows); the stream ends at (639, 349) and the last pixel of
    the top screen row is never drawn. Plus one trailing byte."""
    s = np.asarray(a, np.uint8)[::-1].reshape(-1)
    n = W * H - 1
    out = bytearray()
    i = 0
    while i < n:
        c = s[i]
        e = i
        while e < n and s[e] == c: e += 1
        x, y = (W - 1, H - 1) if e == n else (e % W, e // W)
        if x == 0 and e != n:
            raise ValueError('a run ends at the start of a row: not representable')
        out += struct.pack('<BHH', int(c), x, y)
        i = e
    out += b'\x07'
    return bytes(out)


def decode_mp(data):
    px = np.zeros(W * H, np.uint8)
    si = cx = cy = 0
    while si + 5 <= len(data):
        col, xe, ye = data[si], *struct.unpack_from('<HH', data, si + 1)
        si += 5
        while cy <= H - 1 and cy != ye:
            px[cy * W + cx:cy * W + W] = col; cy += 1; cx = 0
        if cy > H - 1: break
        px[cy * W + cx:cy * W + xe] = col; cx = xe
        if ye == H - 1 and xe == W - 1: break
    return px.reshape(H, W)[::-1]


EGA = [(0, 0, 0), (0, 0, 170), (0, 170, 0), (0, 170, 170), (170, 0, 0), (170, 0, 170), (170, 85, 0),
       (170, 170, 170), (85, 85, 85), (85, 85, 255), (85, 255, 85), (85, 255, 255), (255, 85, 85),
       (255, 85, 255), (255, 255, 85), (255, 255, 255)]


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--out', default=os.path.join(root, 'assets', 'maps'))
    ap.add_argument('--preview', help='also write PNG previews (shown at 4:3) here')
    ap.add_argument('--compare', metavar='GAMEDIR', help='report agreement with the original maps')
    ap.add_argument('--cache', default=os.path.expanduser('~/.cache/conflict-port/naturalearth'))
    a = ap.parse_args()
    ne = natural_earth(a.cache)
    os.makedirs(a.out, exist_ok=True)
    for name in MAPS:
        pic = Map(name, ne).draw()
        mp = encode_mp(pic)
        if len(mp) > 40000:
            sys.exit(f'{name}: {len(mp)} bytes, over the 40000 bytes the game reads per map')
        assert (decode_mp(mp)[1:] == pic[1:]).all()
        with open(os.path.join(a.out, name + '.MP'), 'wb') as f:
            f.write(mp)
        line = f'{name}.MP  {len(mp)} bytes'
        if a.compare:
            orig = decode_mp(open(os.path.join(a.compare, name + '.MP'), 'rb').read())
            x0, y0, x1, y1, t = MAPS[name]['frame']
            o = orig[y0 + t:y1 - t + 1, x0 + t:x1 - t + 1]; p = pic[y0 + t:y1 - t + 1, x0 + t:x1 - t + 1]
            areas = np.isin(o, [2, 10, 11, 12, 14]) & np.isin(p, [2, 10, 11, 12, 14])
            outside = np.ones((H, W), bool); outside[y0:y1 + 1, x0:x1 + 1] = False
            outside[0, W - 1] = False
            line += (f'  same colour as the original on {(o == p)[areas].mean():.1%} of the map area,'
                     f' outside the map identical: {(orig[outside] == pic[outside]).all()}')
        print(line)
        if a.preview:
            os.makedirs(a.preview, exist_ok=True)
            im = Image.fromarray(pic, 'P'); im.putpalette(sum(EGA, ()))
            im.convert('RGB').resize((1280, 960), Image.NEAREST).save(os.path.join(a.preview, name + '.png'))


if __name__ == '__main__':
    main()
