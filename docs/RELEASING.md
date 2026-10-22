# Releasing Daylight_aw

Public downloads live on the repository's **Releases** page:
`https://github.com/bigtymedesigns/daylight_aw/releases`.
They are free, permanent and need no GitHub account.
Actions *Artifacts* are only for testing: they need a login and are deleted after 90 days.

## Steps

1. In `manifest/effects.json` set `"latest": {"until": "<today>"}` (the Latest folder counts back from it).
   Rebuild the manual (`tools/manual/build_manual.py`, see its header) so it shows the current effects.
2. Set the version in `CMakeLists.txt` (`project(Daylight_aw VERSION 1.0.0 ...)`) and add a section to
   `CHANGELOG.md`. Commit and push, then wait for a green Actions run.
3. On github.com, open **Releases → Draft a new release**.
   - **Choose a tag**: type `v1.0.0` (must match the version, with a `v`), then *Create new tag on publish*.
   - Title: `Daylight_aw 1.0.0`. Description: paste the CHANGELOG section.
   - Click **Publish release**.
4. Within ~25 minutes Actions builds everything again and attaches the zips to the release:
   - `Daylight_aw-v1.0.0-Windows-VST3.zip`
   - `Daylight_aw-v1.0.0-macOS-VST3-AU.zip`
   - `Daylight_aw-v1.0.0-Linux-VST3.zip`

## macOS signing (recommended before a wide public release)

The Mac build is ad-hoc signed only, so users must run the `xattr` command in `INSTALL.txt`.
To make it install like any other plugin, it needs Apple's *Developer ID* signing and notarisation.
That requires an Apple Developer account (paid yearly). Once there is one, signing can be added to the
workflow using GitHub *Secrets*, so no certificates are ever stored in the repository.

## Windows

VST3 plugins don't need signing. The zip contains the `.vst3` folder and `INSTALL.txt`.
An installer (.exe) could be added later, but a zip is the most transparent option for open source.
