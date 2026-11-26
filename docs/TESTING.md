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
| Layouts | every parameter of every effect is on screen; group/row/tab limits hold; window size = packed rows |
| GUI snapshots | every effect drawn in Default, a sample in User, plus the dropdown, About, Effect info (download "gui-snapshots" from the run) |
| Browser + settings | New / Last 6 months / families, Latest versions only, collections, 1.0 settings and User theme migration |
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

Install: close REAPER fully, copy `Daylight_aw.vst3` to `C:\Program Files\Common Files\VST3\`, then rescan
(Options > Preferences > Plug-ins > VST > Re-scan).

Tick each line. Write down anything odd, with the effect name and what you did.

### 1.1 checks (new in this version)

**Look and top bar**
- [ ] New instance: dark "deep space" window with a teal glow top-left and a violet glow bottom-right;
      top bar reads `Daylight_aw` (thin Jost logo), `Choose effect`, A/B (faded), BYPASS
- [ ] ToTape9: Medium window (2 rows of groups), group panels with teal titles, ring knobs with violet pointers
- [ ] Compare with board 8 of the design canvas at 100 % and 200 % zoom
- [ ] IN / OUT mini knobs next to A/B: drag, Shift-drag (fine), wheel, double-click = 0.0; the value
      turns teal when not 0 dB; audio level changes as before
- [ ] BYPASS: clicking the round button toggles bypass (same as REAPER's own bypass)
- [ ] **Bug fix:** clicking the BYPASS label, empty top-bar space, the logo or the name button never toggles bypass

**About, dropdown**
- [ ] Logo click: small About popup (name, version, the Airwindows paragraph, licences); closes on a click
      outside and on Esc; that click doesn't move a knob
- [ ] Effect name click: dropdown (presets left, Zoom / Theme / Open theme folder / Reset / Effect info right);
      closes on any click outside
- [ ] Presets: Save preset, load, right-click Rename / Move to trash / Show in folder all work
- [ ] Zoom dropdown 100/125/150/200 %: every open instance resizes
- [ ] Theme dropdown Default / User; "Open theme folder..." opens `Documents\big_tyme_designs\Daylight_aw\Themes\User`
      with theme.json and "How to make your own theme.txt"
- [ ] Change `background.color` in the User theme.json, save: the window updates within a second
- [ ] If you had a 1.0 User theme: it is now in `Themes\User\old_1.0\`
- [ ] Reset to defaults and Effect info work; no tooltips anywhere; no Edit layout anywhere

**Window sizes**
- [ ] A 1-row effect (e.g. Pop3 or a 3-knob effect) opens Small; ToTape9 Medium; StoneFireComp Large
- [ ] ConsoleXChannel: Large with page tabs at the bottom; tabs switch pages

**Effect browser**
- [ ] Folders: Favourites, + New collection, Recently used, New (2), Last 6 months (newest first), All effects,
      categories
- [ ] "Latest versions only" is ticked: the Tape folder shows ToTape9 but not ToTape5-8; untick: all appear
- [ ] Star ToTape8 (untick, star it, tick again): ToTape8 stays in Tape and Favourites; un-star it: it disappears
      from Tape (Latest view)
- [ ] "+ New collection": type "Clippers", Enter; right-click ADClip8 > Add to collection > Clippers; it shows in Clippers
- [ ] One effect in two collections; rename a collection; Move up / Move down; Delete: the effects and favourites stay
- [ ] A second Daylight_aw window shows the new collection within a second (open its effect list)
- [ ] Search finds names and descriptions; with Latest ticked, searching "ToTape8" says an older version matches

**Change effect**
- [ ] ToTape9 playing: effect name > Change effect... > the browser opens (Large window) > pick Galactic3 >
      the confirmation names both effects > Change effect: Galactic3 at its defaults, the window resizes,
      no crash, no lasting noise
- [ ] REAPER's parameter list (Param button) now shows Galactic3's names; save, close, reopen: Galactic3
- [ ] Ctrl+Z after the change: back to ToTape9 with its values (or note what REAPER does)
- [ ] Cancel in the confirmation, or clicking the effect name, returns to the effect unchanged
- [ ] New folder: PunchyDeluxe2 and PointyDeluxe2; both load and sound fine
- [ ] Every row in the browser shows a release date ("Nov 2026")
- [ ] A/B, the current page tab and BYPASS (on) are green; About is small with the short text

**Controls**
- [ ] Right-click + drag on a knob = fine adjustment (like Shift-drag); right-click without moving changes nothing
- [ ] Right-drag works on the IN / OUT mini knobs too

**Old projects**
- [ ] A project saved with 1.0 opens with the same effect, values, A/B, trims and preset name
- [ ] A 1.0 project that uses an older version (e.g. ToTape8) opens and sounds the same, even though the
      browser hides ToTape8

### Loading
- [ ] Daylight_aw appears under big_tyme_designs in the FX browser
- [ ] Inserting it opens the window with the effect list open
- [ ] Before choosing, audio passes through unchanged (toggle the FX on/off: no difference)

### Choosing an effect
- [ ] Choosing asks "Load X? This can't be changed later"; Cancel does nothing
- [ ] Favourites are still there in a new instance and after restarting REAPER
- [ ] Recently used fills up

### Controls (try on ToTape9, Galactic3, ConsoleXChannel, DeEss)
- [ ] Drag changes the value, Shift-drag is fine, mouse wheel works
- [ ] Double-click the knob resets it; double-click the value lets you type a value
- [ ] Stepped controls (e.g. PocketVerbs type, ADClip8 mode) snap to their steps
- [ ] Sound changes as expected; no clicks when moving controls (other than what the effect itself does)

### Presets
- [ ] "Save preset" asks for a name; Enter saves, Esc cancels; an existing name asks "Replace it?" first
- [ ] The name button shows "ToTape9 · My preset"; moving a knob adds "*"; loading the preset again removes it
- [ ] Presets are per effect; a preset saved in one instance shows up in another instance of the same effect
- [ ] Save the project with a preset loaded and edited, reopen: the name and the "*" are back
- [ ] REAPER's preset field shows "ToTape9 - Warm Tape" after loading a preset

### Host integration
- [ ] The REAPER parameter list (the "Param" button) shows the effect's real parameter names
- [ ] Automation: write automation for 2 controls, play back; the knobs move
- [ ] REAPER's bypass and the Daylight_aw bypass button are the same thing, with a short smooth fade
- [ ] A/B: change settings, switch A/B, both are remembered
- [ ] Save the project, close REAPER, reopen: the same effect, values, A/B, tab and trims are back
- [ ] Copy the FX to another track (drag): an identical copy
- [ ] Undo / redo of a parameter change works
- [ ] Mono track: works, no error
- [ ] Changing sample rate (44.1k / 48k / 96k) and buffer size: no glitches, no silence
- [ ] Render (offline, faster than real time) matches what you hear; reverb tails are not cut

### Window and robustness
- [ ] Closing/opening the window many times while playing: no crash, no audio drop
- [ ] 20+ instances on different tracks: CPU stays reasonable (compare with the effect alone)
- [ ] Delete `Documents\big_tyme_designs\Daylight_aw\settings.json`: plugin still opens, defaults return
- [ ] Put nonsense in the User `theme.json`: Default values are used, the dropdown says what was wrong, no crash
- [ ] Open `Daylight_aw_Manual.pdf` from the download zip

## Mac (AU) quick checklist

Use `docs/INSTALL_MAC.txt` to install (unsigned test build: `xattr` command).
- [ ] `auval -v aufx Dlaw Btyd` ends with **AU VALIDATION SUCCEEDED**
- [ ] Logic / GarageBand / REAPER-mac: loads, chooses an effect, sound OK
- [ ] Save, reopen the project: everything restored
- [ ] Automation and bypass work
- [ ] Both Apple silicon and Intel Macs, if possible
