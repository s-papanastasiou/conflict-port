# Conflict (1990, Virgin Mastertronic) — reverse-engineering notes

## Binary
- `Conflict.exe`: MZ, 118,379 bytes, 1,927 relocations, header 0x1E40 bytes. Not packed.
- Compiler: **Zortech C 4.00** (library string at DS:0014). Memory model: **medium**
  (far code, near data: all globals in one 64 KB DGROUP).
- Ghidra loads it at base segment 0x1000, so file paragraph `P` is Ghidra segment `P+0x1000`.
  DGROUP = file para 0x128E = Ghidra segment **0x228E**. `SetupDGroup.java` sets DS=SS=0x228E for analysis.
- Entry 1fe0:000a (`_start`), `main` = 1000:b681.

## Segment map (Ghidra segments)
| Segment | Size | Contents |
|---|---|---|
| 1000 | 47 KB, 93 fn | game logic (main loop, menus, save/load, turn processing) |
| 1b92 | 8 KB, 8 fn | game logic (large functions, end-of-turn processing) |
| 1d93 | 6.7 KB, 19 fn | game logic |
| 1f39 | 2.6 KB, 41 fn | **hand-written EGA driver**: GC port 0x3CE, int10 mode set, int16 keyboard, own file loader. No CGA/Tandy code in this build. No int33 (mouse) found yet. |
| 1fe0 | | C startup |
| 2007 | | software floating point (doubles passed in AX:BX:CX:DX) — game uses `double` |
| 2094… 2281 | | Zortech C library (see `symbols.tsv`) |
| 228e | 34 KB init + 11 KB BSS | DGROUP (strings, tables, game state) |

## RNG (must be bit-exact)
`srand(dos_time_hsec())` at startup. `rand()`: `seed = seed*0x41C64E6D + 0x3039; return (seed>>16) & 0x7FFF` (seed at DS:7FF0, 32-bit).

## Save file (7,372 bytes) — see `tools/savedump.py`
A straight sequence of `fwrite`s of 42 globals (DS:92F0…A1F2, DS:A4AE[2500], DS:AE72[1000], DS:B25A…B276).
Arrays of 8 are per nation, index order: Egypt, Iran, Iraq, Jordan, Lebanon, Libya, Syria, Israel(7).
Arrays at A16E, A1A0, A1E2 contain **near pointers** (into the A4AE/AE72 text buffers and DGROUP
string tables), so the port must convert them to/from DS offsets when saving and loading.

## Open questions / next
- DONE: FP helpers modelled as Ghidra call fixups (tools/ghidra_scripts/ZortechFloat.java);
  decompiler now shows plain double arithmetic. Platform target: SDL3.
- Name the per-nation state arrays (drive by watching values change in DOSBox + savedump).

## Data formats (verified by tools/assets.py)
- **.MP**: full-screen 640x350 map, RLE records `[colour u8][x_end u16][y u16]`; fill from cursor to
  x_end-1 on row y, completing earlier rows to x=639. Rows stored bottom-up (screen_y = 349 - y).
  Ends at (639, 349); files carry 1 trailing byte.
- **.DAT**: 160x200 1bpp (20 bytes/row), AND-blitted (0 bits draw black).
- **.FNT** small (ETECH/EXTRA/SGUARD/SINI): 9 bytes/char = 8 rows + width byte; index = char code
  with punctuation remapped (space 0x40, '.' 0x60, '-' 0x5c, '!' 0x5e, '%' 0x5b, '$' 0x5d, other 0x5f).
  Large (X*.FNT): 36 bytes/char, 16 rows x 2 bytes + width; capital letters + checkbox glyphs.
- Screen: EGA mode 10h 640x350x16 with default palette. Two buffers: display (DS:836c) and back buffer (DS:836e).

## Driver API
See the 1f39 entries in `symbols.tsv`. The SDL3 platform layer must reproduce exactly these routines
on a 640x350 indexed framebuffer, plus the back buffer.

## Input and coordinates
- Keyboard: `get_key()` (1000:0444) = blocking `int86(0x16)` AH=0, returns BIOS code `scan<<8 | ascii`.
  Menu codes: Enter 0x1C0D, Up 0x4800, Down 0x5000, Home 0x4700, End 0x4F00,
  Shift-E 0x1245 (quit -> longjmp 999), Shift-S 0x1F53 (save, newspaper screen only), Shift-B 0x3042 (briefing).
  `kbd_poll_ext()` is only used for "press a key to continue" / skip loops.
- Game logic uses a 640x200 coordinate space; y is scaled by 1.75 (double) onto the 350-line EGA screen.
  Keep that arithmetic as-is for pixel-exact output.

## Decompiler setup
- Constant-pointer inference is off (ExportAll.java): in the near-data model, key codes and counters
  otherwise turn into fake string references.
- tools/split_export.py collapses Ghidra's re-packing of register-passed doubles.

## Quirks found while porting (kept unless noted)
The maintained list is docs/ORIGINAL_BUGS.md; the notes below are the original working notes.
- main() calls `chdir("..")` between loading maps and pictures, so from C:\CONFLICT the pictures
  and save files would be looked up in C:\. The port uses the game directory for everything.
- The game's strings write apostrophes as '%'; the font has an apostrophe in the '%' slot.
- `kbd_poll_ext` (1f39:0a58) ignores ASCII keys and, after an extended key, blocks for a second key.
  Skippable waits (intro, title) therefore only react to arrow/Home/End-type keys.
  PORT CHANGE: in those waits (sys::wait_or_key) any key, including Enter, skips.
- `draw_icon_border` compares its first random icon with an uninitialised stack word (port: 0).
- Delays were CPU-speed busy loops (calibrated in 1000:0739); the port uses real time
  (constants in src/game/sys.cpp).

## Differential testing
tools/oracle.py loads CONFLICT.EXE into Unicorn (relocated to segment 0x1000, so addresses equal
Ghidra's), takes a 64 KB DGROUP image and calls an original function far/cdecl; driver calls are
stubbed, unexpected interrupts fail. tests/difftest.cpp runs the ported function on the same state.
Soft-float runs inside the emulator (DS:7f11 = 0), so double arithmetic is checked too.
Stale stack slots read by nation_turn (bp-0x2e, bp-0x2a) are modelled as values persisting
between calls (initially 0).
- BUG in the original: text_window (1000:0a7c) scans for '|' past the end of its lines text
  and strncpy()s up to it into an 80-byte stack buffer. The summit's "War duration" line ends
  with " " after its last '|', so the copy length depends on leftover buffer contents and can
  overflow (the emulated original crashes on such a state). PORT FIX: scan/copy are bounded.
