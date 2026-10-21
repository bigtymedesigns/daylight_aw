# Theme art tools

The built-in theme pictures are generated, so they can be re-made or varied at any time.

| Script | Makes |
|---|---|
| `paper_tile.py` | `themes/light/paper.png`, `themes/dark/paper.png` – 512 px seamless paper textures (2x density) |
| `charcoal.py` | the charcoal-on-paper renderer (strokes, paper, cat and dog sketches) used by the scripts below |
| `layers_light.py` | light theme cat layer (`cat_1x.png` / `cat_2x.png`) – the 1x file is `themes/light/figure.png` |
| `layers_dark.py` | dark theme cat (same drawing, chalk-light lines with glow) – downscaled to `themes/dark/figure.png` |

Requirements: Python 3, numpy, scipy, Pillow. The art scripts are only needed to redesign the
pictures; building the plugin does not need them.
