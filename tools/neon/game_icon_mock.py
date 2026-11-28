"""게임 아이콘 시안 (실행파일 · 스토어 아이콘) — 네온 간판 화풍

    python3 tools/neon/game_icon_mock.py 출력폴더

A 마티니: 간판의 금빛 마티니 잔 (하늘 술 · 얼음 · 분홍 꼬치 · 반짝이)
B 필기체 A: 간판 Alice 의 첫 글자 A (분홍) + 금빛 밑줄
C 조커 카드: 분홍 카드 틀 안 광대 머리 (조커 j1)
D 카드 속 잔: 하늘 카드 틀 안 금빛 마티니, 올리브 자리에 분홍 스페이드

좌표는 100 x 100 칸. 크기마다 따로 그린다: 64px 미만은 잔 글자 · 반짝이 같은 세부를 빼고 선을 굵게 한다.
결과: icon_<X>_1024.png, 비교판 icon_sheet.png, 바탕화면 · 작업표시줄 흉내 icon_context.png
"""
import math
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

sys.path.insert(0, os.path.dirname(__file__))
from logo_strokes import catmull  # noqa: E402
from logo_alice import ALICE  # noqa: E402
from neon import suit, joker, circle, bez  # noqa: E402

PINK, CYAN, GOLD, WHITE = (255, 106, 190), (110, 220, 255), (255, 214, 90), (255, 255, 255)
BG0, BG1 = (6, 6, 11), (34, 12, 44)


def rot(pts, cx, cy, deg):
    a = math.radians(deg)
    c, s = math.cos(a), math.sin(a)
    return [(cx + (x - cx) * c - (y - cy) * s, cy + (x - cx) * s + (y - cy) * c) for x, y in pts]


def rrect(x0, y0, x1, y1, r, n=8):
    P = []
    for cx, cy, a0 in ((x1 - r, y0 + r, -90), (x1 - r, y1 - r, 0), (x0 + r, y1 - r, 90), (x0 + r, y0 + r, 180)):
        for k in range(n + 1):
            a = math.radians(a0 + k * 90 / n)
            P.append((cx + math.cos(a) * r, cy + math.sin(a) * r))
    P.append(P[0])
    return P


# ------------------------------------------------------------- 시안 (선 목록: (점들, 색, 굵기, 작은 크기에도 그림?))
def martini(cx, top, w, h, col, olive='ice', small=False):
    """잔: 역삼각 잔 + 다리 + 받침. w = 잔 반 너비, h = 전체 높이"""
    bowl_h = h * 0.45
    L = []
    L.append(([(cx - w, top), (cx + w, top), (cx, top + bowl_h), (cx - w, top)], col, 1.0, True))
    L.append(([(cx, top + bowl_h), (cx, top + h * 0.9)], col, 1.0, True))
    L.append(([(cx - w * 0.55, top + h), (cx + w * 0.55, top + h), (cx + w * 0.2, top + h * 0.9), (cx - w * 0.2, top + h * 0.9), (cx - w * 0.55, top + h)], col, 1.0, True))
    ly = top + bowl_h * 0.25
    lw = w * (1 - 0.25) - w * 0.1
    if olive == 'ice':
        L.append(([(cx - lw, ly), (cx + lw, ly)], CYAN, 0.85, True))
        s = w * 0.15
        ix, iy = cx + w * 0.08, top + bowl_h * 0.55
        L.append(([(ix, iy - s), (ix + s, iy), (ix, iy + s), (ix - s, iy), (ix, iy - s)], CYAN, 0.8, False))
        L.append(([(cx + w * 0.06, top + bowl_h * 0.4), (cx + w * 0.3, top - h * 0.14)], PINK, 0.85, True))
    else:  # 스페이드 올리브 + 꼬치
        for p in suit('spade')(cx + w * 0.02, top + bowl_h * 0.42, w * 0.26):
            L.append((p, PINK, 0.8, True))
        L.append(([(cx + w * 0.22, top + bowl_h * 0.3), (cx + w * 0.62, top - h * 0.22)], PINK, 0.7, False))
    return L


def sparks(cx, cy, r, col):
    L = []
    for a in (-80, -45, -10):
        t = math.radians(a)
        L.append(([(cx + math.cos(t) * r * 0.45, cy + math.sin(t) * r * 0.45), (cx + math.cos(t) * r, cy + math.sin(t) * r)], col, 0.9, False))
    return L


def icon_A():
    L = martini(48, 30, 27, 58, GOLD)
    L += sparks(70, 26, 17, GOLD)
    return L


def icon_B():
    # 간판의 A (x 0..74, y -90..4) → 칸 안으로
    k, ox, oy = 0.86, 19, 80
    L = []
    for st in ALICE[0]:
        L.append(([(ox + x * k, oy + y * k) for x, y in catmull(st)], PINK, 1.15, True))
    L.append(([(x, y) for x, y in catmull([(14, 90), (40, 86), (66, 84), (88, 83)])], GOLD, 0.8, True))
    return L


def icon_C():
    card = rot(rrect(25, 14, 75, 86, 7), 50, 50, -8)
    L = [(card, PINK, 0.9, True)]
    for p in joker(50, 50, 21):
        L.append((rot(p, 50, 50, -8), PINK, 0.85, True))
    # 모서리 별 (조커 표시)
    for cx, cy in ((33, 23), (67, 77)):
        for p in suit('star')(cx, cy, 4.2):
            L.append((rot(p, 50, 50, -8), WHITE, 0.6, False))
    return L


def icon_D():
    card = rot(rrect(22, 12, 78, 88, 8), 50, 50, -6)
    L = [(card, CYAN, 0.9, True)]
    for pts, c, w, sm in martini(50, 33, 19, 40, GOLD, olive='spade'):
        L.append((rot(pts, 50, 50, -6), c, w, sm))
    # 카드 모서리 A
    L.append(('A', CYAN, (32, 23), (69, 78)))
    return L


ICONS = {'A': ('마티니 잔', icon_A), 'B': ('필기체 A', icon_B), 'C': ('조커 카드', icon_C), 'D': ('카드 속 잔', icon_D)}


# ------------------------------------------------------------- 그리기
def _bg(N):
    yy, xx = np.mgrid[0:N, 0:N] / N
    d = np.sqrt((xx - 0.5) ** 2 + (yy - 0.42) ** 2)
    t = np.clip(1 - d / 0.75, 0, 1) ** 1.6
    img = np.zeros((N, N, 3), np.float32)
    for i in range(3):
        img[..., i] = BG0[i] + (BG1[i] - BG0[i]) * t
    if N >= 96:  # 벽돌 (큰 크기만)
        g = Image.new('L', (N, N), 0)
        d2 = ImageDraw.Draw(g)
        rows = 6
        hh = N / rows
        lw = max(1, N // 256)
        for r in range(rows + 1):
            d2.line([(0, r * hh), (N, r * hh)], 255, lw)
            off = (r % 2) * N / 4
            for c in range(3):
                x = off + c * N / 2
                d2.line([(x, r * hh), (x, (r + 1) * hh)], 255, lw)
        a = np.asarray(g, np.float32)[..., None] / 255
        img = img * (1 - a * 0.35)
    return img


def render(name, N, ss=4):
    small = N < 64
    S = N * ss
    sc = S / 100
    base = 3.4 if not small else (5.6 if N <= 16 else 4.6)  # 기본 굵기 (100 칸 단위)
    items = ICONS[name][1]()
    core = Image.new('RGB', (S, S), 0)
    hot = Image.new('RGB', (S, S), 0)
    cd, hd = ImageDraw.Draw(core), ImageDraw.Draw(hot)
    font = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf', int(9 * sc))
    for it in items:
        if it[0] == 'A':
            if small: continue
            _, c, *pos = it
            for (x, y) in pos:
                cd.text((x * sc, y * sc), 'A', fill=c, font=font, anchor='mm')
                hd.text((x * sc, y * sc), 'A', fill=tuple(int(v * 0.5 + 127) for v in c), font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf', int(8 * sc)), anchor='mm')
            continue
        pts, c, w, keep = it
        if small and not keep: continue
        P = [(x * sc, y * sc) for x, y in pts]
        wp = max(1, int(base * w * sc))
        cd.line(P, fill=c, width=wp, joint='curve')
        for p in (P[0], P[-1]):
            cd.ellipse([p[0] - wp / 2, p[1] - wp / 2, p[0] + wp / 2, p[1] + wp / 2], fill=c)
        hw = max(1, int(wp * 0.38))
        hc = tuple(int(v * 0.35 + 255 * 0.65) for v in c)
        hd.line(P, fill=hc, width=hw, joint='curve')
    core_s = core.resize((N, N), Image.LANCZOS)
    hot_s = hot.resize((N, N), Image.LANCZOS)
    a = np.asarray(core_s, np.float32)
    h = np.asarray(hot_s, np.float32)
    img = _bg(N)
    gk = 0.45 if small else 1.0
    for rad, amt in ((N * 0.012, 0.9), (N * 0.035, 0.75), (N * 0.08, 0.55)):
        g = np.asarray(core_s.filter(ImageFilter.GaussianBlur(max(0.6, rad))), np.float32)
        img += g * amt * gk
    img = img * (1 - a.max(axis=2, keepdims=True) / 255 * 0.0) + a
    hm = h.max(axis=2, keepdims=True) / 255
    img = img * (1 - hm) + h * hm + img * hm * 0.25
    img = np.clip(img, 0, 255).astype(np.uint8)
    out = Image.fromarray(img).convert('RGBA')
    # 둥근 사각 타일 (모서리 투명)
    m = Image.new('L', (S, S), 0)
    ImageDraw.Draw(m).rounded_rectangle([0, 0, S - 1, S - 1], radius=int(S * 0.2), fill=255)
    out.putalpha(m.resize((N, N), Image.LANCZOS))
    return out


def kfont(px):
    return ImageFont.truetype('/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc', px)


def sheet(outdir, icons):
    keys = list(ICONS)
    cols = [('256 px', 256, 1), ('32 px', 32, 1), ('16 px', 16, 1), ('32 px 확대 ×4', 32, 4), ('16 px 확대 ×8', 16, 8)]
    colw = [280, 70, 60, 150, 150]
    W = 200 + sum(colw) + 40
    rowh = 300
    H = 90 + rowh * len(keys)
    img = Image.new('RGB', (W, H), (18, 18, 24))
    d = ImageDraw.Draw(img)
    d.text((24, 22), '바 앨리스 게임 아이콘 시안 — 실제 크기와 확대', fill=(220, 225, 235), font=kfont(30))
    x = 200
    for (t, _, _), cw in zip(cols, colw):
        d.text((x + cw // 2, 72), t, fill=(150, 158, 175), font=kfont(16), anchor='mm')
        x += cw
    for r, k in enumerate(keys):
        y0 = 90 + r * rowh
        d.text((24, y0 + rowh // 2 - 18), k, fill=(255, 255, 255), font=kfont(44), anchor='lm')
        d.text((24, y0 + rowh // 2 + 26), ICONS[k][0], fill=(200, 205, 215), font=kfont(22), anchor='lm')
        x = 200
        for (t, n, z), cw in zip(cols, colw):
            ic = icons[k][n]
            if z > 1: ic = ic.resize((n * z, n * z), Image.NEAREST)
            img.paste(ic, (x + (cw - ic.width) // 2, y0 + (rowh - ic.height) // 2), ic)
            x += cw
    img.save(os.path.join(outdir, 'icon_sheet.png'))


def context(outdir, icons):
    """바탕화면 · 작업표시줄 · 스팀 목록에서 보이는 모습 흉내"""
    keys = list(ICONS)
    W, H = 1200, 420
    img = Image.new('RGB', (W, H), (40, 52, 78))
    d = ImageDraw.Draw(img)
    # 바탕화면 아이콘 (48px) 줄
    d.text((24, 16), '바탕화면 (48px)', fill=(230, 235, 245), font=kfont(18))
    for i, k in enumerate(keys):
        x = 40 + i * 140
        ic = icons[k][48]
        img.paste(ic, (x + 26, 50), ic)
        d.text((x + 50, 112), f'{k} 바 앨리스', fill=WHITE, font=kfont(14), anchor='mm')
    # 작업표시줄 (24px, 어두운 막대)
    d.rectangle([0, 160, W, 208], fill=(28, 28, 32))
    d.text((24, 140), '작업표시줄 (24px)', fill=(230, 235, 245), font=kfont(18))
    for i, k in enumerate(keys):
        x = 40 + i * 60
        ic = icons[k][24]
        img.paste(ic, (x, 172), ic)
        d.text((x + 12, 216), k, fill=(220, 220, 220), font=kfont(14), anchor='mt')
    # 스팀 라이브러리 목록 (16px 줄)
    d.rectangle([0, 250, W, H], fill=(27, 40, 56))
    d.text((24, 260), '스팀 라이브러리 목록 (16px)', fill=(200, 210, 225), font=kfont(18))
    for i, k in enumerate(keys):
        y = 296 + i * 28
        ic = icons[k][16]
        img.paste(ic, (40, y), ic)
        d.text((66, y + 8), f'Bar Alice: Late Night Poker  ({k})', fill=(190, 200, 215), font=kfont(15), anchor='lm')
    # 스팀 큰 칸 (64px)
    d.text((620, 260), '스팀 · 상점 작은 칸 (64px)', fill=(200, 210, 225), font=kfont(18))
    for i, k in enumerate(keys):
        ic = icons[k][64]
        img.paste(ic, (630 + i * 130, 300), ic)
    img.save(os.path.join(outdir, 'icon_context.png'))


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    sizes = [16, 24, 32, 48, 64, 256, 1024]
    icons = {k: {n: render(k, n) for n in sizes} for k in ICONS}
    for k in ICONS:
        icons[k][1024].save(os.path.join(outdir, f'icon_{k}_1024.png'))
    sheet(outdir, icons)
    context(outdir, icons)


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
