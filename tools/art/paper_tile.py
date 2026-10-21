#!/usr/bin/env python3
"""Generates the tileable sketchbook-paper textures used by the built-in themes.

    python3 tools/art/paper_tile.py            -> themes/light/paper.png, themes/dark/paper.png

The textures are 512 x 512 px and wrap seamlessly (all noise is generated with
periodic boundaries). They are drawn at 2x density, i.e. one tile covers 256 x 256
plugin pixels at 100 % zoom.
"""
import pathlib
import numpy as np
from scipy.ndimage import gaussian_filter
from PIL import Image

ROOT = pathlib.Path(__file__).resolve().parents[2]
N = 512


def wrap_noise(rng, sigma):
    n = gaussian_filter(rng.standard_normal((N, N)), sigma, mode="wrap")
    return n / (n.std() + 1e-9)


def fibres(rng, amount):
    f = np.zeros((N, N))
    for _ in range(int(380 * amount)):
        x, y = rng.uniform(0, N), rng.uniform(0, N)
        a = rng.uniform(0, np.pi)
        length = rng.uniform(6, 30)
        for t in np.linspace(0, 1, int(length)):
            xi = int(x + np.cos(a) * length * t) % N
            yi = int(y + np.sin(a) * length * t) % N
            f[yi, xi] += 1
    return gaussian_filter(f, 0.6, mode="wrap")


def tile(seed, base, tex, fib, fibre_sign):
    rng = np.random.default_rng(seed)
    img = np.zeros((N, N, 3)) + np.array(base, float)
    tone = tex * (1.1 * wrap_noise(rng, 60) + 0.7 * wrap_noise(rng, 18)) + 1.4 * tex * wrap_noise(rng, 1.0)
    img += tone[..., None]
    img += fibre_sign * fibres(rng, fib)[..., None] * 9
    return Image.fromarray(np.clip(img, 0, 255).astype(np.uint8))


if __name__ == "__main__":
    tile(22, (226, 223, 215), 1.8, 1.8, -1).save(ROOT / "themes/light/paper.png", optimize=True)
    tile(5, (60, 58, 55), 1.2, 0.9, -1).save(ROOT / "themes/dark/paper.png", optimize=True)
    print("paper tiles written")
