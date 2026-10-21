# Daylight_aw build spec (v0.2)

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

## Audio engine

- One effect per instance. It is chosen once and then locked; there is silent pass-through
  until it is chosen.
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

- **Size and zoom.** Width 700. Small is 240 tall (1–5 controls); Large is 575 tall (3 rows of 5,
  with pages of 15 shown as tabs at the bottom). Zoom is 100 / 125 / 150 / 200 % and is applied
  as a transform.
- **Layouts.** They come from `layouts/<id>.json`, or an automatic layout.
  - A group holds 1–5 controls and never splits.
  - Rows are left-aligned.
  - Any parameter missing from a layout file is appended automatically.
- **Controls.**
  - Types are knob, stepped and switch. Steps are auto-detected from the effect's display text.
  - Drag 250 px for the full range; Shift gives fine control.
  - Double-click the knob to reset it; double-click the value to type one.
  - The mouse wheel moves 1 % per notch, or 0.1 % with Shift.
- **Themes.** Themes are data: `theme.json` tokens plus `paper.png` (tileable, drawn at 2×) plus
  `figure.png`.
  - Light and Dark are built in.
  - User lives in a folder, reloads live, and falls back to Light for any missing or invalid
    value.
- **Font.** Shantell Sans (OFL), embedded.
- **Static drawing.** Paper, figure, horizon, frames and titles are cached in one image, so idle
  CPU is close to zero.

## User presets

- One JSON file per preset: `Documents/big_tyme_designs/Daylight_aw/Presets/<Effect name>/<Preset>.dlpreset`.
- Contents: format, plugin, effect id, effect name, effect version, and `values` as a list of {name, value 0..1}.
- On load, values are matched by parameter name. Unknown names are ignored; missing ones get the effect default.
- Trims, bypass and A/B are not stored. A preset loads into the active A/B slot.
- Writes are atomic. Names are sanitised for Windows and macOS (illegal characters, reserved names, 64 characters).
- Delete moves the file to the system trash.
- The menu bar shows `Effect · Preset`, with `*` when any value differs from the loaded or saved preset.
  The name and the snapshot are saved with the session.

## Data-driven pipeline

`manifest/effects.json` → `tools/gen_effects.py` → `cmake/Effects.cmake` + `src/generated/EffectList.gen.cpp`.
CI fails if the generated files are stale (`--check`).

## Verification

- `DaylightTests`:
  - null tests against the original code (float and double, odd block sizes);
  - pass-through, bypass, trims and mono;
  - state round trip and robustness;
  - layout rules and step detection;
  - snapshots of every effect in both themes.
- pluginval at strictness 10 (Linux, Windows, macOS). auval strict (macOS).
- ASan/UBSan build of the tests.

## Stages

- **A** (this build). Engine with all 21 V1 effects, selector, generic and 3 hand layouts, themes,
  zoom, trims, A/B, about, CI.
- **B** (done, v0.2). Hand-made layouts for all 21 V1 effects.
- **C** (presets done, v0.2). User presets (`.dlpreset`, per effect: save / load / rename / trash). Next: polish from test feedback.
- **D.** Release routine, upstream sync, signed/notarised Mac builds (optional, needs an Apple
  developer account).
