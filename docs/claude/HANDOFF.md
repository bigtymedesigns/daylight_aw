# Daylight_aw – HANDOFF (2026-11-26, 1.1 built, waiting for REAPER test)

Holds only "now"; overwritten each checkpoint/stage close, older versions live in git history.
Decisions for 1.1: `Daylight_1.1_Plan.md` + `Stage_1.1_DecisionLog.md`. Build list + acceptance criteria: `Daylight_1.1_ChangeList.md`. History up to 1.0.0: `Daylight_Coding_Progress.md`. Public per-version history: `CHANGELOG.md`. Working rules: `CLAUDE.md` (repo root).

## Status
- **1.0.0 released** (tag `v1.0.0`, 0cfe8b4).
- **1.1 approved by Johan (2026-11-26) and coded**: all change-list items 1–11, incl. "Latest versions only" (one build for everyone, manifest families) instead of a second private build. Version 1.1.0 set (CMakeLists + manifest `release`, previous 1.0.0, date 2026-11-26 placeholder).
- Docs updated: CHANGELOG 1.1.0, README, BUILD_SPEC, TESTING ("1.1 checks"), RELEASING, CONTRIBUTING, LAYOUTS, themes/README + user guide, THIRD_PARTY_NOTICES, manual source text.
- Manual PDF rebuilt for 1.1 (4 pages, new pictures; Playwright + Chromium installed in a scratch venv, not in the repo).
- Fonts: Inter + Jost (OFL) cut by `tools/fonts/make_fonts.py`; Shantell, Light/Dark/Original themes and user layouts removed.
- Round 2 (Johan's Yes, 2026-11-26): Change effect (dropdown → browser → confirmation; DAW undo/session restore switch back), release date on every row, green A/B / page tab / bypass-on, smaller About, PunchyDeluxe2 + PointyDeluxe2 added (533 effects, pin 91b3343).
- **Local Windows build OK; DaylightTests 7119 checks, 0 failed** (null tests bit-identical for all 533 effects; change effect while audio runs: 30 changes, no crash/NaN); `gen_effects --check` OK. Snapshots checked. Window sizes: 438 Small, 79 Medium, 16 Large (8 with tabs). pluginval/auval/sanitizers: CI only (not run locally).
- Test plugin: `build\Daylight_aw_artefacts\Release\VST3\Daylight_aw.vst3`.

## Next step
Build + tests green locally → Johan tests in REAPER (docs/TESTING.md "1.1 checks") → commit → CI green → release.

## Open items
- Change effect: REAPER's own undo / parameter-name refresh only testable by Johan.
- "Latest versions only" check box placement is not from a mockup (right of the list heading).
- `tools/art/` (old paper/cat art) is unused since 1.1; kept.
- Mac signing (friend) – not part of 1.1.

## Johan's to-dos
- REAPER "1.1 checks", then commit (CLAUDE.md, docs/claude/, all 1.1 changes).
- Optional: `git tag -d 1.0.0` (stray local tag).

## Starter line
*"stage start: Daylight_aw 1.1 finish. Read CLAUDE.md and docs/claude/HANDOFF.md; confirm status and next step in 2–3 lines before you start."*
