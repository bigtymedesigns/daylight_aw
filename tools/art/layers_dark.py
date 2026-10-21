import numpy as np, sys
sys.argv = ['x', 'none']
exec(open(__import__('pathlib').Path(__file__).with_name('charcoal.py')).read().split("if __name__ == '__main__':")[0])
from scipy.ndimage import gaussian_filter
from PIL import Image
rng = np.random.default_rng(22)
# replay the same random sequence as gen_layers so the cat is identical
s = 1.06
_ = paper(rng, (226, 223, 215), 1.8, 1.8)
ox, oy = W - 430 * s - 60, H - 560 * s - 30
chp = Charcoal(rng)
hy = oy + 470 * s
for k in range(3):
    chp.stroke([(40, hy + rng.normal(0, 2)), (ox - 40 * s, hy + rng.normal(0, 2))], 1.0, 0.06, 1, 1.0)
ch = Charcoal(rng)
ground(ch, ox, oy, s, 1.0)
cat(ch, ox, oy, s)
ink = ch.ink * ch.tooth
sm = ch.smudge
lines = np.clip(gaussian_filter(ink, 0.5), 0, 1)
x0, y0 = int(ox - 120 * s), int(oy - 10)
a = lines[y0:, x0:]
shadow = np.clip(gaussian_filter(sm, 0.5), 0, 1)[y0:, x0:]
glow = np.clip(gaussian_filter(a, 8) * 2.2, 0, 1) * 0.40
chalk = np.array((228, 224, 214), float)
# shadow stays dark (it is a shadow), lines + glow are chalk-light
alpha_light = 1 - (1 - a) * (1 - glow)
alpha = 1 - (1 - alpha_light) * (1 - shadow * 0.9)
col_light = chalk
col = (col_light[None, None, :] * alpha_light[..., None] + np.array((10, 10, 10))[None, None, :] * (shadow * 0.9 * (1 - alpha_light))[..., None]) / np.maximum(alpha, 1e-6)[..., None]
rgba = np.dstack([np.clip(col, 0, 255), alpha * 255]).astype(np.uint8)
im = Image.fromarray(rgba, 'RGBA'); im.save('cat_dark_2x.png', optimize=True)
p = Image.open('paper_dark_2x.jpg').convert('RGBA')
p.alpha_composite(im, (p.width - im.width, p.height - im.height))
p.crop((p.width - 700, p.height - 700, p.width, p.height)).convert('RGB').resize((350, 350)).save('cat_dark_prev.png')
print(im.size)
