# Bugs and quirks in the original CONFLICT (1990)

Found while porting, mostly by comparing the port with the original code run in an emulator.

**Fixes are on by default.** `conflict --original-bugs` turns them all off for exact 1990
behaviour (the differential tests always run that way). In the source every fix is guarded by
`fixes::fix(N)` with N the number below, so `grep -rn "fix(N)" src` finds it. `tests/fixes_tests.cpp`
checks the fixes marked *tested*.

Severity: **crash** (can crash or corrupt memory), **logic** (wrong game result), **minor**
(cosmetic or harmless), **latent** (cannot happen with the shipped data).

| # | Where | Severity | What happens in the original | Status |
|---|-------|----------|------------------------------|--------|
| 1 | `text_window` 1000:0a7c | crash | Splitting the extra lines on `|` scans past the end of the string and `strncpy`s up to the next `|` anywhere in memory into an 80-byte stack buffer. The summit's "War duration … months.\| " line ends with a space after its last `|`; with no `|` nearby the stack is overwritten (the emulated original crashes). | **Fixed, always** (scan and copy bounded) |
| 2 | `post_brigade` 1000:8e39 | logic | Posting a brigade moves 10 SAMs from home into the border's *light tank* slot instead of the SAM slot. | **Fixed** — tested |
| 3 | `fight_wars` 1b92:0bd1 | logic | The "battles fought" counter is not reset per pair of nations. After any battle in a month, every later hostile pair counts as fighting: its war-month counter rises, nuclear use is considered and the conquest check runs — a hostile border with no troops on either side can even count as Israel being overrun, ending the game. | **Fixed** — tested |
| 4 | `peace_offer_check` 1b92:12ed | minor | Compares forces using the loop variable after its loop has ended (always 7 = Israel); works by luck. | No behaviour change; the port names Israel explicitly |
| 5 | `nation_turn` 1000:3138 | logic | Reads two stack slots before writing them: the relation of a nation with itself (for Egypt, left over from an earlier call) and, in the nuclear-programme step, a `random(8)` roll left by the *previous* nation's turn. Relations can also be stale for the "pressure a weaker rival" check. | **Fixed** (own relation per pair; fresh roll) |
| 6 | `text_width` 1f39:09e9 | minor | For the last character it means to measure the glyph's real width but re-tests the same pixel column every time, so it adds 8 or 0. Centring of titles is slightly off. | **Fixed** |
| 7 | `kbd_poll_ext` 1f39:0a58 | minor | Ignores normal keys; after an arrow/Home/End key it blocks waiting for a *second* key (pauses, battles and nuclear warnings). Enter cannot skip pauses. | **Fixed** (any key skips pauses; extended keys return at once) |
| 8 | `load_fonts` 1000:07fc | crash (heap) | Small fonts are `malloc(0x465)` but the loader writes 0x466 bytes: one byte past each block. | **Fixed, always** |
| 9 | `main` 1000:b681 | logic | `chdir("..")` between loading maps and pictures: run from `C:\CONFLICT`, the weapon pictures and save files are looked for in `C:\`. | **Fixed, always** (everything from the game directory) |
| 10 | `draw_icon_border` 1000:24c2 | minor | First random icon is compared with an uninitialised stack word. | **Fixed, always** (initialised to 0) |
| 11 | `choose_quantity` 1000:9d3e | latent | The quantity step is left unset when a unit costs exactly 60 (no weapon does). | **Fixed** |
| 12 | `player_month` 1000:4675 | latent | The Foreign Office loop variable is read before it is set. | **Fixed, always** (initialised to 0) |
| 13 | `border_command` 1000:8462 | latent | An unexpected menu choice prints an error and continues with an unset nation index. | **Fixed** (returns) |
| 14 | `str_find4` 1000:0357 | minor | After a partial match the search resumes at the mismatching character, missing some matches; news items can be left out of a nation's "Official News". | **Fixed** — tested |
| 15 | `border_report` 1000:76f4 | minor | The front-line bar lives in static data and is edited in place; after loading a game it shows whatever the last war left there. | **Fixed** (rebuilt from the front position) |
| 16 | `intelligence` 1000:6fa6 | logic | "Support insurgents" costs 2 from the budget even when the insurgency is already at its maximum and nothing changes. | **Fixed** |
| 17 | strings | minor | "illegal nuclar value" (typo in an internal error message). | **Fixed** |
| 18 | `mobilise` 1000:5aa5 | logic | A share of Egypt's / Syria's home forces (2–10 tenths, depending on the enemy and on Libya/Iraq/Jordan relations) is computed, but the comparison is inverted: whenever the share is below 100 % *all* home forces move to the front. | **Fixed** — tested |
| 19 | `helicopter_battle` 1b92:16c1 | logic | Always uses *Israel's* gunships on the defender's border, even in a war between two other nations (they can be lost there). | **Fixed** (attacker's own gunships) — tested |
| 20 | `israeli_strikes` 1b92:0714 | logic | A successful strike on nuclear facilities subtracts 3 from the programme level with no floor; levels 0–2 become negative, the AI then prints "illegal nuclar value" every month and the programme never advances. | **Fixed** (clamped at 0) — tested |
| 21 | `air_strikes` 1b92:0007, `israeli_strikes` 1b92:0714 | logic | "`if (r == 10 && bombers < 6) bombers = 0; else bombers -= 3;`" — the `else` also fires for the other results in that band, so the target loses 3 more bombers (possibly going negative). | **Fixed** |
| 22 | `air_battle` 1b92:1993, `helicopter_battle` 1b92:16c1 | logic | SAM defence = SAMs × 100 / (troops + 1) is computed in 16 bits; from 328 SAMs on a border the product overflows and the defence turns negative, so large SAM deployments stop shooting anything down. | **Fixed** (32-bit) — tested |
| 23 | `border_report` 1000:76f4 | crash (data) | The front-line markers are written at position front + 4 / + 5 of a 10-character bar; the front reaches ±5 at the end of a war, so the original writes a byte before the bar or overwrites its terminator (the bar then runs on into the next string). | **Fixed** (markers clamped) — tested |
| 24 | `covert_operations` 1000:74e1 | logic | A failed assassination attempt (the plot is exposed) *lowers* US disapproval by 15 instead of raising it, so a botched assassination improved relations with the US — while a successful one leaves them unchanged. Found while documenting the rules ([HOW_THE_GAME_WORKS.md](HOW_THE_GAME_WORKS.md)). | **Fixed** (raises it by 15) — tested |
