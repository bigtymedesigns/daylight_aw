# Contributing to Daylight_aw

Anyone can change Daylight_aw: add Airwindows effects, make layouts and themes, or fix things.
Everything happens on GitHub, and GitHub builds the plugins, so you don't need a compiler.

## What needs what

| You want to... | Rebuild needed? | Where |
|---|---|---|
| Change colours or the look | **No.** Use the User theme | `Documents/big_tyme_designs/Daylight_aw/Themes/User/` (see `themes/README.md`) |
| Make your own layout for an effect | **No.** Settings → Edit layout | `Documents/big_tyme_designs/Daylight_aw/Layouts/` (see `docs/LAYOUTS.md`) |
| Change the built-in layout everyone gets | Yes | `layouts/<id>.json` (same format: copy your file there) |
| Add an Airwindows effect | Yes | `manifest/effects.json` + 3 source files |
| Change the built-in themes | Yes | `themes/<name>/theme.json` + images |

## The GitHub way (no tools needed)

1. **Fork**: on https://github.com/bigtymedesigns/daylight_aw, click **Fork**. You now have your own copy.
2. **Edit**: change files in your fork, either in the browser (pencil icon, or *Add file → Upload files*)
   or with GitHub Desktop.
3. **Build**: in your fork, open **Actions** and enable workflows once. Every push then builds
   Windows/macOS/Linux plugins and runs all tests. Download them from the run's *Artifacts*.
4. **Share back (optional)**: open a **Pull Request** to the main repository. If it passes the tests
   and fits the project, it can be merged into the official Daylight_aw.

## Adding an Airwindows effect

1. Get the effect's three files from Airwindows Consolidated, at the commit pinned in
   `manifest/effects.json`: `src/autogen_airwin/<Name>.h`, `<Name>.cpp`, `<Name>Proc.cpp`
   (https://github.com/baconpaul/airwin2rack). Put them in
   `third_party/airwin_consolidated/autogen_airwin/`. **Never edit them**: the sound must stay original.
2. Optional: copy `res/awpdoc/<Name>.txt` to `third_party/airwin_consolidated/awpdoc/` for Effect info.
3. Add one line to `manifest/effects.json`:
   ```json
   { "id": "purestdrive", "upstream": "PurestDrive", "version": 1, "since": "1.1.0",
     "category": "Saturation", "included": true, "description": "One sentence." }
   ```
   - `id`: lower-case letters/digits, permanent. Never rename or reuse it.
   - `since`: the Daylight_aw version that first includes it.
   - `released`: the date Airwindows first published it (YYYY-MM-DD). The **Latest** folder shows the
     effects released in the months before `"latest": {"until": ...}` at the top of the manifest:
     set `until` to the release date of each new Daylight_aw version.
   - Optional `"tail": 10` for reverbs/delays: seconds the effect keeps sounding after the input
     stops, so DAWs don't cut it off when they pause silent plugins or render.
4. Optional: `layouts/<id>.json` (copy an existing one). Without it the effect gets an automatic layout.
5. Push. The build runs `tools/gen_effects.py` automatically: it checks the manifest and layouts
   (clear error messages if something is wrong) and regenerates the effect list.

The automatic null tests compare every effect with the original Airwindows code, bit for bit.

## Rules that keep sessions safe

- Never rename or remove an effect `id`, and never reorder parameter slots.
- Changing the sound of an existing effect (new upstream code) means bumping its `version`.
- Add a session fixture to `tests/fixtures/` when the state format changes.

## Making your own variant

If you publish your own modified plugin (not via a Pull Request), give it its own identity so it
can't clash with the official one in people's DAWs. In `CMakeLists.txt`, change `PRODUCT_NAME`,
`COMPANY_NAME`, `BUNDLE_ID`, `PLUGIN_MANUFACTURER_CODE` and `PLUGIN_CODE`. The licence is the
GNU AGPL v3: your variant must stay open source, with its source code published.
