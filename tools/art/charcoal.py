"""Charcoal sketch renderer used to draw the theme art (paper, cat). See tools/art/README.md."""
import numpy as np, sys
from scipy.ndimage import gaussian_filter
from PIL import Image

W, H = 1400, 1150  # Large size at 200 % zoom

def noise(shape, sigma, rng):
    n = gaussian_filter(rng.standard_normal(shape), sigma)
    return n / (n.std() + 1e-9)

def paper(rng, base, fiber_amt=1.0, tex=1.0):
    img = np.zeros((H, W, 3)) + np.array(base, float)
    tone = tex * (1.1 * noise((H, W), 160, rng) + 0.7 * noise((H, W), 30, rng)) + 1.4 * tex * noise((H, W), 1.0, rng)
    img += tone[..., None]
    # fibres
    fib = np.zeros((H, W))
    for _ in range(int(2500 * fiber_amt)):
        x, y = rng.uniform(0, W), rng.uniform(0, H)
        a = rng.uniform(0, np.pi); L = rng.uniform(6, 30)
        for t in np.linspace(0, 1, int(L)):
            xi, yi = int(x + np.cos(a) * L * t), int(y + np.sin(a) * L * t)
            if 0 <= xi < W and 0 <= yi < H: fib[yi, xi] += 1
    img -= gaussian_filter(fib, 0.6)[..., None] * 9
    # gentle vignette
    yy, xx = np.mgrid[0:H, 0:W]
    d = np.sqrt(((xx - W / 2) / W) ** 2 + ((yy - H / 2) / H) ** 2)
    img -= (d ** 2 * 22)[..., None]
    return img

def catmull(pts, n=24):
    pts = np.array(pts, float)
    p = np.vstack([pts[0], pts, pts[-1]])
    out = []
    for i in range(1, len(p) - 2):
        p0, p1, p2, p3 = p[i - 1], p[i], p[i + 1], p[i + 2]
        for t in np.linspace(0, 1, n, endpoint=False):
            t2, t3 = t * t, t * t * t
            out.append(0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t2 + (-p0 + 3 * p1 - 3 * p2 + p3) * t3))
    out.append(p[-2])
    return np.array(out)

class Charcoal:
    def __init__(self, rng):
        self.rng = rng
        self.ink = np.zeros((H, W))
        tooth = noise((H, W), 0.8, rng) * 0.6 + noise((H, W), 3, rng) * 0.4
        self.tooth = np.clip(0.55 + 0.35 * tooth, 0, 1)
        r = 7
        yy, xx = np.mgrid[-r:r + 1, -r:r + 1]
        self.r = r; self.yy = yy; self.xx = xx

    def dab(self, x, y, rad, a):
        r = self.r
        xi, yi = int(x), int(y)
        if xi - r < 0 or yi - r < 0 or xi + r >= W or yi + r >= H: return
        d = np.sqrt((self.xx - (x - xi)) ** 2 + (self.yy - (y - yi)) ** 2)
        m = np.clip(1 - (d - rad + 0.8) / 1.6, 0, 1) * a
        sl = self.ink[yi - r:yi + r + 1, xi - r:xi + r + 1]
        sl[:] = 1 - (1 - sl) * (1 - m)

    def stroke(self, pts, width=2.6, alpha=0.55, passes=3, jitter=1.6, taper=True):
        rng = self.rng
        for _ in range(passes):
            q = np.array(pts, float) + rng.normal(0, jitter, (len(pts), 2))
            c = catmull(q, 18)
            seg = np.sqrt((np.diff(c, axis=0) ** 2).sum(1))
            L = np.concatenate([[0], np.cumsum(seg)])
            total = L[-1] if L[-1] > 0 else 1
            ss = np.arange(0, total, 0.8)
            xs, ys = np.interp(ss, L, c[:, 0]), np.interp(ss, L, c[:, 1])
            for s, x, y in zip(ss, xs, ys):
                t = s / total
                w = width * (np.sin(np.pi * t) ** 0.5 if taper else 1) * rng.uniform(0.75, 1.15)
                self.dab(x + rng.normal(0, 0.3), y + rng.normal(0, 0.3), max(w, 0.6), alpha * rng.uniform(0.5, 1))

    def hatch(self, x0, y0, x1, y1, n, ang, length, alpha=0.25, width=1.3, clip=None):
        rng = self.rng
        for _ in range(n):
            x, y = rng.uniform(x0, x1), rng.uniform(y0, y1)
            if clip is not None and not clip(x, y): continue
            L = length * rng.uniform(0.6, 1.2)
            dx, dy = np.cos(ang) * L / 2, np.sin(ang) * L / 2
            self.stroke([(x - dx, y - dy), (x + dx, y + dy)], width, alpha, 1, 0.4)

def dog(ch, ox, oy, s, detail=1.0):
    """Original sitting dog seen from behind (3/4), head turned up-left toward the sky. Box ~400x520 units."""
    P = lambda pts: [(ox + x * s, oy + y * s) for x, y in pts]
    w = 2.6 * s * 1.4
    # back of skull -> forehead
    ch.stroke(P([(255, 192), (264, 150), (254, 104), (226, 74), (192, 66), (168, 74)]), w, 0.6)
    # muzzle top -> nose
    ch.stroke(P([(168, 74), (146, 54), (124, 34), (110, 24)]), w, 0.6)
    ch.stroke(P([(110, 24), (98, 28), (98, 40), (108, 46)]), w * 1.1, 0.65)
    ch.stroke(P([(101, 30), (104, 38)]), w * 1.6, 0.75, 2, 0.6, False)
    # chin / throat -> chest
    ch.stroke(P([(108, 46), (124, 70), (140, 100), (150, 132), (146, 170), (138, 204)]), w, 0.6)
    # eye hint
    ch.stroke(P([(150, 84), (157, 80), (163, 84)]), w * 0.7, 0.45, 2, 0.6)
    # floppy ear
    ch.stroke(P([(222, 80), (248, 98), (262, 138), (258, 180), (242, 188), (232, 160), (226, 118)]), w * 0.9, 0.55)
    # chest/front side down to the left haunch
    ch.stroke(P([(138, 204), (120, 248), (104, 294), (98, 350), (96, 410), (92, 466), (90, 498)]), w, 0.6)
    # front paw + inner leg line
    ch.stroke(P([(84, 500), (104, 508), (134, 508), (142, 500)]), w * 0.85, 0.5)
    ch.stroke(P([(136, 330), (132, 400), (134, 460), (138, 498)]), w * 0.7, 0.4)
    # far haunch peeking out behind the front leg
    ch.stroke(P([(96, 392), (70, 430), (58, 468), (66, 496), (84, 500)]), w * 0.75, 0.42)
    # back side
    ch.stroke(P([(255, 192), (282, 236), (296, 280), (304, 330), (322, 380), (348, 425), (360, 465), (350, 494), (320, 506)]), w, 0.6)
    # base
    ch.stroke(P([(142, 508), (190, 513), (240, 515), (285, 512), (320, 506)]), w * 0.8, 0.45)
    # tail curling on the ground
    ch.stroke(P([(304, 508), (342, 516), (380, 513), (402, 497), (398, 480)]), w * 0.9, 0.55)
    ch.stroke(P([(306, 500), (342, 506), (376, 503), (392, 490)]), w * 0.55, 0.3)
    # inner contours: haunches, spine, shoulder
    ch.stroke(P([(330, 482), (318, 430), (292, 396), (258, 388)]), w * 0.7, 0.4)
    ch.stroke(P([(236, 205), (228, 280), (224, 360)]), w * 0.5, 0.2, 2)
    ch.stroke(P([(176, 236), (196, 262), (204, 296)]), w * 0.5, 0.22, 2)
    if detail > 0:
        def inside(x, y):
            u, v = (x - ox) / s, (y - oy) / s
            edge = 255 + (v - 192) * 0.33  # follows the back line
            return 190 < v < 500 and edge - 70 < u < edge - 6
        ch.hatch(ox + 150 * s, oy + 190 * s, ox + 360 * s, oy + 500 * s, int(260 * detail), -1.05, 22 * s, 0.17, 1.2 * s, inside)
        def inside2(x, y):
            u, v = (x - ox) / s, (y - oy) / s
            return 234 < u < 256 and 100 < v < 180
        ch.hatch(ox + 230 * s, oy + 100 * s, ox + 260 * s, oy + 185 * s, int(40 * detail), -1.05, 14 * s, 0.18, 1.1 * s, inside2)

def cat(ch, ox, oy, s, detail=1.0):
    """Original sitting cat seen from behind, head tilted up toward the sky. Box ~400x520 units."""
    P = lambda pts: [(ox + x * s, oy + y * s) for x, y in pts]
    def R(pts, ang=-0.26, cx=205, cy=120):
        c, sn = np.cos(ang), np.sin(ang)
        return [(cx + (x - cx) * c - (y - cy) * sn, cy + (x - cx) * sn + (y - cy) * c) for x, y in pts]
    PH = lambda pts: P(R(pts))
    w = 2.6 * s * 1.4
    # head: left cheek -> left ear -> crown -> right ear -> right cheek
    ch.stroke(PH([(150, 158), (138, 128), (140, 98), (156, 72)]), w, 0.6)
    ch.stroke(PH([(156, 72), (150, 44), (148, 18), (166, 34), (186, 54)]), w, 0.6)
    ch.stroke(PH([(186, 54), (202, 56), (218, 52)]), w * 0.8, 0.45)
    ch.stroke(PH([(218, 52), (236, 30), (254, 12), (256, 40), (252, 70)]), w, 0.6)
    ch.stroke(PH([(252, 70), (266, 102), (266, 132), (256, 160)]), w, 0.6)
    # inner ear strokes
    ch.stroke(PH([(158, 60), (156, 36), (172, 52)]), w * 0.55, 0.3, 2)
    ch.stroke(PH([(232, 50), (248, 28), (248, 58)]), w * 0.55, 0.3, 2)
    # whiskers poking out on the left
    for (a, b) in [((140, 118), (96, 104)), ((141, 126), (98, 124)), ((143, 134), (104, 144))]:
        ch.stroke(PH([a, b]), w * 0.35, 0.35, 1, 0.5)
    # body
    ch.stroke(P([(150, 160), (138, 212), (120, 272), (104, 340), (88, 400), (76, 450), (84, 490), (120, 506)]), w, 0.6)
    ch.stroke(P([(256, 160), (272, 212), (292, 280), (316, 350), (334, 410), (340, 460), (326, 494), (290, 508)]), w, 0.6)
    ch.stroke(P([(120, 506), (170, 512), (230, 513), (290, 508)]), w * 0.8, 0.45)
    # long tail lying on the ground, curling up at the tip
    ch.stroke(P([(292, 508), (346, 514), (388, 504), (408, 478), (402, 452), (390, 448)]), w * 0.9, 0.55)
    ch.stroke(P([(296, 497), (344, 501), (378, 492), (392, 472), (390, 456)]), w * 0.75, 0.45)
    # inner contours
    ch.stroke(P([(326, 480), (312, 422), (284, 392)]), w * 0.7, 0.4)
    ch.stroke(P([(88, 480), (100, 430), (124, 398)]), w * 0.7, 0.4)
    ch.stroke(P([(206, 170), (204, 260), (206, 360)]), w * 0.5, 0.2, 2)
    if detail > 0:
        def inside(x, y):
            u, v = (x - ox) / s, (y - oy) / s
            edge = 256 + (v - 160) * 0.25
            return 165 < v < 500 and edge - 66 < u < edge - 8
        ch.hatch(ox + 150 * s, oy + 160 * s, ox + 350 * s, oy + 500 * s, int(260 * detail), -1.05, 22 * s, 0.17, 1.2 * s, inside)
        def inside2(x, y):
            u, v = (x - ox) / s, (y - oy) / s
            return 228 < u < 258 and 80 < v < 155
        ch.hatch(ox + 225 * s, oy + 80 * s, ox + 262 * s, oy + 158 * s, int(40 * detail), -1.05, 14 * s, 0.18, 1.1 * s, inside2)

def ground(ch, ox, oy, s, amt):
    rng = ch.rng
    # smudged cast shadow (light from the sky, upper left -> shadow to the right, flattened = depth)
    yy, xx = np.mgrid[0:H, 0:W]
    cx, cy = ox + 290 * s, oy + 522 * s
    e = ((xx - cx) / (230 * s)) ** 2 + ((yy - cy) / (26 * s)) ** 2
    sm = np.clip(1 - e, 0, 1) ** 1.2 * 0.38 * (0.6 + 0.4 * amt)
    ch.smudge = gaussian_filter(sm, 6 * s)
    # a few loose ground strokes, fading with distance
    for i in range(int(14 * amt)):
        y = oy + rng.uniform(512, 560) * s
        x0 = ox + rng.uniform(-40, 260) * s
        ch.stroke([(x0, y), (x0 + rng.uniform(80, 220) * s, y + rng.normal(0, 1.5))], 1.4 * s, 0.13, 1, 0.4)
    for i in range(int(6 * amt)):
        y = oy + rng.uniform(560, 610) * s
        x0 = ox + rng.uniform(-260, 120) * s
        ch.stroke([(x0, y), (x0 + rng.uniform(120, 300) * s, y + rng.normal(0, 1))], 1.2 * s, 0.07, 1, 0.3)

def sky(ch, ox, oy, s, rng):
    # a few sketched stars + a thin crescent the dog is looking at
    for (u, v, r) in [(-180, -60, 14), (-60, -170, 10), (60, -120, 11), (-280, -190, 8), (-380, -80, 10), (-470, -210, 7)]:
        x, y = ox + u * s, oy + v * s
        for a in (0, np.pi / 2, np.pi / 4, 3 * np.pi / 4):
            k = r * s * (1 if a in (0, np.pi / 2) else 0.55)
            ch.stroke([(x - np.cos(a) * k, y - np.sin(a) * k), (x + np.cos(a) * k, y + np.sin(a) * k)], 1.1 * s, 0.35, 1, 0.3, False)
    cx, cy, R = ox - 140 * s, oy - 190 * s, 34 * s
    th = np.linspace(-1.9, 1.9, 14)
    outer = [(cx + R * np.cos(t), cy + R * np.sin(t)) for t in th]
    inner = [(cx + R * 0.45 + R * 0.8 * np.cos(t), cy + R * 0.8 * np.sin(t)) for t in th[::-1]]
    ch.stroke(outer, 1.5 * s * 1.4, 0.45, 2, 0.6)
    ch.stroke(inner, 1.3 * s * 1.4, 0.35, 2, 0.6)

def render(opt, seed):
    rng = np.random.default_rng(seed)
    cfg = {
        'A': dict(base=(236, 235, 231), s=0.95, ground=0.6, sky=False, dark=(44, 44, 46), fib=0.7, tex=0.8, horizon=False),
        'B': dict(base=(226, 223, 215), s=1.06, ground=1.0, sky=False, dark=(32, 31, 32), fib=1.8, tex=1.8, horizon=True),
        'D': dict(base=(226, 223, 215), s=1.06, ground=1.0, sky=False, dark=(32, 31, 32), fib=1.8, tex=1.8, horizon=True, fig='cat'),
        'C': dict(base=(232, 232, 230), s=0.95, ground=0.7, sky=True, dark=(46, 47, 52), fib=1.0, tex=1.1, horizon=True),
    }[opt]
    img = paper(rng, cfg['base'], cfg['fib'], cfg['tex'])
    ch = Charcoal(rng)
    s = cfg['s']
    ox, oy = W - 430 * s - 60, H - 560 * s - 30  # bottom-right anchor
    ground(ch, ox, oy, s, cfg['ground'])
    if cfg['horizon']:
        hy = oy + 470 * s
        for k in range(3):
            ch.stroke([(40, hy + rng.normal(0, 2)), (ox - 40 * s, hy + rng.normal(0, 2))], 1.0, 0.06, 1, 1.0)
    (cat if cfg.get('fig') == 'cat' else dog)(ch, ox, oy, s)
    if cfg['sky']: sky(ch, ox + 120 * s, oy + 40 * s, s, rng)
    ink = ch.ink * ch.tooth
    ink = 1 - (1 - ink) * (1 - getattr(ch, 'smudge', 0) * (0.7 + 0.3 * ch.tooth))
    ink = gaussian_filter(ink, 0.5)
    dark = np.array(cfg['dark'], float)
    out = img * (1 - ink[..., None]) + dark * ink[..., None]
    out = np.clip(out, 0, 255).astype(np.uint8)
    Image.fromarray(out).save(f'bg_light_{opt}.png')
    Image.fromarray(out).convert('RGB').save(f'bg_light_{opt}.jpg', quality=90)

if __name__ == '__main__':
  for o, sd in zip(sys.argv[1:] or 'ABC', [11, 22, 33]):
      render(o, sd)
      print('done', o)
