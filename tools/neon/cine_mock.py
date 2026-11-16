"""완성 연출 시안 (D-314, D-315) — 장면별 정지 화면 (언리얼 PDBoardCine.cpp 와 같은 흐름)

    python3 tools/neon/cine_mock.py 출력폴더   → cine_fuse.png · cine_joker.png · cine_upper.png (각 4장면 가로)
"""
import math
import os
import random
import sys

from PIL import ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, PINK, CYAN, GOLD  # noqa: E402
from neon import suit, circle  # noqa: E402

KO = '/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc'
WHITE, FG = (255, 255, 255), (220, 228, 240)
PAL = [PINK, CYAN, GOLD, (138, 255, 196), (180, 140, 255)]


def card(M, T, cx, cy, r, rank, col, w=1.0):
    M.rr(cx - r * w, cy - r, max(2, 2 * r * w), 2 * r, r * 0.22, col, 2.0, 1.0)
    if w > 0.5:
        T.append((cx, cy - r * 0.25, rank, col, int(r * 0.7), 'mm'))
        for p in suit('diamond')(cx, cy + r * 0.5, r * 0.28): M.ln(p, col, 1.4, 1.0)


def confetti(M, cx, cy, s, n, gold=False, seed=3):
    rnd = random.Random(seed)
    for i in range(n):
        a, v = rnd.uniform(0, math.tau), rnd.uniform(100, 300)
        x, y = cx + math.cos(a) * v * s, cy + math.sin(a) * v * s + 160 * s * s
        r, L = rnd.uniform(0, 6), rnd.uniform(3, 6)
        c = GOLD if gold else PAL[i % 5]
        M.ln([(x - math.cos(r) * L, y - math.sin(r) * L), (x + math.cos(r) * L, y + math.sin(r) * L)], c, 1.6, 1.0)


def firework(M, cx, cy, rad, col, rays=14):
    for k in range(rays):
        a = math.tau * k / rays
        M.ln([(cx + math.cos(a) * rad * 0.55, cy + math.sin(a) * rad * 0.55), (cx + math.cos(a) * rad, cy + math.sin(a) * rad + 6)], col, 1.8, 1.0)


def jcard(M, cx, cy, r, col=PINK):
    M.rr(cx - r, cy - r, 2 * r, 2 * r, r * 0.22, col, 2.0, 1.0)
    M.ln(circle(cx, cy - r * 0.1, r * 0.35, 20), col, 1.6, 1.0)
    M.ln([(cx - r * 0.5, cy - r * 0.35), (cx, cy - r * 0.8), (cx + r * 0.5, cy - r * 0.35)], col, 1.6, 1.0)


def crown(M, cx, cy, r):
    xs, ys = [-1, -1, -.5, 0, .5, 1, 1, -1], [0, -.8, -.4, -1, -.4, -.8, 0, 0]
    M.ln([(cx + x * r, cy + y * r) for x, y in zip(xs, ys)], GOLD, 2.2, 1.0)


def panel(M, T, i, title):
    x0 = 20 + i * 315
    M.rr(x0, 120, 300, 480, 14, (90, 96, 120), 1.0, 0.5)
    T.append((x0 + 150, 140, title, FG, 15, 'mm'))
    return x0 + 150, 360


def save(M, T, out, name, head):
    T.append((640, 60, head, FG, 18, 'mm'))
    img = M.draw()
    d = ImageDraw.Draw(img)
    for (x, y, s, c, px, a) in T:
        d.text((x, y), s, fill=c, font=ImageFont.truetype(KO, px), anchor=a)
    img.save(os.path.join(out, name))


def main(out):
    os.makedirs(out, exist_ok=True)
    # 합성
    M, T = Sign(), []
    cx, cy = panel(M, T, 0, '0.0초 · 칸에서 튀어나옴'); card(M, T, cx - 60, cy + 120, 20, '7', (255, 217, 106)); card(M, T, cx, cy, 44, '7', (255, 217, 106))
    cx, cy = panel(M, T, 1, '0.3초 · 빙글빙글'); card(M, T, cx, cy, 70, '7', (255, 217, 106), 0.35)
    cx, cy = panel(M, T, 2, '0.5초 · 펑 + 꽃가루'); M.ln(circle(cx, cy, 100, 40), (255, 217, 106), 2.2, 1.0); card(M, T, cx, cy, 70, '7', (255, 217, 106)); confetti(M, cx, cy, 0.35, 50); T.append((cx, cy + 105, 'Lv2', WHITE, 22, 'mm'))
    cx, cy = panel(M, T, 3, 'Lv4 는 금색 꽃가루'); card(M, T, cx, cy, 80, '7', GOLD); confetti(M, cx, cy, 0.45, 70, gold=True); T.append((cx, cy + 115, 'Lv4', GOLD, 24, 'mm'))
    save(M, T, out, 'cine_fuse.png', '합성 연출 (약 0.8초, 게임 멈춤, 클릭 = 건너뛰기)')
    # 조커
    M, T = Sign(), []
    cx, cy = panel(M, T, 0, '재료 카드가 앞으로')
    for k in range(5): a = math.tau * k / 5 - math.pi / 2; M.rr(cx + math.cos(a) * 100 - 14, cy + math.sin(a) * 100 - 18, 28, 36, 5, WHITE, 1.8, 1.0)
    cx, cy = panel(M, T, 1, '돌며 가운데로 모임')
    for k in range(5): a = math.tau * k / 5 + 1.2; M.rr(cx + math.cos(a) * 40 - 14, cy + math.sin(a) * 40 - 18, 28, 36, 5, WHITE, 1.8, 1.0)
    cx, cy = panel(M, T, 2, '하얀 빛')
    for rr in (30, 60, 90, 120): M.ln(circle(cx, cy, rr, 40), WHITE, 1.2, 1.0 - rr / 160)
    cx, cy = panel(M, T, 3, '조커 + 폭죽'); jcard(M, cx, cy, 46); firework(M, cx - 80, cy - 110, 60, PINK); firework(M, cx + 90, cy - 90, 60, GOLD); firework(M, cx, cy + 120, 50, PINK); T.append((cx, cy + 75, '지휘봉', PINK, 18, 'mm'))
    save(M, T, out, 'cine_joker.png', '조커 연출 (약 1.4초)')
    # 상위 조커
    M, T = Sign(), []
    cx, cy = panel(M, T, 0, '어두워짐 · 두 장이 돔'); jcard(M, cx - 90, cy, 28); jcard(M, cx + 90, cy + 20, 28)
    cx, cy = panel(M, T, 1, '부딪혀 금빛 번쩍')
    for rr in (40, 80, 120): M.ln(circle(cx, cy, rr, 40), GOLD, 1.4, 1.0 - rr / 160)
    cx, cy = panel(M, T, 2, '왕관이 내려와 앉음'); jcard(M, cx, cy, 52, GOLD); crown(M, cx, cy - 64, 22)
    for k in range(3): a = k * math.tau / 3; M.ln(circle(cx + math.cos(a) * 100, cy + math.sin(a) * 60, 7, 16), WHITE, 1.4, 1.0)
    cx, cy = panel(M, T, 3, '간판 전구 + 금색 폭죽'); jcard(M, cx, cy - 20, 44, GOLD); crown(M, cx, cy - 72, 18)
    M.rr(cx - 120, cy + 50, 240, 52, 14, GOLD, 2.0, 1.0)
    for k in range(16): M.ln(circle(cx - 112 + k * 15, cy + 56, 2.5, 8), (255, 225, 150), 1.0, 1.0)
    T.append((cx, cy + 80, '하얀 기사', GOLD, 22, 'mm')); T.append((cx + 80, cy - 90, 'NEW', GOLD, 14, 'mm'))
    firework(M, cx - 100, cy - 150, 60, GOLD); firework(M, cx + 100, cy - 130, 60, GOLD); confetti(M, cx, cy, 0.4, 40, gold=True)
    save(M, T, out, 'cine_upper.png', '상위 조커 연출 (약 2.5초, 처음 만들면 NEW + 0.5초, 대표 11종은 둘레에 소품)')
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
