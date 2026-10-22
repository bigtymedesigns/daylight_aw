# Testing Daylight_aw

Two kinds of testing:

1. **Automatic**. Runs on GitHub for every push (see `.github/workflows/build.yml`).
2. **By hand in a DAW**. Johan tests in REAPER on Windows; the AU is tested on a Mac.

## 1. Automatic checks (GitHub Actions)

| Check | What it proves |
|---|---|
| `gen_effects.py --check` | generated effect list matches `manifest/effects.json` |
| Null tests (all effects, float + double, odd block sizes) | sound is **bit-identical** to the original Airwindows code |
| Pass-through / bypass / trims / mono | no effect chosen = untouched audio; bypass and trims behave; mono tracks work |
| Session state | save/load round trip; unknown effects from newer versions are kept; broken data is ignored |
| Layouts | every parameter of every effect is on screen; group/row/tab limits hold |
| GUI snapshots | every effect drawn in Light and Dark (download "gui-snapshots" from the run) |
| pluginval (strictness 10) | host-style stress test: threads, automation, editor, state, odd buffer sizes |
| auval (Mac) | Apple's Audio Unit validation |
| Sanitizers | no memory errors or undefined behaviour |

A green tick on GitHub means all of the above passed.

**Note on DeEss and ButterComp2.** The original code of these two shares a tiny noise counter
between every copy of the effect that is running. Two copies therefore can't be compared
sample-for-sample in one process. They are allowed a difference of 1e-9 (far below audibility),
which is just what the original plugins do too. The generator detects this automatically for
any future effect.

## 2. REAPER checklist (Windows)

Install: copy `Daylight_aw.vst3` to `C:\Program Files\Common Files\VST3\`, then rescan
(Options > Preferences > Plug-ins > VST > Re-scan).

Tick each line. Write down anything odd, with the effect name and what you did.

### Loading
- [ ] Daylight_aw appears under big_tyme_designs in the FX browser
- [ ] Inserting it opens the window with "Choose effect" and the effect list open
- [ ] Before choosing, audio passes through unchanged (toggle the FX on/off: no difference)

### Choosing an effect
- [ ] Categories, All effects, Search all work
- [ ] Star / right-click adds a favourite; it is still there in a new instance and after restarting REAPER
- [ ] Recently used fills up
- [ ] Choosing asks "Load X? This can't be changed later"; Cancel does nothing
- [ ] After loading, the window has the right size (Small for 1–5 controls, Large for more)

### Controls (try on ToTape9, Galactic, ConsoleXChannel, DeEss)
- [ ] Drag changes the value, Shift-drag is fine, mouse wheel works
- [ ] Double-click the knob resets it; double-click the value lets you type a value
- [ ] Hover underlines the name and shows one more decimal
- [ ] Stepped controls (e.g. PocketVerbs type, ADClip8 mode) snap to their steps
- [ ] ConsoleXChannel: tabs 1/2/3 at the bottom switch pages
- [ ] Sound changes as expected; no clicks when moving controls (other than what the effect itself does)

### Presets
- [ ] Settings panel (click the effect name): "Save preset" asks for a name; Enter saves, Esc cancels
- [ ] The menu bar shows "ToTape9 · My preset"; moving a knob adds "*"; loading the preset again removes it
- [ ] Saving with an existing name asks "Replace it?" first
- [ ] Right-click a preset: Rename, Move to trash, Show in folder all work
- [ ] Presets are per effect: a DeEss instance doesn't list ToTape9 presets
- [ ] A preset saved in one instance shows up in another instance of the same effect (reopen the panel)
- [ ] Save the project with a preset loaded and edited, reopen: the name and the "*" are back
- [ ] Files are in `Documents\big_tyme_designs\Daylight_aw\Presets\<Effect>\` and open in a text editor

### Effect info + tooltips (0.3.0)
- [ ] Settings → **Effect info** opens the full Airwindopedia text; it scrolls; Close returns
- [ ] Clicking the effect name while Effect info is open closes it
- [ ] Hovering a knob for about a second shows a tooltip. Renamed controls show their Airwindows
      name (e.g. ConsoleXChannel "Thresh" → "FC Thrs")
- [ ] Tooltips also on A/B, BYPASS, the logo, the menu bar and the tab buttons
- [ ] Tooltips look right in Light and Dark, and at 150 % / 200 % zoom
- [ ] REAPER's preset field (top of the FX window) shows "ToTape9 - Warm Tape" after loading a preset

### Knobs + tooltip switch (0.4.0)
- [ ] Knobs look round with a fine grainy edge in Light and Dark, at 100 % and 200 %
- [ ] Settings → Tooltips Off: no tooltips appear; On: they're back; the choice survives a REAPER restart
- [ ] User theme: set `"knob.style": "soft"` (or `"rough"`) in theme.json, save: knobs change live

### Host integration
- [ ] The REAPER parameter list (the "Param" button) shows the effect's real parameter names
- [ ] Automation: write automation for 2 controls, play back; the knobs move
- [ ] REAPER's bypass and the Daylight_aw bypass button are the same thing, with a short smooth fade
- [ ] In/Out trim (Settings): 0 dB = no change; ±12 dB works; a small indicator shows when not at 0
- [ ] A/B: change settings, switch A/B, both are remembered
- [ ] Save the project, close REAPER, reopen: the same effect, values, A/B, tab and trims are back
- [ ] Copy the FX to another track (drag): an identical copy
- [ ] Undo / redo of a parameter change works
- [ ] Mono track (set the track to 1 channel / mono item): works, no error
- [ ] Changing sample rate (44.1k / 48k / 96k) and buffer size: no glitches, no silence
- [ ] Render (offline, faster than real time) matches what you hear

### Window
- [ ] Settings: Zoom 100/125/150/200 % resizes the window correctly
- [ ] Theme Light / Dark / User switch instantly; your choice is remembered
- [ ] "Open theme folder" opens `Documents\big_tyme_designs\Daylight_aw\Themes\User`; edit `theme.json`
      (e.g. change `paper.color`), save, and the window updates within a second
- [ ] About: name, version, credits are right
- [ ] Closing/opening the window many times while playing: no crash, no audio drop
- [ ] 20+ instances on different tracks: CPU stays reasonable (compare with the effect alone)

### Robustness
- [ ] Open a project saved with this build in the next build: everything still loads
- [ ] Delete `Documents\big_tyme_designs\Daylight_aw\settings.json`: plugin still opens, defaults return
- [ ] Put nonsense in the User `theme.json`: the plugin falls back to Light values, no crash

## Mac (AU) quick checklist

Use `docs/INSTALL_MAC.txt` to install (unsigned test build: `xattr` command).
- [ ] `auval -v aufx Dlaw Btyd` ends with **AU VALIDATION SUCCEEDED**
- [ ] Logic / GarageBand / REAPER-mac: loads, chooses an effect, sound OK
- [ ] Save, reopen the project: everything restored
- [ ] Automation and bypass work
- [ ] Both Apple silicon and Intel Macs, if possible
