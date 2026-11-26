# Daylight_aw — coding progress

Copied from the claude.ai project on 2026-11-26; this repo copy is now the master. History up to 1.0.0; current status lives in `HANDOFF.md`.

GitHub repo: https://github.com/bigtymedesigns/daylight_aw (Actions builds Windows VST3, macOS VST3/AU, Linux VST3). Local repo on Johan's PC: `Documents\GitHub\daylight_aw`.

**Update routine.** Since 1.1 Claude Code works directly in the local repo and builds/tests on Johan's PC (rules: `CLAUDE.md`). Johan reviews, commits and pushes with GitHub Desktop (check "Current repository" is daylight_aw). Plugin builds go to `C:\Program Files\Common Files\VST3\` (copy only `Daylight_aw.vst3`; close REAPER fully first or the DLL is locked), never into the repo. Working rule (Johan): before starting any build, list the contents and ask a separate Yes/No confirmation.

**Cloud build notes (chat era, kept for reference).** Ubuntu apt mirrors and raw.githubusercontent are blocked in the cloud sandbox; build with `-DDAYLIGHT_MINIMAL_X11=ON` plus a compiler-launcher wrapper adding `-DJUCE_USE_XRANDR=0 -DJUCE_USE_XINERAMA=0 -DJUCE_USE_XCURSOR=0` (juceaide needs it too). Full build of 531 effects ≈ 10 min on 2 cores, tests ≈ 4 min. When the device shell wasn't available, many changed files went in one .tar that Johan extracted with `tar -xf <file>.tar -C daylight_aw` (used for 1.0.0).

Public releases: GitHub Releases page (docs/RELEASING.md).

## Done and tested in REAPER
- v0.1–0.5: engine, GUI, presets, Effect info, tooltips, grain knobs, Original theme, trim knobs.
- v0.6.0: review fixes, user layouts (Settings → Edit layout), Linux package, docs.

## FINAL 1.0.0 — pushed 2026-11-07 (c82270e + 0cfe8b4 "Old designs removed"), CI run #16 all green
Decisions (Johan, 2026-11-06): version stays 1.0.0 (first public release); all Consolidated effects; Default theme; New/Last added folders; short manual.

- 531 effects = every effect registered in Airwindows Consolidated (old versions included). Consolidated pin d013e102 (31 Oct 2026). Games (airwindows.com, 2 posts) aren't in Consolidated. 15 unregistered WIP sources (ConsoleX3Buss/Channel/Pre, ConsoleXSubIn/Out) left out. New category folder Unclassified (22).
- `released` for new effects = first commit date in airwindows/airwindows git history. `tail` 10 s for Reverb + Ambience; `dither` for Dithers.
- Upstream code changed for ChannelX, kRockstar2, PurestWarm3, RetroBass, Ultralight2, VerbSevens → `version` 2.
- Manifest `release` replaces `latest`: `{"version": "1.0.0", "date": "2026-11-06", "newMonths": 3, "previous": ""}`. New = released within 3 months before `date` (17 effects). Last added = `since == version` when `previous` is set (0 in 1.0.0). gen_effects checks version = CMakeLists.
- Effect browser folders: Favourites, Recently used, New, Last added, All effects, 23 categories; folder column is a scrolling Viewport.
- Zeroed construction (`src/dsp/ZeroedNew.h`): effects are created in zero-filled memory (some upstream effects leave state uninitialised, e.g. PunchyDeluxe) → deterministic.
- Default theme (`themes/default/theme.json`) = Johan's "Night Mixer", `knob.style: "round"`. Order Default, Light, Dark, Original, User; new installs and unknown settings → Default.
- Manual: 4 pages, no theme/layout content, no effect list; "Designed by big_tyme_designs" once.
- CI: linux-package in `ubuntu:22.04` container on ubuntu-24.04 (ubuntu-22.04 runner deprecated, removed 17 Apr 2027).
- Local: 8542 checks pass, ASan/UBSan clean, -Werror clean, pluginval 10 SUCCESS.
- CI run #16 (0cfe8b4): all 6 jobs green (Linux tests 7m, ASan 11m, TSan 9m, Windows 36m, macOS 26m, Linux package 11m). Artifacts: Windows 5.8 MB, macOS 20 MB, Linux 7 MB (zipped).

## Open items at 1.0.0 (status 2026-11-26 in HANDOFF.md)
- Johan: REAPER checks (docs/TESTING.md "Final 1.0 checks") → publish Release with new tag v1.0.0. An old tag `1.0.0` (no v) points at f85b7c6 (135-effect build): delete it to avoid confusion.
- Mac signing/notarisation (friend) → possibly 1.0.1.
- Next update: set `release.version`, `date`, `previous: "1.0.0"`, new effects `since` = new version (docs/RELEASING.md).
