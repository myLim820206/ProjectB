"""칵테일 잔 15종 (잔 3 × 색 5) — Alice 시안 잔(docs/images/ui-1120/reference.webp 칵테일 칸)과 같은 퀄리티로 (D-422)

    python tools/ui_art/cocktails.py            → Content/UIArt/cock_<잔>_<색>.png + docs/images/ui-1120/cocktails_preview.png

잔 (게임 규칙: 칵테일 번호 % 3): 0 마티니(V 잔 · 반쪽 오렌지 · 꼬치 · 술 물결 · 얼음), 1 하이볼(긴 잔 · 라임 바퀴 · 빨대 · 기포), 2 온더록(낮은 잔 · 큰 얼음 둘 · 오렌지 껍질)
색 (칵테일 번호 % 5): 잔 네온 색 + 술 색 짝 — 0 분홍/파랑(시안 그대로), 1 은빛/금빛, 2 금빛/빨강, 3 하늘/초록, 4 보라/분홍
그리는 법: 검은 바탕에 빛을 더해 그린 뒤(번짐 + 굵은 관 + 밝은 심), 알파 = 가장 밝은 채널로 바꿔 미리 곱한 알파 PNG 로 저장 (게임 붓은 PreMultipliedAlpha)
"""
import math
import os

import numpy as np
from PIL import Image, ImageChops, ImageDraw, ImageFilter

ROOT = os.path.join(os.path.dirname(__file__), '..', '..')
OUT = os.path.join(ROOT, 'Content', 'UIArt')
PREVIEW = os.path.join(ROOT, 'docs', 'images', 'ui-1120', 'cocktails_preview.png')
W, H = 240, 300        # 저장 크기 (게임 68 × 85 의 약 3.5배)
SS = 3                 # 그릴 때 더 크게 그려 줄임 (매끈하게)

GLASS = [(255, 90, 200), (225, 232, 245), (255, 205, 80), (110, 225, 255), (190, 130, 255)]
LIQUID = [(90, 170, 255), (255, 200, 80), (255, 80, 90), (120, 240, 150), (255, 110, 180)]
ORANGE = (255, 150, 45)
LIME = (190, 240, 70)


def lighten(c, k): return tuple(int(v + (255 - v) * k) for v in c)


class Neon:
    def __init__(self):
        self.size = (W * SS, H * SS)
        self.glow = Image.new('RGB', self.size, (0, 0, 0))
        self.core = Image.new('RGB', self.size, (0, 0, 0))
        self.fill = Image.new('RGB', self.size, (0, 0, 0))
        self.g, self.c, self.f = ImageDraw.Draw(self.glow), ImageDraw.Draw(self.core), ImageDraw.Draw(self.fill)

    def P(self, pts): return [(x * SS, y * SS) for x, y in pts]

    def tube(self, pts, col, wd=5.0, glow=1.0):
        P = self.P(pts)
        self.g.line(P, fill=tuple(int(v * 0.85 * glow) for v in col), width=int(wd * SS * 3.2), joint='curve')
        self.c.line(P, fill=col, width=int(wd * SS), joint='curve')
        self.c.line(P, fill=lighten(col, 0.55), width=max(1, int(wd * SS * 0.35)), joint='curve')
        for p in (P[0], P[-1]):  # 둥근 끝
            r = wd * SS / 2
            self.c.ellipse([p[0] - r, p[1] - r, p[0] + r, p[1] + r], fill=col)

    def poly(self, pts, col, a):
        self.f.polygon(self.P(pts), fill=tuple(int(v * a) for v in col))

    def disc(self, cx, cy, r, col, a=1.0):
        self.f.ellipse([(cx - r) * SS, (cy - r) * SS, (cx + r) * SS, (cy + r) * SS], fill=tuple(int(v * a) for v in col))

    def done(self):
        img = ImageChops.add(self.fill, self.glow.filter(ImageFilter.GaussianBlur(9 * SS)).point(lambda v: int(v * 0.75)))
        img = ImageChops.add(img, self.glow.filter(ImageFilter.GaussianBlur(3 * SS)).point(lambda v: int(v * 0.45)))
        img = ImageChops.lighter(img, self.core)
        img = img.resize((W, H), Image.LANCZOS)
        a = np.asarray(img).astype(np.float32) / 255.0
        alpha = a.max(axis=2)
        return Image.fromarray((np.dstack([a, alpha]) * 255 + 0.5).astype(np.uint8), 'RGBA')


def circle(cx, cy, r, a0=0, a1=360, n=40):
    return [(cx + math.cos(math.radians(a0 + (a1 - a0) * k / n)) * r, cy + math.sin(math.radians(a0 + (a1 - a0) * k / n)) * r) for k in range(n + 1)]


def rot(pts, cx, cy, deg):
    a = math.radians(deg); c, s = math.cos(a), math.sin(a)
    return [(cx + (x - cx) * c - (y - cy) * s, cy + (x - cx) * s + (y - cy) * c) for x, y in pts]


def cube(N, cx, cy, s, deg, col):
    q = rot([(cx - s, cy - s), (cx + s, cy - s), (cx + s, cy + s), (cx - s, cy + s), (cx - s, cy - s)], cx, cy, deg)
    N.poly(q, lighten(col, 0.5), 0.18)
    N.tube(q, lighten(col, 0.35), 3.0, 0.6)


def wave(x0, x1, y, amp=4, n=4):
    return [(x0 + (x1 - x0) * k / 24, y + math.sin(k / 24 * n * math.pi) * amp) for k in range(25)]


def citrus(N, cx, cy, r, col, deg=-15, half=True):
    """채운 감귤 조각 (반쪽 또는 바퀴): 속은 채우고, 껍질은 밝은 관, 결 선은 어둡게"""
    a0, a1 = (180, 360) if half else (0, 360)
    rim = rot(circle(cx, cy, r, a0, a1, 30), cx, cy, deg)
    inner = rot(circle(cx, cy, r * 0.78, a0, a1, 30), cx, cy, deg)
    N.poly(rim + ([rim[0]] if half else []), col, 0.95)
    N.poly(inner + ([inner[0]] if half else []), lighten(col, 0.25), 0.95)
    for k in range(1, 4 if half else 6):
        ang = a0 + (a1 - a0) * k / (4 if half else 6)
        p = rot([(cx, cy), (cx + math.cos(math.radians(ang)) * r * 0.78, cy + math.sin(math.radians(ang)) * r * 0.78)], cx, cy, deg)
        N.f.line(N.P(p), fill=tuple(int(v * 0.55) for v in col), width=int(1.6 * SS))
    N.tube(rim + ([rim[0]] if half else []), lighten(col, 0.15), 3.2, 0.7)


def martini(gi):
    N = Neon()
    G, L = GLASS[gi], LIQUID[gi]
    rimL, rimR, tip = (30, 92), (210, 86), (122, 186)
    # 술: 테 아래 V 안을 채우고 물결
    yl = 106
    def on_edge(p, q, y): return (p[0] + (q[0] - p[0]) * (y - p[1]) / (q[1] - p[1]), y)
    l0, r0 = on_edge(rimL, tip, yl), on_edge(rimR, tip, yl)
    N.poly([l0, r0, tip], L, 0.42)
    N.tube(wave(l0[0] + 6, r0[0] - 6, yl, 4, 3), lighten(L, 0.2), 3.2, 0.7)
    cube(N, 108, 130, 10, 20, L); cube(N, 132, 148, 9, -15, L)
    # 잔
    N.tube([rimL, rimR, tip, rimL], G, 6.0)
    N.tube([tip, (122, 252)], G, 5.5)
    N.tube([(122, 252), (86, 268), (158, 268), (122, 252)], G, 5.5)
    # 꼬치 + 구슬, 반쪽 오렌지
    N.tube([(140, 150), (176, 60)], G, 3.6)
    N.tube(circle(180, 50, 10), G, 3.6)
    citrus(N, 50, 86, 34, ORANGE, -18, True)
    return N.done()


def highball(gi):
    N = Neon()
    G, L = GLASS[gi], LIQUID[gi]
    TL, TR, BL, BR = (66, 70), (174, 70), (74, 262), (166, 262)
    def x_at(p, q, y): return p[0] + (q[0] - p[0]) * (y - p[1]) / (q[1] - p[1])
    yl = 104
    N.poly([(x_at(TL, BL, yl), yl), (x_at(TR, BR, yl), yl), (BR[0] - 3, 250), (BL[0] + 3, 250)], L, 0.4)
    N.tube(wave(x_at(TL, BL, yl) + 6, x_at(TR, BR, yl) - 6, yl, 3.5, 4), lighten(L, 0.2), 3.2, 0.7)
    for (bx, by, br) in ((100, 200, 5), (128, 168, 4), (146, 222, 6), (112, 140, 3.5), (138, 128, 3)):  # 기포
        N.tube(circle(bx, by, br, n=18), lighten(L, 0.4), 2.2, 0.5)
    cube(N, 104, 236, 11, 12, L)
    # 잔 (두꺼운 바닥)
    N.tube([TL, BL, BR, TR], G, 6.0)
    N.tube([(BL[0] + 4, 248), (BR[0] - 4, 248)], G, 3.0, 0.6)
    # 빨대 · 라임 바퀴
    N.tube([(132, 210), (160, 30), (186, 22)], lighten(G, 0.3), 4.0, 0.8)
    citrus(N, 176, 72, 26, LIME, 0, False)
    return N.done()


def rocks(gi):
    N = Neon()
    G, L = GLASS[gi], LIQUID[gi]
    TL, TR, BL, BR = (44, 140), (196, 140), (54, 262), (186, 262)
    def x_at(p, q, y): return p[0] + (q[0] - p[0]) * (y - p[1]) / (q[1] - p[1])
    yl = 172
    N.poly([(x_at(TL, BL, yl), yl), (x_at(TR, BR, yl), yl), (BR[0] - 4, 244), (BL[0] + 4, 244)], L, 0.42)
    N.tube(wave(x_at(TL, BL, yl) + 6, x_at(TR, BR, yl) - 6, yl, 3.5, 3), lighten(L, 0.2), 3.2, 0.7)
    cube(N, 96, 196, 22, 12, L); cube(N, 146, 210, 20, -10, L)
    N.tube([TL, BL, BR, TR], G, 6.0)
    N.tube([(BL[0] + 4, 242), (BR[0] - 4, 242)], G, 3.4, 0.6)
    # 오렌지 껍질 꼬임 (테에 걸침)
    peel = [(150, 150), (164, 128), (180, 122), (192, 132), (186, 150), (172, 152), (170, 140), (182, 112), (200, 98)]
    N.tube(peel, ORANGE, 5.0)
    return N.done()


KINDS = [martini, highball, rocks]


def main():
    os.makedirs(OUT, exist_ok=True)
    sheet = Image.new('RGB', (5 * W + 60, 4 * H + 40), (14, 9, 30))
    for k, fn in enumerate(KINDS):
        for gi in range(5):
            im = fn(gi)
            im.save(os.path.join(OUT, 'cock_%d_%d.png' % (k, gi)))
            a = np.asarray(im).astype(np.float32) / 255
            x, y = 20 + gi * (W + 4), 20 + k * (H + 4)
            bg = np.asarray(sheet.crop((x, y, x + W, y + H))).astype(np.float32) / 255
            o = a[..., :3] + bg * (1 - a[..., 3:])
            sheet.paste(Image.fromarray((o * 255).astype(np.uint8)), (x, y))
    # 꺼진 잔 (가지지 않은 칸): 같은 그림을 어둡게
    for gi in range(5):
        a = np.asarray(Image.open(os.path.join(OUT, 'cock_0_%d.png' % gi))).astype(np.float32) / 255 * 0.3
        x, y = 20 + gi * (W + 4), 20 + 3 * (H + 4)
        bg = np.asarray(sheet.crop((x, y, x + W, y + H))).astype(np.float32) / 255
        sheet.paste(Image.fromarray(((a[..., :3] + bg * (1 - a[..., 3:])) * 255).astype(np.uint8)), (x, y))
    sheet.save(PREVIEW)
    print('ok', PREVIEW)


if __name__ == '__main__':
    main()
