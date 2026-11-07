# Changelog

All notable changes to Daylight_aw. Versions follow semantic versioning.
Sessions and presets from every earlier version keep loading (tested with fixtures in `tests/fixtures`).

## 1.0.0
- **All 531 Airwindows effects** from the current Airwindows Consolidated (old versions included).
  Effects without an Airwindows category are in their own **Unclassified** folder. Every effect is
  tested bit-identical against the original code.
- Effect browser folders: Favourites, Recently used, **New** (Airwindows releases of the 3 months
  before this version), **Last added** (effects added in an update; empty in 1.0.0), All effects and
  the categories. The folder column scrolls. Manifest field `release` replaces `latest`.
- New built-in **Default** theme with perfectly round knobs; new installs start with it. Theme order:
  Default, Light, Dark, Original, User. Theme files accept `knob.style: "round"`.
- Effects start from cleared memory: a few original effects leave part of their state uninitialised,
  which made two instances differ slightly at the start. Now every instance is repeatable.
- Updated Airwindows code for ChannelX, kRockstar2, PurestWarm3, RetroBass, Ultralight2 and
  VerbSevens (their `version` is now 2).
- Hand-made layouts for 35 effects with 6 or more controls; all others use the automatic layout,
  with pages for effects with more than 15 controls.
- Effects that use a parameter name twice (Pop3, StoneFireComp) restore every value correctly in
  sessions, presets and layouts. Every effect is compiled in its own unit.
- Reverbs and ambiences report a 10 s tail to the DAW.
- Short PDF instruction manual (docs/Daylight_aw_Manual.pdf) in every download.

## 0.6.0 (release candidate)
- **Your own layouts, no rebuild**: Settings → **Edit layout** writes the current layout to
  `Documents/big_tyme_designs/Daylight_aw/Layouts/<effect id>.json` and shows it; edit it in any text
  editor and the window updates live. **Remove** goes back to the built-in layout. Broken or
  incomplete files are repaired (Settings lists what was fixed); no parameter can ever go missing.
  Guide: docs/LAYOUTS.md (also copied into that folder).
- Reverbs report their tail to the DAW (Galactic, PocketVerbs: 10 s), so hosts that pause silent
  plugins or render offline never cut the reverb off (manifest field `tail`).
- Tooltips are off for new installs (Settings → Tooltips On). Theme order: Light, Dark, Original, User.
- Session safety: settings that hosts may save from another thread (A/B, preset name) are now
  protected; A/B and preset snapshots are stored bit-exactly (they were rounded to 7 decimals);
  a DAW preset of a known effect now replaces the "needs a newer Daylight_aw" placeholder.
- Presets: renaming can no longer overwrite another preset on case-sensitive systems, and a failed
  rename puts the file back.
- Faster editor opening (choice detection is done once per effect), only one theme reload per
  settings change, tab hover repaints only the page name.
- The Favourites star is drawn (looks the same on every system). Code cleanup, zero compiler warnings.
- Builds: Linux VST3 package (built on an older Ubuntu for wide compatibility), pluginval also checks
  the AU, warnings count as errors in CI, the effect list is generated automatically during the build.
- New docs: CONTRIBUTING.md (how others add effects/layouts via GitHub), docs/RELEASING.md.
- New tests: big host blocks (chunking) stay bit-identical, exact A/B round trip, unknown → known state.

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
