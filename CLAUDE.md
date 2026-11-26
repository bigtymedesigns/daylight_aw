# Daylight_aw – working rules for Claude Code

Project context: `docs/claude/HANDOFF.md` (status, next step, open items). Read it first each session.
Public docs: `README.md` (repo map, update routine), `docs/BUILD_SPEC.md`, `docs/TESTING.md`, `docs/RELEASING.md`.
Decision history: `CHANGELOG.md` (public, per version) + the current stage decision log in `docs/claude/`.

## Rules
- Johan commits and pushes himself (GitHub Desktop). Never run git commit, push, reset or rebase.
- Before any build, release or irreversible change: list the plan, recommended additions and 2–4 acceptance criteria, then wait for Yes/No. Small normal tweaks need no Yes/No.
- Only hand over code that passed your own build/tests. Say what can only be tested by Johan (REAPER checklist, listening, Mac/AU, CI).
- Clearly separate verified (built, ran, tested) from assumed. Mention hidden costs or risks up front.
- Push back when there's a clearly better, simpler or more robust way: alternative, why, trade-off – then follow Johan's choice.
- Show a mockup before visible UI changes.
- This repo is public (AGPL). Nothing private goes in it; `docs/claude/` is visible to everyone once pushed.

## Stage routine
- "stage start" / "checkpoint" / "stage close" follow the project workflow: decisions go to the stage decision log; checkpoint updates HANDOFF.md only; stage close merges the log, archives to `docs/claude/_archive/`, overwrites HANDOFF.md with "now", and ends with a ready-to-copy commit headline.

## Project specifics
- Toolchain (checked 2026-11-26): VS Build Tools 2022 17.14 (MSVC 14.44, Windows SDK 10.0.26100), CMake 3.31 + Ninja bundled with it, Python 3.15, Git. Same generator as CI (`windows-2022`). CMake is **not on PATH**: use the full path below or a "Developer PowerShell for VS 2022".
  `$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"`
- Build:
  1. `python tools/gen_effects.py` (after any manifest/layout change; CI checks with `--check`)
  2. `& $cmake -S . -B build -G "Visual Studio 17 2022" -A x64`
  3. `& $cmake --build build --config Release --parallel --target Daylight_aw_VST3 DaylightTests`
  First configure downloads JUCE 8.0.15; a full build compiles 531 effect units (long – run in the background).
- Test: `.\build\DaylightTests_artefacts\Release\DaylightTests.exe` (null tests, state, layouts). Snapshots: add `--snapshots snapshots`. pluginval, sanitizers, TSan and auval run only in CI (`.github/workflows/build.yml`).
- Output: `build\Daylight_aw_artefacts\Release\VST3\Daylight_aw.vst3` (Johan installs to `C:\Program Files\Common Files\VST3\` himself).
- Key paths: `manifest/effects.json` (effect list + `release` block), `layouts/`, `themes/`, `src/` (`dsp`, `gui`, `PluginProcessor.*`), `tests/`, `tools/`, `third_party/airwin_consolidated` (never edit the Airwindows DSP).
- Never rename or remove an effect `id` or reorder parameter slots; old sessions depend on them. Sound must stay bit-identical to the original Airwindows code.
- Release steps: `docs/RELEASING.md` (version in `CMakeLists.txt` and manifest `release` must match; rebuild the manual).
