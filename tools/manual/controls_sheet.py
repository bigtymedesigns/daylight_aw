#!/usr/bin/env python3
"""Builds the control overview picture for the manual from `DaylightTests --controls <dir>` output.
Usage: controls_sheet.py <controls dir> <out.png> <fonts dir>"""
import sys
from PIL import Image, ImageDraw, ImageFont

src, out, fonts = sys.argv[1], sys.argv[2], sys.argv[3]
cell = (24, 150, 296, 420)  # first control cell at 2x
cols = [("Knob", "totape9", cell), ("Stepped knob (dots)", "golem", (560, 150, 832, 420)),
        ("Stepped knob (17 steps)", "monitoring3", cell), ("Switch", "pauldither", cell),
        ("Trim knobs (settings)", "debess", (784, 170, 1044, 370))]
W, H = 300, 280
label = ImageFont.truetype(f"{fonts}/shantell_semibold.ttf", 22)
big = ImageFont.truetype(f"{fonts}/shantell_semibold.ttf", 30)
sheet = Image.new("RGB", (len(cols) * (W + 16) + 166, 2 * (H + 16) + 80), (26, 26, 26))
dr = ImageDraw.Draw(sheet)
for i, (lab, _, _) in enumerate(cols):
    dr.text((166 + i * (W + 16), 20), lab, fill=(230, 228, 220), font=label)
for r, th in enumerate(["light", "dark"]):
    dr.text((14, 80 + r * (H + 16) + H // 2 - 15), th.capitalize(), fill=(230, 228, 220), font=big)
    for i, (_, eid, c) in enumerate(cols):
        im = Image.open(f"{src}/{eid}_{th}.png").convert("RGB").crop(c)
        if im.width > W:
            im = im.resize((W, int(W * im.height / im.width)))
        sheet.paste(im, (166 + i * (W + 16), 64 + r * (H + 16)))
sheet.save(out)
