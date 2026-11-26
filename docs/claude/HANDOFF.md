# Daylight_aw – HANDOFF (2026-11-26, stage start 1.1)

Holds only "now"; overwritten each checkpoint/stage close, older versions live in git history.
Decisions for 1.1: `Daylight_1.1_Plan.md` (+ `Stage_1.1_DecisionLog.md` for anything decided during the build). History up to 1.0.0: `Daylight_Coding_Progress.md`. Public per-version history: `CHANGELOG.md`. Working rules: `CLAUDE.md` (repo root).

## Status
- **1.0.0 released**: tag `v1.0.0` (0cfe8b4) is on GitHub; CI run #16 all green.
- **1.1 design done**, all mockups approved (canvas "Daylight_aw 1.1 mockups", board 8 FINAL: https://claude.ai/artifact/S4d9MawtdxQzM8HTZCL697).
- Work moved to **Claude Code** on Johan's PC (2026-11-26). Toolchain checked: VS Build Tools 2022 17.14 (MSVC 14.44, SDK 10.0.26100), CMake 3.31 + Ninja bundled, Python 3.15, Git. Commands in `CLAUDE.md`. No local build run yet.

## Next step
Johan answers Yes/No on `Daylight_1.1_ChangeList.md` (plus the open questions in it) → build 1.1 in the repo in the order given there.

## Open items
- Open questions Q1–Q5 in `Daylight_1.1_ChangeList.md` (Inter as UI font, theme images, old User theme, upstream check, release date).
- Manual rebuild needs Python `playwright` + Chromium (not installed on this PC); ask before installing.
- pluginval not installed locally; it runs in CI.
- Mac signing/notarisation (friend) – still open, not part of 1.1.

## Johan's to-dos
- Yes/No on the 1.1 change list.
- Commit: `CLAUDE.md` + `docs/claude/` (new). Note: the repo is public, so `docs/claude/` will be visible on GitHub.
- Optional: delete the stray local tag `1.0.0` (no v; not on GitHub any more): `git tag -d 1.0.0`.

## Starter line
*"stage start: Daylight_aw 1.1 build. Read CLAUDE.md, docs/claude/HANDOFF.md and docs/claude/Daylight_1.1_ChangeList.md; confirm status and next step in 2–3 lines before you start."*
