import numpy as np, sys
sys.argv = ['x', 'none']
exec(open(__import__('pathlib').Path(__file__).with_name('charcoal.py')).read().split("if __name__ == '__main__':")[0])
from scipy.ndimage import gaussian_filter
from PIL import Image

def layers(seed=22):
    rng = np.random.default_rng(seed)
    base, s, dark = (226, 223, 215), 1.06, np.array((32, 31, 32), float)
    img = paper(rng, base, 1.8, 1.8)
    ox, oy = W - 430 * s - 60, H - 560 * s - 30
    # paper layer: paper + faint horizon only
    chp = Charcoal(rng)
    hy = oy + 470 * s
    for k in range(3):
        chp.stroke([(40, hy + rng.normal(0, 2)), (ox - 40 * s, hy + rng.normal(0, 2))], 1.0, 0.06, 1, 1.0)
    inkp = gaussian_filter(chp.ink * chp.tooth, 0.5)
    pap = np.clip(img * (1 - inkp[..., None]) + dark * inkp[..., None], 0, 255).astype(np.uint8)
    Image.fromarray(pap).save('paper_B_2x.jpg', quality=90)
    Image.fromarray(pap).resize((W // 2, H // 2), Image.LANCZOS).save('paper_B_1x.jpg', quality=90)
    # cat layer: cat + shadow + ground strokes, transparent elsewhere
    ch = Charcoal(rng)
    ground(ch, ox, oy, s, 1.0)
    cat(ch, ox, oy, s)
    ink = ch.ink * ch.tooth
    ink = 1 - (1 - ink) * (1 - ch.smudge * (0.7 + 0.3 * ch.tooth))
    ink = np.clip(gaussian_filter(ink, 0.5), 0, 1)
    x0, y0 = int(ox - 120 * s), int(oy - 10)
    a = (ink[y0:, x0:] * 255).astype(np.uint8)
    rgba = np.zeros(a.shape + (4,), np.uint8)
    rgba[..., :3] = dark.astype(np.uint8); rgba[..., 3] = a
    im = Image.fromarray(rgba, 'RGBA')
    im.save('cat_2x.png', optimize=True)
    im.resize((im.width // 2, im.height // 2), Image.LANCZOS).save('cat_1x.png', optimize=True)
    print('cat', im.size, 'offset from right/bottom 0')

layers()
