# Daylight_aw

**Original Airwindows effects in a calm, hand-drawn interface.**
A free, open-source VST3 / AU plugin by **big_tyme_designs**, with 135 Airwindows effects.
Manual: `docs/Daylight_aw_Manual.pdf` (also in every download).

- One Airwindows effect per instance, chosen once from a searchable list (categories, favourites,
  recently used and the newest Airwindows releases in **Latest**).
- The sound is the **original Airwindows code, unchanged**. Automatic tests prove the output is
  bit-identical.
- Each effect gets its own layout, with related controls grouped. The window is Small or Large,
  with tabs for big effects.
- Light, Dark, Original (plain DAW-style faders) and a User theme you can edit (`themes/README.md`).
- A/B compare, input/output trim, zoom 100–200 %, full automation, sessions restore exactly.
- User presets per effect, the full Airwindopedia text for every effect, and tooltips.
- Make your own layouts without rebuilding: Settings → Edit layout (`docs/LAYOUTS.md`).

See `CHANGELOG.md` for what changed in each version.

Not an official Airwindows release. All DSP is by Chris Johnson / Airwindows (MIT). Please
support him: https://www.patreon.com/airwindows

## Download

Finished versions: the **Releases** page of this repository. Test builds: open the **Actions** tab,
pick the latest green run, and download from *Artifacts* at the bottom:

- `Daylight_aw-Windows-VST3`
- `Daylight_aw-macOS-VST3-AU`
- `Daylight_aw-Linux-VST3` (community-tested)

Tagged versions also appear under **Releases**. Install steps are in the zips (`INSTALL.txt`).

## Repository map

| Path | What |
|---|---|
| `manifest/effects.json` | **The list of effects** (id, Airwindows name, category, description, version) |
| `layouts/<id>.json` | Optional hand-made layout per effect (groups, labels, control types) |
| `themes/light`, `themes/dark` | Built-in themes (`theme.json` + `paper.png` + `figure.png`) |
| `src/dsp` | Effect registry and host wrapper (no GUI code) |
| `src/PluginProcessor.*` | Parameters, audio path, trims, bypass, A/B, session state |
| `src/gui` | Window, controls, panels, theme loading, settings |
| `third_party/airwin_consolidated` | Unchanged Airwindows code (pinned commit) |
| `tools/gen_effects.py` | Turns the manifest into build files |
| `tools/sync_upstream.py` | Reports and applies Airwindows upstream changes |
| `tests/` | Automatic tests and GUI snapshots |
| `docs/` | Build spec, testing checklist, install guides |

## Building locally

Needs CMake 3.22+, a C++20 compiler and Python 3. JUCE 8.0.15 is fetched automatically.

```
python3 tools/gen_effects.py
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target Daylight_aw_VST3 DaylightTests
./build/DaylightTests_artefacts/Release/DaylightTests
```

On a Mac also build `Daylight_aw_AU`. On Linux, install the X11/freetype dev packages listed in
`.github/workflows/build.yml`.

## Updating (the routine)

**Add an effect.** Copy its three files (`X.h`, `X.cpp`, `XProc.cpp`) from airwin2rack into
`third_party/airwin_consolidated/autogen_airwin/`. Add one line to `manifest/effects.json`. If you
like, add `layouts/<id>.json`. Then run `tools/gen_effects.py` and push. Effects without a layout
file get an automatic one.

**Change a layout.** Edit `layouts/<id>.json` and push. Missing parameters are added
automatically, so nothing can disappear. The tests check every layout.

**Change the look.** Edit `themes/*/theme.json`, or the PNGs. Users can do the same in their
User theme folder without rebuilding.

**Airwindows updated upstream.** Run `tools/sync_upstream.py <airwin2rack checkout>`. It reports
which included effects changed. Apply with `--apply`, then bump `version` for those effects.

**Never** rename or remove an effect `id`, or reorder parameter slots. Old sessions depend on
them. Sessions store the effect id plus parameter names, so loading stays robust.

**Release.** See `docs/RELEASING.md`: bump the version, then publish a release on github.com.
GitHub builds and attaches the downloads.

## Contributing and releases

- Want to add effects, layouts or themes? See `CONTRIBUTING.md`. Everything works through GitHub.
- Public downloads are on the **Releases** page; how to publish one is in `docs/RELEASING.md`.

## Licence

GNU AGPL v3 (because of JUCE). See `LICENSE` and `THIRD_PARTY_NOTICES.md`.
