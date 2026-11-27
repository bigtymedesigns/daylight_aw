# Daylight_aw – HANDOFF (2026-11-26, 1.1 stage closed)

Holds only "now"; overwritten each checkpoint/stage close, older versions live in git history.
Decision history: `CHANGELOG.md` (public, per version) and `docs/BUILD_SPEC.md`. Stage files of 1.1 (plan, change list, decision log, coding progress up to 1.0): `docs/claude/_archive/`. Working rules: `CLAUDE.md` (repo root).

## Status
- **1.1.0 released** on GitHub (commit d30da65; CI run #21 all green). 533 effects (Airwindows Consolidated pin 91b3343).
- ⚠ The release was published with tag **`1.1.0`** (no "v"): the release job only attaches the download zips for `v*` tags. Fix: see Johan's to-dos.
- Work happens in Claude Code on Johan's PC. Toolchain + build/test commands: `CLAUDE.md`. The local `build/` folder holds the compiled effects (next build is fast; deleting it means ~35 min again).
- Manual tools: Chromium for Playwright is installed for Johan's user (`AppData\Local\ms-playwright`); the Python packages were in a temporary venv – reinstall `playwright` (and `fonttools` for fonts) when needed.

## Next step
Next update = new Airwindows effects when they appear: `tools/sync_upstream.py` report → add effects (manifest: since, released, category, description) → version + `release.previous` → CHANGELOG + manual → local build/tests → Johan commits → CI → release with a **`v`** tag (`docs/RELEASING.md`).

## Open items
- Optional workflow hardening: let the release job also accept tags without "v" (small change to `.github/workflows/build.yml`), so this can't happen again.
- Later ideas: MIDI learn (see archived 1.1 plan, "Later"); Mac signing/notarisation (friend).
- "Latest versions only" check box placement was not mocked up; Johan approved the build as is.

## Johan's to-dos
- Fix the 1.1.0 downloads: on github.com → Releases → Daylight_aw 1.1.0 → Edit → tag: type `v1.1.0`, "Create new tag on publish" → Update release. The new tag starts a build that attaches the three zips (~40 min). Then delete the old tag: Tags → `1.1.0` → Delete, and locally `git tag -d 1.1.0` (also the stray `git tag -d 1.0.0`).
- Commit the stage close (headline in the chat).
- claude.ai mirror: replace `HANDOFF.md` in the Daylight project with this file; the old `claude/Daylight_1.1_Plan.md` and `Daylight_Coding_Progress` copies there can go (masters are archived in the repo).

## Starter line
*"stage start: Daylight_aw update – add the new Airwindows effects. Read CLAUDE.md and docs/claude/HANDOFF.md; confirm status and next step in 2–3 lines before you start."*
