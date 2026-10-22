# Daylight_aw themes

A theme is one folder:

```
theme.json   colours and strengths ("tokens")
paper.png    seamless paper texture, 512 x 512 px recommended (drawn at 2x: one tile = 256 px on screen)   - optional
figure.png   the picture in the bottom-right corner, transparent PNG, about 320 x 317 px              - optional
```

The built-in **Light** and **Dark** themes use exactly this format (`themes/light`, `themes/dark` in the
source code). Your own theme lives in `Documents/big_tyme_designs/Daylight_aw/Themes/User/`.
Choose **Theme -> User** in the plugin's settings: the folder is created as a copy of the current theme.
While the plugin window is open, changes you save are shown within about a second.

Themes change colours and pictures only - never sizes, positions or fonts.
Anything missing or invalid falls back to the Light theme's value, so a theme can never break the plugin.

## Colours
`#RRGGBB` or `#RRGGBBAA` (AA = opacity, 00-FF).

## Tokens

| Token | Meaning |
|---|---|
| `paper.color` | background colour (shown under/instead of the paper texture) |
| `paper.textureStrength` | 0-1, how strongly paper.png is shown |
| `horizon.color`, `horizon.opacity` | the faint horizon line behind the figure |
| `ink.text` | parameter names, values, logo row text |
| `ink.muted` | secondary text (descriptions, tab names) |
| `ink.groupTitle` | group titles (INPUT, TONE ...) |
| `frame.color`, `frame.opacity` | group frame outline |
| `frame.glowColor`, `frame.glowRadius` (px), `frame.glowStrength` (0-1) | soft glow around frames (0 = off) |
| `knob.fill`, `knob.edge`, `knob.pointer`, `knob.shadow`, `knob.ticks` | knobs, stepped knobs, switches |
| `knob.style` | knob outline: `"rough"` (hand-cut edge), `"soft"` (round pencil circle with a light second stroke) or `"grain"` (round with a fine charcoal edge) |
| `bar.fill`, `bar.border`, `bar.glow` | the menu bar, BYPASS and tab buttons background |
| `panel.fill`, `panel.border`, `panel.text`, `panel.glow` | dropdowns, About window, Effect info, dialogs, tooltips |
| `button.outline`, `button.activeFill`, `button.activeText` | A/B, segmented buttons, BYPASS, tabs |
| `logo.color`, `logo.glow` | the "Daylight_aw" logo |

Keep text readable: aim for at least 4.5:1 contrast between text and background. The plugin shows a
warning in its settings if your theme is below that.
