#!/usr/bin/env python3
"""Differential test: run ported functions and the original (via tools/oracle.py) on the same
DGROUP state and compare the results byte for byte.

usage: .venv/bin/python tools/difftest.py [--game DIR] [--build DIR] CASE...
  CASE = FUNCTION[:ARG,ARG...][@SAVE][#SEED]   e.g. nation_turn:3@CONF1.SAV#42
         SAVE '-' or omitted = fresh data segment; SEED defaults to 1
  --sweep FUNCTION  run the function on every save x seeds 1..N with every nation argument
"""
import argparse
import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from oracle import Oracle, diff  # noqa: E402

# DGROUP ranges the port legitimately does not reproduce.
MASK = [
    (0x91a0, 0x91a6, 'news::format cursor/buffer pointers (C++ statics in the port)'),
    (0x8318, 0x83a2, 'EGA driver private state (pen, spans, segments)'),
    (0x92aa, 0x92b8, "C library sprintf's string-stream state"),
]


def masked(off):
    return any(a <= off < b for a, b, _ in MASK)


def run_case(o, build, game, func, args, save, seed, tmp):
    prefix = os.path.join(tmp, 'case')
    cmd = [os.path.join(build, 'difftest'), game, save, str(seed), func, *map(str, args), prefix]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        return f'port failed: {res.stderr.strip()}'
    addr, port_ax = res.stdout.split()
    port_restart = int(port_ax.split(':')[1]) if port_ax.startswith('restart:') else None
    before = bytearray(open(prefix + '.before', 'rb').read())
    port_after = open(prefix + '.after', 'rb').read()
    keys = parse_keys(os.environ.get('DIFFTEST_KEYS', ''))
    try:
        orig_after, orig_ax = o.call(before, addr, args, keys=keys)
    except RuntimeError as e:
        return f'original failed: {e}'
    d = [x for x in diff(orig_after, port_after) if not masked(x[0])]
    problems = []
    orig_restart = orig_ax - 0x10000 if orig_ax >= 0x10000 else None
    if orig_restart != port_restart:
        problems.append(f'restart: original {orig_restart}, port {port_restart}')
    if func in ('random', 'choose_quantity') and port_restart is None and int(port_ax) != orig_ax:
        problems.append(f'return value: original {orig_ax}, port {port_ax}')
    for off, a, b in d[:8]:
        problems.append(f'DS:{off:04x} original {a.hex()} port {b.hex()}')
    if len(d) > 8:
        problems.append(f'... {len(d) - 8} more differing ranges')
    return '\n    '.join(problems) if problems else None


KEY_NAMES = {'enter': 0x1c0d, 'esc': 0x011b, 'up': 0x4800, 'down': 0x5000, 'home': 0x4700,
             'end': 0x4f00, 'S-e': 0x1245, 'S-s': 0x1f53, 'S-b': 0x3042}


def parse_keys(text):
    return [KEY_NAMES[k] for k in text.split(',') if k]


def parse_case(text):
    seed = 1
    save = '-'
    if '#' in text:
        text, s = text.rsplit('#', 1)
        seed = int(s, 0)
    if '@' in text:
        text, save = text.split('@', 1)
    args = []
    if ':' in text:
        text, a = text.split(':', 1)
        args = [int(x, 0) for x in a.split(',') if x]
    return text, args, save, seed


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    ap = argparse.ArgumentParser()
    ap.add_argument('--game', default=os.path.join(root, 'conflict'))
    ap.add_argument('--build', default=os.path.join(root, 'build'))
    ap.add_argument('--sweep', help='function taking one nation argument (or none)')
    ap.add_argument('--seeds', type=int, default=5)
    ap.add_argument('cases', nargs='*')
    a = ap.parse_args()
    o = Oracle(os.path.join(a.game, 'Conflict.exe'))
    cases = [parse_case(c) for c in a.cases]
    if a.sweep:
        name, fixed, _, _ = parse_case(a.sweep)
        saves = ['-'] + sorted(os.path.join(a.game, f) for f in os.listdir(a.game) if '.SAV' in f.upper())
        nargs = {'new_game': 0, 'event_news': 1, 'relation_news': 2, 'pressure': 2,
                 'adjust_relation': 3, 'drift_news': 2, 'end_of_month': 0, 'build_digest': 0,
                 'deliver_arms': 0, 'garrison_borders': 0, 'covert_operations': 0, 'newspaper': 0, 'foreign_office_advice': 0,
                 'make_peace': 2, 'leadership_analysis': 0, 'summit': 0, 'procurement': 0, 'review_forces': 0, 'nuclear_program': 0, 'west_bank': 0, 'player_month': 0, 'strategic_action': 0, 'fight_wars': 0, 'air_strikes': 0,
                 'israeli_strikes': 0, 'ground_battle': 2, 'helicopter_battle': 2, 'mobilise': 2}.get(name, 1)
        for save in saves:
            for seed in range(1, a.seeds + 1):
                if nargs >= 2:
                    for x in range(8):
                        for y in range(8):
                            if x == y:
                                continue
                            if nargs == 2:
                                cases.append((name, [x, y], save, seed))
                            else:
                                for r in (1, 3, 6, 9):
                                    cases.append((name, [x, y, r], save, seed))
                elif fixed or nargs == 0:
                    cases.append((name, fixed, save, seed))
                else:
                    for n in range(8 if name != 'event_news' else 7):
                        cases.append((name, [n if name != 'event_news' else n + 1], save, seed))
    failed = 0
    with tempfile.TemporaryDirectory() as tmp:
        for func, args, save, seed in cases:
            r = run_case(o, a.build, a.game, func, args, save, seed, tmp)
            label = f'{func}({",".join(map(str, args))}) save={os.path.basename(save)} seed={seed}'
            if r:
                failed += 1
                print(f'FAIL {label}\n    {r}')
    print(f'{len(cases) - failed}/{len(cases)} cases match')
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
