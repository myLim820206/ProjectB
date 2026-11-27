"""칵테일 잔 60종 (잔 12 × 색 5, D-423) — Alice 시안 잔(DOC/assets/images/ui-1120/reference.webp 칵테일 칸)과 같은 퀄리티로 (D-422)

    python tools/ui_art/cocktails.py            → Content/UIArt/cock_<잔>_<색>.png + DOC/assets/images/ui-1120/cocktails_preview.png

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
PREVIEW = os.path.join(ROOT, 'DOC', 'assets', 'images', 'ui-1120', 'cocktails_preview.png')
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
    # 오렌지 껍질 꼬임: 테에 걸쳐 늘어진 리본 나선 (D-423)
    for k, off in enumerate((0.0, math.pi)):
        pts = [(172 + 14 * math.cos(t * 2.2 + off) + t * 3, 112 + t * 12) for t in [i / 10 for i in range(0, 71)]]
        N.tube(pts, ORANGE if k == 0 else lighten(ORANGE, 0.3), 4.2 if k == 0 else 2.6, 0.8)
    return N.done()


def chaikin(P, n=3):
    """꺾은선을 부드럽게 (모서리 깎기 n 번)"""
    for _ in range(n):
        Q = [P[0]]
        for a, b in zip(P, P[1:]):
            Q += [(a[0] * 0.75 + b[0] * 0.25, a[1] * 0.75 + b[1] * 0.25), (a[0] * 0.25 + b[0] * 0.75, a[1] * 0.25 + b[1] * 0.75)]
        Q.append(P[-1]); P = Q
    return P


def mirror(left, cx=120):
    return left + [(2 * cx - x, y) for x, y in left[::-1]]


def coupe(gi):
    """쿠페: 넓고 얕은 그릇 + 가는 다리, 체리(꼭지)"""
    N = Neon(); G, L = GLASS[gi], LIQUID[gi]
    bowl = [(36, 110)] + [(120 + math.cos(math.radians(a)) * 84, 110 + math.sin(math.radians(a)) * 48) for a in range(180, -1, -6)][::-1] + [(204, 110)]
    bowl = [(120 + math.cos(math.radians(a)) * 84, 110 + math.sin(math.radians(a)) * 46) for a in range(0, 181, 6)]
    N.poly([(40, 120)] + [p for p in bowl if p[1] >= 120] + [(200, 120)], L, 0.42)
    N.tube(wave(44, 196, 122, 3.5, 4), lighten(L, 0.2), 3.0, 0.7)
    N.tube([(36, 110), (204, 110)] , G, 5.5)
    N.tube(bowl, G, 6.0)
    N.tube([(120, 156), (120, 250)], G, 5.0)
    N.tube([(120, 250), (82, 266), (158, 266), (120, 250)], G, 5.0)
    N.disc(150, 104, 13, (235, 40, 70)); N.tube(circle(150, 104, 13, n=24), (255, 90, 110), 3.0, 0.7)  # 체리
    N.tube([(150, 92), (158, 60), (176, 44)], (120, 220, 90), 3.0, 0.6)
    return N.done()


def margarita(gi):
    """마가리타: 계단 진 넓은 잔 + 소금 테(점) + 라임 조각"""
    N = Neon(); G, L = GLASS[gi], LIQUID[gi]
    rim = [(28, 92), (212, 92)]
    upper = [(28, 92), (60, 140), (180, 140), (212, 92)]
    lower = [(84, 140), (104, 176), (136, 176), (156, 140)]
    N.poly([(44, 112), (196, 112), (180, 140), (60, 140)], L, 0.42)
    N.poly([(88, 146), (152, 146), (136, 174), (104, 174)], L, 0.42)
    N.tube(wave(48, 192, 112, 3, 4), lighten(L, 0.2), 3.0, 0.7)
    N.tube(rim + [(212, 92)], G, 5.5); N.tube(upper, G, 6.0); N.tube(lower, G, 5.5)
    N.tube([(120, 176), (120, 252)], G, 5.0)
    N.tube([(120, 252), (84, 266), (156, 266), (120, 252)], G, 5.0)
    for k in range(18):  # 소금 테
        x = 32 + k * 10.4
        N.disc(x, 88 - (k % 2) * 3, 2.4, (245, 245, 255))
    citrus(N, 196, 92, 28, LIME, 35, True)
    return N.done()


def hurricane(gi):
    """허리케인: 잘록한 곡선 긴 잔 + 칵테일 우산"""
    N = Neon(); G, L = GLASS[gi], LIQUID[gi]
    left = [(78, 40), (70, 90), (92, 140), (72, 200), (84, 236), (106, 246)]
    right = [(162, 40), (170, 90), (148, 140), (168, 200), (156, 236), (134, 246)]
    from PIL import ImageDraw  # noqa
    import itertools  # noqa
    def smooth(P):
        out = []
        for i in range(len(P) - 1):
            a, b = P[i], P[i + 1]
            for k in range(6): out.append((a[0] + (b[0] - a[0]) * k / 6, a[1] + (b[1] - a[1]) * k / 6))
        out.append(P[-1]); return out
    L_s, R_s = smooth(left), smooth(right)
    N.poly([p for p in L_s if p[1] > 84] + [p for p in R_s if p[1] > 84][::-1], L, 0.42)
    N.tube(wave(72, 168, 86, 3, 3), lighten(L, 0.2), 3.0, 0.7)
    N.tube(L_s, G, 6.0); N.tube(R_s, G, 6.0)
    N.tube([(106, 246), (106, 258), (82, 268), (158, 268), (134, 258), (134, 246)], G, 5.0)
    # 우산
    canopy = [(150, 54), (176, 22), (206, 48)]
    N.poly([(146, 56), (176, 20), (210, 50)], (255, 120, 60), 0.85)
    N.tube(canopy + [(196, 54), (186, 46), (176, 56), (166, 46), (156, 56), (150, 54)], (255, 170, 80), 3.0, 0.7)
    N.tube([(176, 30), (150, 120)], (240, 220, 180), 3.0, 0.5)
    return N.done()


def flute(gi):
    """샴페인: 가늘고 긴 잔 + 올라가는 기포 + 딸기"""
    N = Neon(); G, L = GLASS[gi], LIQUID[gi]
    TL, TR, BL, BR = (96, 40), (144, 40), (110, 176), (130, 176)
    N.poly([(98, 64), (142, 64), (130, 172), (110, 172)], L, 0.45)
    N.tube(wave(100, 140, 64, 2.5, 3), lighten(L, 0.2), 2.8, 0.7)
    for (bx, by, br) in ((114, 150, 3), (124, 126, 3.5), (116, 102, 2.6), (128, 86, 2.2), (120, 74, 2)):
        N.tube(circle(bx, by, br, n=14), lighten(L, 0.45), 2.0, 0.5)
    N.tube([TL, (100, 120), BL, BR, (140, 120), TR], G, 5.5)
    N.tube([(120, 176), (120, 252)], G, 5.0)
    N.tube([(120, 252), (86, 266), (154, 266), (120, 252)], G, 5.0)
    berry = [(150, 30), (166, 34), (170, 50), (158, 66), (146, 54), (144, 38), (150, 30)]
    N.poly(berry, (240, 50, 80), 0.9); N.tube(berry, (255, 100, 120), 2.6, 0.6)
    N.tube([(148, 30), (156, 22), (164, 30)], (120, 220, 90), 2.6, 0.6)
    return N.done()


def wine(gi):
    """와인 잔: 위가 살짝 오므라든 튤립 그릇 + 다리, 술 반쯤"""
    N = Neon(); G, L = GLASS[gi], LIQUID[gi]
    bowl = chaikin(mirror([(82, 44), (68, 92), (64, 130), (80, 162), (104, 176), (120, 178)]))
    N.poly([p for p in bowl if p[1] >= 124], L, 0.45)
    N.tube(wave(66, 174, 124, 3, 3), lighten(L, 0.2), 3.0, 0.7)
    N.tube(bowl, G, 6.0)
    N.tube([(120, 178), (120, 252)], G, 5.0)
    N.tube([(120, 252), (82, 266), (158, 266), (120, 252)], G, 5.0)
    N.tube([(84, 78), (80, 112)], lighten(G, 0.6), 3.0, 0.5)  # 잔 빛
    return N.done()


def shots(gi):
    """샷 두 잔: 작은 두꺼운 잔 둘, 앞 잔엔 레몬 조각"""
    N = Neon(); G, L = GLASS[gi], LIQUID[gi]
    for (x0, y0, k) in ((44, 150, 0), (124, 170, 1)):
        TL, TR, BL, BR = (x0, y0), (x0 + 72, y0), (x0 + 8, y0 + 92), (x0 + 64, y0 + 92)
        N.poly([(x0 + 4, y0 + 22), (x0 + 68, y0 + 22), (x0 + 63, y0 + 80), (x0 + 9, y0 + 80)], L, 0.45)
        N.tube(wave(x0 + 6, x0 + 66, y0 + 22, 2.5, 2), lighten(L, 0.2), 2.8, 0.7)
        N.tube([TL, BL, BR, TR], G, 5.5)
        N.tube([(x0 + 10, y0 + 80), (x0 + 62, y0 + 80)], G, 3.0, 0.6)
    citrus(N, 188, 168, 22, (255, 225, 70), -30, True)
    return N.done()


def tiki(gi):
    """티키 머그: 얼굴 새긴 원통 + 잎 장식 + 빨대"""
    N = Neon(); G, L = GLASS[gi], LIQUID[gi]
    body = [(66, 80), (60, 150), (70, 250), (170, 250), (180, 150), (174, 80)]
    N.poly([(70, 96), (170, 96), (176, 150), (168, 244), (72, 244), (64, 150)], L, 0.25)
    N.tube(body, G, 6.0); N.tube([(66, 80), (174, 80)], G, 5.0)
    # 얼굴: 눈썹 · 눈 · 코 · 입
    N.tube([(82, 120), (110, 112)], G, 4.0); N.tube([(130, 112), (158, 120)], G, 4.0)
    N.tube(circle(98, 134, 8, n=18), G, 3.5); N.tube(circle(142, 134, 8, n=18), G, 3.5)
    N.tube([(120, 128), (110, 178), (130, 178)], G, 3.5)
    N.tube([(88, 200), (100, 216), (140, 216), (152, 200)], G, 4.0)
    for k in range(4): N.tube([(100 + k * 13, 204), (100 + k * 13, 214)], G, 2.5, 0.5)
    # 잎 셋 + 빨대
    for (ex, ey) in ((92, 40), (120, 30), (150, 42)):
        leaf = [(120, 80), ((120 + ex) / 2 - 8, (80 + ey) / 2), (ex, ey), ((120 + ex) / 2 + 8, (80 + ey) / 2), (120, 80)]
        N.poly(leaf, (90, 200, 90), 0.7); N.tube(leaf, (140, 240, 120), 2.6, 0.6)
    N.tube([(150, 120), (176, 30), (196, 24)], lighten(G, 0.3), 4.0, 0.8)
    return N.done()


def mule(gi):
    """구리 머그: 손잡이 달린 낮은 원통 + 민트 잎 + 라임"""
    N = Neon(); G, L = GLASS[gi], LIQUID[gi]
    body = [(52, 120), (58, 252), (162, 252), (168, 120)]
    N.poly([(56, 140), (164, 140), (160, 248), (60, 248)], L, 0.3)
    N.tube(body + [(52, 120)], G, 6.0)
    for k in range(3): N.tube([(58, 160 + k * 34), (162, 160 + k * 34)], G, 2.2, 0.4)  # 망치 무늬 띠
    N.tube([(168, 150), (204, 150), (210, 168), (210, 204), (204, 222), (166, 222)], G, 5.5)  # 손잡이
    for (cx, cy, d) in ((88, 104, -30), (112, 92, 10), (138, 104, 40)):  # 민트
        leaf = rot([(cx, cy + 14), (cx - 10, cy), (cx, cy - 18), (cx + 10, cy), (cx, cy + 14)], cx, cy, d)
        N.poly(leaf, (60, 210, 110), 0.8); N.tube(leaf, (130, 255, 160), 2.6, 0.6)
    citrus(N, 160, 112, 22, LIME, 0, False)
    return N.done()


def snifter(gi):
    """브랜디 잔: 위는 좁고 아래가 둥글게 부푼 큰 그릇 + 짧은 다리, 소용돌이 술"""
    N = Neon(); G, L = GLASS[gi], LIQUID[gi]
    bowl = chaikin(mirror([(94, 86), (74, 120), (60, 164), (70, 200), (96, 220), (120, 224)]))
    N.poly([p for p in bowl if p[1] >= 180], L, 0.45)
    N.tube(wave(64, 176, 180, 3, 3), lighten(L, 0.2), 3.0, 0.7)
    N.tube([(84, 200), (106, 192), (132, 202), (156, 194)], lighten(L, 0.35), 2.4, 0.5)  # 소용돌이
    N.tube(bowl, G, 6.0)
    N.tube([(120, 224), (120, 250)], G, 5.0)
    N.tube([(120, 250), (84, 266), (156, 266), (120, 250)], G, 5.0)
    N.tube([(82, 130), (76, 162)], lighten(G, 0.6), 3.0, 0.5)  # 잔 빛
    return N.done()


KINDS = [martini, highball, rocks, coupe, margarita, hurricane, flute, wine, shots, tiki, mule, snifter]  # 칵테일 번호 % 12 (D-423)
NAMES = ['마티니', '하이볼', '온더록', '쿠페', '마가리타', '허리케인', '샴페인', '와인', '샷', '티키', '구리 머그', '브랜디']


def main():
    os.makedirs(OUT, exist_ok=True)
    cw, ch = W // 2, H // 2
    sheet = Image.new('RGB', (5 * (cw + 6) + 140, 12 * (ch + 4) + 20), (14, 9, 30))
    from PIL import ImageDraw, ImageFont
    d = ImageDraw.Draw(sheet)
    fnt = ImageFont.truetype('C:/Windows/Fonts/malgunbd.ttf', 20)
    for k, fn in enumerate(KINDS):
        d.text((12, 10 + k * (ch + 4) + ch // 2), NAMES[k], font=fnt, fill=(228, 224, 205), anchor='lm')
        for gi in range(5):
            im = fn(gi)
            im.save(os.path.join(OUT, 'cock_%d_%d.png' % (k, gi)))
            sm = im.resize((cw, ch), Image.LANCZOS)
            a = np.asarray(sm).astype(np.float32) / 255
            x, y = 130 + gi * (cw + 6), 10 + k * (ch + 4)
            bg = np.asarray(sheet.crop((x, y, x + cw, y + ch))).astype(np.float32) / 255
            sheet.paste(Image.fromarray(((a[..., :3] + bg * (1 - a[..., 3:])) * 255).astype(np.uint8)), (x, y))
    sheet.save(PREVIEW)
    print('ok', PREVIEW)


if __name__ == '__main__':
    main()
