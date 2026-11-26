# Daylight_aw 1.1 – change list for Yes/No (2026-11-26)

From `Daylight_1.1_Plan.md` + canvas board 8 FINAL, checked against the repo at 0cfe8b4.
"AC" = acceptance criteria. ⚑ = something I found in the repo that the plan didn't settle (see Open questions).

## What stays exactly the same
- The sound: all 531 effects stay bit-identical (null tests untouched). No effect added, removed or renamed; parameter slots unchanged.
- Session format and presets: 1.0.0 sessions and `.dlpreset` files load unchanged (fixtures in `tests/fixtures`).
- Effect info panel, A/B, automation, zoom levels, Recently used, search (already searches names + descriptions).

## 1. Top bar (board 8)
- Logo "Daylight_aw" in **Jost Light** (300, letter-spaced, ~20 px); clicking it opens About.
- Effect-name button ~170 px; reads "Choose effect" until an effect is chosen; opens the presets/settings dropdown. A/B becomes its own small segmented button next to it (today it sits inside the name bar).
- **IN / OUT** trims as ~22 px ring knobs with label + value next to A/B; value turns teal when not 0 dB. Same handling as today (drag, Shift/right-drag fine, wheel, double-click = 0 dB).
- **BYPASS** label + ~22 px round button at the right.
- Bug fix: bypass toggles only from its button. Likely cause found: the bypass hit area is 150×30 px (`PluginEditor.cpp:212`), much bigger than the drawn button.

AC
- At 100 % and 200 % zoom the top bar matches board 8 (order, sizes, colours).
- Clicking anywhere in the top bar except the bypass button never changes bypass (checked in REAPER).
- Trims change the level as before; 0 dB stays bit-transparent (existing tests).

## 2. Fonts and type scale
- ⚑ Shantell Sans is the font of the **whole** interface today, not only the logo. Board 8 uses **Inter** for all text and Jost only for the logo/About title. Plan: embed Inter (SIL OFL) for all UI text, Jost Light (+ Regular for the About title); remove the 3 Shantell files, `OFL.txt` entry and its licence line; remove the `style.font` theme token (only the Original theme used it).
- Type scale everywhere: 13 px buttons/names, 12 px values, 11 px caps section titles.
- `THIRD_PARTY_NOTICES.md` and the About licence line list Inter and Jost.
- Plugin size grows by roughly 1 MB (static Inter weights); acceptable for a 6 MB download.

AC
- No Shantell file or reference left in the repo; licence texts for Inter and Jost included.
- Every text in the GUI uses one of the three sizes (checked in snapshots).

## 3. About popup (board 8, window 3)
- ~220 px, drops below the logo: "Daylight_aw" (Jost 14 px) + version; the paragraph "Based on the work of Chris Johnson (Airwindows). Airwindows code used under the MIT licence. **Not an official Airwindows release.**" (11 px); licence line (10 px): Airwindows (MIT) · Airwindows Consolidated (MIT) · JUCE (AGPLv3) · Inter, Jost (SIL OFL 1.1) · Daylight_aw: AGPLv3.
- No Close button, no "by big_tyme_designs"; closes on click outside or Esc.

AC
- Text exactly as above; closes on outside click and Esc; nothing else on screen reacts to that closing click.

## 4. Presets + settings dropdown (board 8, window 2)
- ~460 × 300 px panel dropping from the name button; closes on any click outside.
- Left: PRESETS – Save preset, list (active one highlighted), "Loads into slot A · right-click for more"; right-click Rename / Move to trash / Show in folder (existing functions).
- Right: ZOOM dropdown (100/125/150/200 %), THEME dropdown (Default / User) + "Open theme folder…" link, Reset to defaults (existing: resets the effect's parameters), Effect info.
- Removed: tooltips **as a feature** (setting + all tooltip code), Edit layout, Remove, trims, Light/Dark/Original.

AC
- Every remaining function works from the new panel (preset save/rename/trash/show, zoom, theme, open folder, reset, Effect info).
- No tooltip ever appears; an old `settings.json` with `tooltips: true` loads without error.

## 5. Themes: only Default + User
- Delete `themes/light`, `themes/dark`, `themes/original` (incl. paper/figure PNGs) and the Original fader/list view code (`style.view`, slider tokens).
- New Default "deep space" from board 8: background `#07080b`; teal halo (`#2fc4a5` at 20 %, centre 10 %/22 %, radius 42 %) and violet halo (`#c657ec` at 17 %, 90 %/88 %, 45 %); group panels 1 px white 9 % outline + `rgba(10,12,16,.35)` fill, radius 8; ring knobs (no fill, white 30 % ring, violet pointer); teal caps group titles; A/B active = violet.
- New tokens: `halo1.*` / `halo2.*` (color, x, y, radius, strength), `knob.style: "ring"`. Unknown tokens are ignored, missing/broken ones fall back to Default.
- ⚑ Drop `paper.png` / `figure.png` support (the new look has no paper texture; simpler code).
- Saved theme Light/Dark/Original/unknown → Default.
- User theme folder: copy of Default `theme.json` + **"How to make your own theme.txt"** (edit, save, reloads in ~1 s, broken values fall back safely). ⚑ An existing 1.0 User theme (a copy of the old Light/Dark) would look broken on the new design: move it to `Themes/User/old_1.0/` once and write the new copy. Nothing is deleted.
- Theme drawing stays cached in one image, so idle CPU stays near zero.

AC
- Fresh install looks like board 8 (snapshot compared side by side).
- Old settings with Light/Dark/Original open as Default; old User folder ends up in `old_1.0/`.
- Editing the User `theme.json` updates the window within ~1 s; nonsense in it → Default values, no crash (test).

## 6. User layouts removed
- Delete `UserLayouts.cpp/.h`, Edit layout / Remove, the layout-guide copy into the user folder. Built-in `layouts/<id>.json` stay (contributors still use them; `docs/LAYOUTS.md` keeps only that part).
- Users' existing `Documents/big_tyme_designs/Daylight_aw/Layouts` files are left on disk and ignored.

AC
- No Edit layout anywhere; a file in the user Layouts folder has no effect; all 35 built-in layouts unchanged (layout tests).

## 7. Window sizes: Small / Medium / Large
- Size from the **rows needed after group packing** (groups never split): 1 row → Small 700×240, 2 rows → **Medium 700×395 (new)**, 3 rows → Large 700×575. More than 3 rows → pages with the tab strip (Large only), as today.
- ToTape9 → Medium (1+2+2 / 3+1).

AC
- ToTape9 opens at 700×395 (100 %); zoom scales all three sizes.
- Test over all 531 effects: window rows ≥ packed rows, every parameter visible, no clipping; only Large shows tabs.

## 8. Effect browser
- Remove the old **New** and **Last added** folders.
- **New** = effects with `since == release.version` (when `previous` is set); hidden when empty → hidden in 1.1.0 unless effects are added (Q4).
- **Last 6 months** = effects released in the 6 months before the newest `released` date in the manifest, newest first. All 531 effects have `released`, so no fallback is needed: today that is 2026-05-01 … 2026-10-31 = **23 effects**. Manifest `release.newMonths: 3` → `lastMonths: 6`; `gen_effects.py` computes the flags.
- **Favourites collections** (board 4/7): listed under ★ Favourites; "+ New collection"; right-click an effect → Add to collection ▸ (tick list) / New collection…; right-click a collection → Rename / Delete / Move up / Move down (menu instead of drag-to-reorder: simpler and robust). Tags, so one effect can be in several. Deleting a collection never touches effects or favourites. Stored in `settings.json` (`collections: [{name, ids}]`), synced live between instances by the existing reload mechanism.

AC
- Last 6 months lists 23 effects, newest first; New is hidden in 1.1.0.
- Collections survive a REAPER restart and show up in another open instance within ~1 s; deleting one leaves favourites intact.
- Old `settings.json` without collections loads fine; unknown effect ids in a collection are ignored (test).

## 9. Controls
- Right-click + drag = fine adjustment (same as Shift), on effect knobs and trims. Right-click without moving does nothing (kept free for a later per-knob menu).

AC
- Right-drag moves at the fine rate; a plain right-click changes nothing and opens nothing.

## 10. Tests, docs, release files
- Tests: snapshots for Default (all effects) + a User sample (Light/Dark/Original dropped); new tests for the three sizes, New / Last 6 months, collections round trip, settings migration (old themes, `tooltips` key), old User theme move. Null, state and fixture tests stay as they are.
- Version 1.1.0 in `CMakeLists.txt`; manifest `release`: `{"version": "1.1.0", "date": <release day>, "lastMonths": 6, "previous": "1.0.0"}`.
- Docs: `CHANGELOG.md` 1.1.0, `README.md`, `docs/BUILD_SPEC.md` (sizes, themes, fonts, browser), `docs/TESTING.md` (new "1.1 checks" for REAPER; obsolete sections removed), `themes/README.md`, `docs/LAYOUTS.md`, manual rebuilt (needs `pip install playwright` + Chromium, ~150 MB – I'll ask first).

AC
- `gen_effects.py --check` passes; versions match.
- CHANGELOG and TESTING.md describe exactly what was built.

## Build order (one Yes covers all; I report after each step)
1. Removals + data: themes, Original view, tooltips, user layouts, manifest/gen_effects, settings migration + collections storage. Build + tests.
2. Look: fonts, Default theme + halos + ring knobs, top bar, sizes, type scale. Build + tests + snapshots for you to compare with board 8.
3. Panels: dropdown, About, browser folders + collections, right-drag fine. Build + tests.
4. Docs, version numbers, manual. Then one test build for your REAPER checks.

## Done for 1.1 when
1. Local Windows build passes all `DaylightTests` (null tests bit-identical + new tests).
2. CI green on all 6 jobs (pluginval 10, auval, sanitizers, TSan).
3. 1.0.0 sessions and presets open unchanged (fixtures + your REAPER check).
4. Your REAPER "1.1 checks" pass and the look matches board 8.

## Recommended additions (say which you want)
- **R1 Upstream check**: run `tools/sync_upstream.py` against the newest Airwindows Consolidated to see what Airwindows released since the 31 Oct pin. If there are new effects, adding them fills the New folder in 1.1 (separate Yes/No once I know the list).
- **R2** Collections reorder by menu (Move up/down) instead of drag – already in item 8; say if you want drag.
- **R3** Delete the stray local tag `1.0.0` (you, `git tag -d 1.0.0`; already gone from GitHub).

## Open questions
- **Q1** Inter for all interface text (board 8), Shantell removed completely? (recommended: yes)
- **Q2** Drop `paper.png` / `figure.png` support from themes? (recommended: yes)
- **Q3** Move an old 1.0 User theme to `Themes/User/old_1.0/` and write the new Default copy? (recommended: yes)
- **Q4** Do R1 (upstream check for new effects) as part of 1.1? (recommended: yes, check first, decide after)
- **Q5** Release date for `release.date` – set at release time? (recommended: yes)

Not in 1.1: MIDI learn (later update), Mac signing.
