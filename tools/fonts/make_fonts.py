#!/usr/bin/env python3
"""Makes the embedded fonts in resources/fonts/ from the Google Fonts variable fonts.

Sources (SIL Open Font Licence 1.1), from https://github.com/google/fonts:
  ofl/inter/Inter[opsz,wght].ttf   ->  Inter 400 / 500 / 600 / 700 (optical size 14)
  ofl/jost/Jost[wght].ttf          ->  Jost 300 / 400
JUCE can't choose a weight inside a variable font, so static instances are cut, and subset to
Latin (incl. Latin-1 + Latin Extended-A, punctuation, arrows) to keep the plugin small.

Needs fonttools:   python -m pip install fonttools
Usage:             python tools/fonts/make_fonts.py <Inter[opsz,wght].ttf> <Jost[wght].ttf>
"""
import pathlib
import sys

from fontTools import subset
from fontTools.ttLib import TTFont
from fontTools.varLib import instancer

OUT = pathlib.Path(__file__).resolve().parents[2] / "resources" / "fonts"
UNICODES = (list(range(0x20, 0x7F)) + list(range(0xA0, 0x180)) + list(range(0x2000, 0x2070))
            + list(range(0x2190, 0x2200)) + [0x2212, 0x20AC, 0x2122, 0x02C6, 0x02DA, 0x02DC, 0x2605, 0x2606])


def cut(src, axes, out_name):
    font = instancer.instantiateVariableFont(TTFont(src), axes, updateFontNames=False)
    options = subset.Options()
    options.layout_features = ["kern", "liga", "calt", "tnum", "case"]
    options.name_IDs = ["*"]
    options.notdef_outline = True
    sub = subset.Subsetter(options)
    sub.populate(unicodes=UNICODES)
    sub.subset(font)
    path = OUT / out_name
    font.save(path)
    print(f"{path.name}: {path.stat().st_size // 1024} KB")


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    inter, jost = sys.argv[1], sys.argv[2]
    OUT.mkdir(parents=True, exist_ok=True)
    for w in (400, 500, 600, 700):
        cut(inter, {"wght": w, "opsz": 14}, f"inter_{w}.ttf")
    for w in (300, 400):
        cut(jost, {"wght": w}, f"jost_{w}.ttf")


if __name__ == "__main__":
    main()
