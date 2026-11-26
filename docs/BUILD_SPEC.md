# Daylight_aw build spec (1.1)

## Identity

| Item | Value |
|---|---|
| Plugin name | `Daylight_aw` |
| Vendor | `big_tyme_designs` |
| Manufacturer / plugin code | `Btyd` / `Dlaw` |
| Bundle ID | `com.bigtymedesigns.daylightaw` |
| AU type | `aufx Dlaw Btyd` (effect) |
| Formats | VST3 (Windows x64, macOS universal), AU (macOS universal) |
| macOS minimum | 10.13 |
| Framework | JUCE 8.0.15 (pinned), C++20, CMake |
| User folder | `Documents/big_tyme_designs/Daylight_aw/` (Presets, Themes/User, settings.json) |
| Fonts | Inter 400/500/600/700 (interface), Jost 300/400 (logo), SIL OFL, embedded |

## Audio engine

- One effect per instance; silent pass-through until it is chosen. It can be changed later
  (`changeEffect`, message thread): the new effect host is swapped in under the host's callback lock,
  the audio thread stops pushing slot values until the new defaults are set (`paramsReady`), and the
  old host is a `shared_ptr` that is freed when no thread uses it any more (never on the audio thread).
  A saved state of another known effect loaded into an instance switches to that effect (DAW undo).
- Each effect has two instances:
  - The **audio** instance is used only by the audio thread.
  - The **display** instance is used only by the GUI, for value texts and step detection.
  - So there is no shared mutable state between threads. The audio thread reaches its
    instance through an atomic pointer.
- Parameter values are pushed to the effect at the start of each block, only when they changed.
- Float and double (64-bit) processing are both supported natively, through the effect's
  `processReplacing` and `processDoubleReplacing`.
- Blocks larger than the size given in `prepareToPlay` are processed in chunks.
- Mono tracks: the left input is copied into a scratch right channel, and the left output is used.
- Trims are ±12 dB, smoothed over 20 ms. They are skipped entirely at 0 dB, so the output is
  bit-transparent.
- Bypass is the host bypass parameter. It crossfades over 10 ms. When fully bypassed the effect
  is not run at all.
- No forced denormal flushing. The effects behave exactly as they do in the original plugins.
- Shared function-level statics in a few original effects (DeEss, ButterComp2) are left as they
  are, because that is original behaviour. The generator detects them and flags them in
  `EffectInfo::hasSharedState`.

## Host parameters

| Parameter | Detail |
|---|---|
| `p01` … `p87` | 87 slots. The largest effect has 37 parameters; the rest is headroom for future effects. |
| Slot names | Follow the chosen effect. Unused slots are named `-` and are not automatable. |
| `intrim`, `outtrim` | Input and output trim, ±12 dB |
| `bypass` | Host bypass |

Host display info is refreshed after an effect is chosen.

## Session state (XML)

```
<DaylightState formatVersion pluginVersion effectId effectVersion>
  <Param name index value/> ...
  <AB active ...> <Trims .../> <UI tab/>
```

When loading, parameters are matched by name first and by index as a fallback.

An effect id this version doesn't know is kept verbatim, passed through, and saved back
unchanged. Broken data is ignored.

## GUI

- **Top bar** (56 px): logo "Daylight_aw" (Jost Light 20 px; click = compact About popup), effect-name
  button (~170 px; "Choose effect" before an effect is chosen; opens the presets + settings dropdown),
  A/B, IN / OUT trims as 22 px mini knobs with value (teal when not 0 dB; hidden before an effect is
  chosen), BYPASS label + 22 px round button. Only the round button toggles bypass.
- **Popups**: the presets + settings dropdown (460 x 300; compact layout in the Small window) and About
  (180 px, 12/10/9 px text) close on any click outside
  them or Esc; that click does nothing else. Effect info and the effect browser are panels under the
  top bar.
- **Size and zoom.** Width 700. The window follows the rows of the packed layout: Small 240 (1 row),
  Medium 395 (2 rows), Large 575 (3 rows; more than 3 rows = pages of 3 rows with tabs, Large only).
  Groups never split. Zoom is 100 / 125 / 150 / 200 % and is applied as a transform.
- **Layouts.** They come from `layouts/<id>.json` (embedded) or an automatic layout.
  - A group holds 1-5 controls and never splits. Rows are left-aligned on a 5-column grid
    (124 px columns, 12 px gaps).
  - Any parameter missing from a layout file is appended automatically.
- **Controls.**
  - Types are knob, stepped and switch. Steps are auto-detected from the effect's display text.
  - Ring knobs: 46 px, name 13 px above, value 12 px below.
  - Drag 250 px for the full range; Shift + drag or right-click + drag gives fine control (10x).
    A right-click without moving does nothing (kept for a future per-knob menu).
  - Double-click the knob to reset it; double-click the value to type one.
  - The mouse wheel moves 1 % per notch, or 0.1 % with Shift.
- **Type scale.** 13 px buttons and names, 12 px values, 11 px caps section titles (Inter).
- **Themes.** Default ("deep space") and User. A theme is `theme.json` tokens only (colours, halo
  positions, knob style "ring"/"round", corner radius). The User theme lives in a folder, reloads live,
  and falls back to Default for any missing or invalid value; it is created as a copy of Default plus
  "How to make your own theme.txt". A 1.0 User theme (`formatVersion` 1) is moved to `old_1.0/` first.
  Old saved themes (Light, Dark, Original) open as Default.
- **Static drawing.** Background, halos, top-bar line, group panels and titles are cached in one image,
  so idle CPU is close to zero.

## Effect browser

- Folders: Favourites (+ the user's collections under it, "+ New collection"), Recently used, New
  (effects with `since == release.version`; hidden when empty), Last 6 months (released within
  `release.lastMonths` before the newest `released` date; newest first), All effects, categories.
- Collections are tags: one effect can be in several; deleting a collection never removes effects or
  favourites. Right-click an effect: favourite, Add to collection, New collection. Right-click a
  collection: Rename, Move up/down, Delete.
- **Latest versions only** (default on): per family (`EffectInfo::family`; name without its trailing
  number unless the manifest sets `family`), only the newest version is listed in categories, All,
  New, Last 6 months and search. Older versions that are a favourite or in a collection stay listed.
  Favourites, collections and Recently used always list what is in them. Effects are never removed
  from the plugin, so sessions always load.
- Search covers names and descriptions. Every row shows the Airwindows release date (category first in
  mixed lists).
- *Change effect...* in the dropdown opens the browser in the Large window; choosing asks for confirmation.

## Global settings

`settings.json` (formatVersion 2): theme, zoom, latestOnly, favourites, collections
(`[{name, effects: [ids]}]`), recent. Shared live between instances (file time checked once a second).
Unknown keys (e.g. 1.0's `tooltips`) are ignored.

## User presets

- One JSON file per preset: `Documents/big_tyme_designs/Daylight_aw/Presets/<Effect name>/<Preset>.dlpreset`.
- Contents: format, plugin, effect id, effect name, effect version, and `values` as a list of {name, value 0..1}.
- On load, values are matched by parameter name. Unknown names are ignored; missing ones get the effect default.
- Trims, bypass and A/B are not stored. A preset loads into the active A/B slot.
- Writes are atomic. Names are sanitised for Windows and macOS (illegal characters, reserved names, 64 characters).
- Delete moves the file to the system trash.
- The menu bar shows `Effect · Preset`, with `*` when any value differs from the loaded or saved preset.
  The name and the snapshot are saved with the session.

## Effect info

- The Airwindopedia text for each included effect is embedded (`third_party/airwin_consolidated/awpdoc/`,
  from the pinned airwin2rack commit, MIT). It is shown in a scrollable Effect info panel.
- No tooltips (removed in 1.1).
- Program name reported to the host: `Effect` or `Effect - Preset`.

## Data-driven pipeline

`manifest/effects.json` → `tools/gen_effects.py` → `cmake/Effects.cmake` + `src/generated/EffectList.gen.cpp`.
CI fails if the generated files are stale (`--check`).

## Verification

- `DaylightTests`:
  - null tests against the original code (float and double, odd block sizes);
  - pass-through, bypass, trims and mono;
  - state round trip and robustness;
  - layout rules and step detection;
  - window size per effect (rows), browser folders, families, collections, settings and theme migration;
  - snapshots of every effect (Default) and of the panels.
- pluginval at strictness 10 (Linux, Windows, macOS). auval strict (macOS).
- ASan/UBSan build of the tests.

## Stages

- **A** (this build). Engine with all 21 V1 effects, selector, generic and 3 hand layouts, themes,
  zoom, trims, A/B, about, CI.
- **B** (done, v0.2). Hand-made layouts for all 21 V1 effects.
- **C** (presets done, v0.2). User presets (`.dlpreset`, per effect: save / load / rename / trash). Next: polish from test feedback.
- **D.** Release routine, upstream sync, signed/notarised Mac builds (optional, needs an Apple
  developer account).
