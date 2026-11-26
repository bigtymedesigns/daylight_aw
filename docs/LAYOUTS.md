# Daylight_aw layouts

A layout decides which controls of an effect are grouped together, in what order, what they are
called on screen and whether they are knobs, stepped knobs or switches. It never changes the sound.

Layouts are built into the plugin: `layouts/<effect id>.json` in the repository. Effects without a
file get an automatic layout (5 controls per row). To change a layout, edit or add the file and
rebuild (or open a Pull Request, see CONTRIBUTING.md). Since 1.1 there are no user layout files.

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
- A **group** is a panel with a `title` and 1 to 5 `controls`. Groups are placed left to right,
  5 controls per row, 3 rows per page; a group is never split.
- The window size follows the rows the groups need: 1 row Small, 2 rows Medium, 3 rows Large
  (more than 3 rows: pages with tabs, Large window).
- A **control** is either the parameter's name (exactly as the effect names it), or an object:
  - `"param"`: the parameter's name (required)
  - `"label"`: the name shown on screen
  - `"type"`: `"knob"` (default), `"stepped"` (snaps to choices, `"steps": n`) or `"switch"` (2 positions)
- Keys starting with `_` are notes and are ignored.

## It can't break anything

- Unknown parameter names, doubles and unknown types are ignored (the tests report them).
- Any parameter the file leaves out is added at the end automatically: nothing can go missing.
- A file that isn't valid JSON is ignored and the automatic layout is used.
- `DaylightTests` checks every layout file on every build.
