# Daylight_aw layouts

A layout decides which controls of an effect are grouped together, in what order, what they are
called on screen and whether they are knobs, stepped knobs or switches. It never changes the sound.

## Your own layouts (no rebuild needed)

1. Open the effect, click its name (Settings) and press **Edit layout**.
   Daylight_aw writes `<effect id>.json` into this folder, as a copy of the layout you see, and shows
   the file. Folder: `Documents/big_tyme_designs/Daylight_aw/Layouts/`.
2. Open the file in any text editor (Notepad, TextEdit, VS Code ...), change it, save.
   Every open window of that effect updates within a second.
3. **Remove** (next to Edit layout) moves your file to the trash: back to the built-in layout.

Your files are never overwritten by updates. They work in the Light, Dark and User themes; the
Original theme always shows the plain fader list.

## The format

```json
{
  "formatVersion": 1,
  "pages": [
    { "name": "Main", "groups": [
      { "title": "Input",   "controls": [ "Input" ] },
      { "title": "Tone",    "controls": [ "Tilt", "Shape" ] },
      { "title": "Flutter", "controls": [ "Flutter", { "param": "FlutSpd", "label": "Speed" } ] }
    ] }
  ]
}
```

- `pages`: one or more pages; a page is a tab (only shown when there is more than one).
  For a single page you can also write `"groups": [...]` directly instead of `"pages"`.
- A **group** is a frame with a `title` and 1 to 5 `controls`. Groups are placed left to right,
  5 controls per row, 3 rows per page; a group is never split.
- A **control** is either the parameter's name (exactly as listed in `"_parameters"` in your file),
  or an object:
  - `"param"`: the parameter's name (required)
  - `"label"`: the name shown on screen
  - `"type"`: `"knob"` (default), `"stepped"` (snaps to choices, `"steps": n`) or `"switch"` (2 positions)
- Keys starting with `_` are notes for you and are ignored.

## It can't break anything

- Unknown parameter names, doubles and unknown types are ignored, and Settings shows what was fixed.
- Any parameter your file leaves out is added at the end automatically: nothing can go missing.
- A file that isn't valid JSON is ignored and the built-in layout is used.

## Sharing a layout

Send the `.json` file to a friend (same folder on their computer), or add it to the project:
put it in `layouts/` in the repository (see CONTRIBUTING.md). Then it becomes the built-in
layout for everyone.
