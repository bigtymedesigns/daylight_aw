# Changelog

All notable changes to Daylight_aw. Versions follow semantic versioning.
Sessions and presets from every earlier version keep loading (tested with fixtures in `tests/fixtures`).

## 0.5.0 (test build)
- New theme **Original**: a plain fader view like a DAW shows for a plugin without a GUI. One fader
  per parameter with Airwindows' own names, order and value texts; the computer's own UI font;
  flat menus. Same window sizes and tabs (15 per page) as the other themes; only the look changes.
- **New** category in the effect selector: the effects added in the latest Daylight_aw update
  (manifest field `since`).
- Input/Output trims are knobs (faders in Original): drag, Shift = fine, wheel, double-click = 0 dB.
- The repository ignores built plugins and downloaded build zips.

## 0.4.0 (test build)
- Knobs are now round with a fine charcoal grain on the edge (Light and Dark). The style is a theme
  token (`knob.style`: "grain", "soft" or "rough"), so a User theme can choose another.
- Tooltips can be switched on/off in Settings (remembered, shared by all instances).

## 0.3.0 (test build)
- **Effect info**: the full Airwindopedia text by Chris Johnson for each effect, in a scrollable panel
  (Settings → Effect info). The one-line description stays in the settings panel.
- **Tooltips** on every control, with the original Airwindows parameter name where the layout uses a
  friendlier label, plus short hints for knobs, A/B, bypass, tabs and the menu.
- The DAW's preset/program field shows the effect and preset: "ToTape9 - Warm Tape".
- Robustness: ThreadSanitizer CI job with an audio + GUI thread stress test; tests load sessions
  saved by earlier versions; upstream sync script also tracks the Airwindopedia texts.

## 0.2.0 (test build)
- Hand-made layouts for all 21 V1 effects; clearer ConsoleXChannel labels.
- User presets per effect: save, load, rename, move to trash, show in folder.
  Menu bar shows "Effect · Preset", with "*" when edited; remembered in the session.
- Fix: renaming a preset by changing only upper/lower case on Windows and macOS.

## 0.1.0 (first test build)
- Engine with 21 Airwindows effects (bit-identical to the original code), effect selector,
  Small/Large windows with tabs, Light/Dark/User themes, zoom, trims, A/B, bypass, About.
- GitHub Actions: Windows VST3, macOS VST3 + AU (universal), tests, pluginval, auval, sanitizers.
