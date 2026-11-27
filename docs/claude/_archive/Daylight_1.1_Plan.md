# Daylight_aw 1.1 – plan and decisions (checkpoint 26-11-2026)

Copied from the claude.ai project (`claude/Daylight_1.1_Plan.md`) on 2026-11-26; this repo copy is now the master.
Build list with acceptance criteria: `Daylight_1.1_ChangeList.md`.

**Status:** design phase done, all mockups approved. Next: written change list with acceptance criteria → Johan's Yes/No → build in the repo.
Mockups: design canvas "Daylight_aw 1.1 mockups" – https://claude.ai/artifact/S4d9MawtdxQzM8HTZCL697 (board 8 "FINAL" = the chosen look; boards 1–7 = options and explorations).

## Decisions (Johan, 26-11-2026)

### Top bar (option A)
- Logo "Daylight_aw" in Jost Light (300, letter-spaced) – replaces Shantell Sans. Check in the repo whether anything else still uses Shantell; if not, drop the font + its licence line.
- Effect-name button narrower (~170 px at 100 %); opens the presets/settings dropdown. Before an effect is chosen it reads "Choose effect".
- Input/Output trims move to the top bar next to A/B as mini knobs (~22 px) with "IN"/"OUT" label + value; value teal when not 0 dB. Trims removed from the dropdown.
- Bypass button ~22 px, same size as the mini knobs, with "BYPASS" label.
- Bug fix: bypass toggles only from the bypass button, not from clicking anywhere in the top panel.
- Logo click → About popup.

### About popup (option A, compact)
- ~220 px wide; name (14 px) + version, then one paragraph: "Based on the work of Chris Johnson (Airwindows). Airwindows code used under the MIT licence. Not an official Airwindows release." (11 px), then the licence line (10 px).
- No "by big_tyme_designs", no Close button: closes on click outside or Esc.

### Presets + settings dropdown (option B)
- ~460 px wide (not full width), drops from the name button; closes on any click outside.
- Left ~60 %: Presets – Save preset + list; right-click rename/delete/show in folder.
- Right column: Zoom (small dropdown 100/125/150/200 %), Theme (small dropdown Default / User) + "Open theme folder…" link, then Reset to defaults and Effect info.
- Removed: tooltips (setting + feature), Edit layout, Remove, trims, Light/Dark/Original themes.

### Themes
- Only Default + User. Light, Dark, Original removed. Unknown/old saved theme → Default.
- New Default look ("deep space"): near-black background (`#07080b`), two corner halos (teal top-left, violet bottom-right), group panels = thin faint outline + very slight dark fill, ring knobs: no fill, thin outline ring + violet pointer. Teal group titles kept.
- New theme tokens needed: halo colours/positions/strengths, knob style "ring" with transparent fill.
- User theme = copy of Default in the theme folder + a short, clear "How to make your own theme" guide in that folder (edit theme.json, save, live reload ~1 s; broken values fall back safely). Low-effort for users.
- User layouts removed completely (Edit layout feature, user layout files).

### Window sizes
- Three sizes: Small 1 row (0–5 params, ~700×240), Medium 2 rows (new, ~700×395), Large 3 rows (~700×575; 16+ params keep page tabs, only Large has the tab strip).
- Size chosen by rows needed after group packing, not by parameter count (groups never split; 9–10 params can still need 3 rows). ToTape9 → Medium (1+2+2 / 3+1).

### Effect browser
- Remove old "New" and "Last added" folders.
- New = effects added since the previous Daylight_aw update (manifest `since == version`); hidden when empty.
- Last 6 months = Airwindows effects released in the 6 months before the release date of the newest effect in the manifest; sorted newest first. Fallback if not possible: since the last update. (Manifest `released` dates per effect – check in repo.)
- Favourites collections: user-made collections shown under ★ Favourites (Clippers, Mix bus…). Tags, not folders – one effect can be in several. Right-click effect → Add to collection / New collection…; right-click collection → rename / delete / reorder. Deleting a collection never removes effects or favourites. Stored in the global settings file, synced live between instances.

### Controls
- Fine adjustment: Shift + drag (unchanged) and right-click + drag (one-handed). Right-click without moving stays free for a future per-knob menu.
- Unified type scale: 13 px buttons/names, 12 px values, 11 px caps section titles.

### Later (not 1.1)
- MIDI learn – planned for a future update: learn mode + per-knob right-click "MIDI Learn", pickup mode, saved with the session. VST3 caveat: CCs need IMidiMapping/IMidiLearn or JUCE's CC-emulation parameters (can add many hidden params) and a MIDI input on an effect. Until then, use the DAW's own learn (REAPER: Param → Learn).

## Next step
Write the full 1.1 change list with 2–4 acceptance criteria for Johan's Yes/No before any build. Next release: `release.version` 1.1.0, `previous: "1.0.0"` (docs/RELEASING.md).
