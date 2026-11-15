"""NEON PALLAS 로고 — 70·80년대 바 간판 시안

    python tools/neon/logo_retro.py 출력폴더

A 라스베이거스 화살표: 전구가 둘러진 화살표 간판 + 필기체 Pallas, 위에 NEON 블록 글자 (전구가 차례로 도는 GIF)
B 칵테일 바: 필기체 Neon Pallas + 밑줄 꼬리, 부채꼴 카드 잔, 아래 'POKER · OPEN' 작은 상자
C 80년대 노을: 반원 노을 줄무늬 + 원근 바닥선 위로 NEON PALLAS, 아래 GODDESS OF POKER
"""
import math
import os
import sys

from PIL import Image

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, spade_crown, PINK, CYAN, GOLD, WHITE  # noqa: E402
from neon import suit, circle, arc  # noqa: E402

W, H = 1280, 720
SCRIPT = 'C:/Windows/Fonts/segoescb.ttf'
BLOCK = 'C:/Windows/Fonts/bahnschrift.ttf'
KO = 'C:/Windows/Fonts/malgunbd.ttf'
ORANGE, RED, LIME, VIOLET = (255, 150, 70), (255, 80, 90), (150, 255, 120), (190, 130, 255)


def bulbs(M, pts, step, phase, c=(255, 225, 150)):
    """점 목록을 따라 전구. phase 로 차례로 켜짐 (3개 중 1개 꺼짐)"""
    # 경로를 따라 같은 간격으로
    L = [0.0]
    for i in range(1, len(pts)):
        L.append(L[-1] + math.dist(pts[i - 1], pts[i]))
    n = int(L[-1] // step)
    j = 0
    for k in range(n):
        d = k * step
        while j < len(L) - 2 and L[j + 1] < d: j += 1
        t = (d - L[j]) / max(1e-6, L[j + 1] - L[j])
        x = pts[j][0] + (pts[j + 1][0] - pts[j][0]) * t
        y = pts[j][1] + (pts[j + 1][1] - pts[j][1]) * t
        on = (k + phase) % 3 != 0
        M.ln(circle(x, y, 4.2, 12), c, 1.4, 1.0 if on else 0.15)
        if on: M.dots.append((x, y, 1.6))


def logo_A(phase=0, state='on', t=0.0):
    M = Sign()
    fl = state == 'flicker'
    # 화살표 간판 몸통 (왼쪽 사각 + 오른쪽 화살촉, 아래로 꺾인 꼬리)
    body = [(230, 250), (900, 250), (900, 200), (1060, 330), (900, 460), (900, 410), (230, 410), (230, 250)]
    M.ln(body, ORANGE, 2.4, 0.5 if fl else 1.0)
    inner = [(246, 264), (914, 264), (914, 230), (1036, 330), (914, 430), (914, 396), (246, 396), (246, 264)]
    bulbs(M, inner, 26, phase)
    # 기둥
    M.ln([(330, 410), (330, 640)], (120, 130, 150), 2.0, 0.6); M.ln([(350, 410), (350, 640)], (120, 130, 150), 2.0, 0.6)
    # 위 NEON 블록 (네모 칸 안 글자)
    for i, ch in enumerate('NEON'):
        x = 300 + i * 92
        b = 0.08 if (fl and i == 2) else 1.0
        M.rr(x, 110, 78, 90, 8, CYAN, 1.8, b)
        M.word(x + 39, 182, ch, CYAN, 66, [b], font=BLOCK)
    bright = [1.0] * 6
    if fl: bright = [1, 1, 0.1, 1, 0.1, 1]
    if state == 'short': bright = [1, 1, 1, 0.35, 1, 1]
    M.word(590, 372, 'Pallas', PINK, 112, bright, track=2, font=SCRIPT)
    spade_crown(M, 980, 330, 26, GOLD, 0.5 if fl else 1.0)
    M.word(640, 470, 'GODDESS  OF  POKER', GOLD, 26, None, track=3, font=BLOCK)
    M.word(640, 680, '네온 팔라스', WHITE, 20, None, track=6, font=KO)
    if state == 'short':
        M.spark(660, 330, n=18, r=56, seed=int(t * 10) + 5)
    return M.draw()


def logo_B():
    M = Sign()
    # 카드 다섯 장 부채 = 잔 모양 위
    for k in range(5):
        a = math.radians(-30 + k * 15)
        cx, cy = 640 + math.sin(a) * 70, 210 - math.cos(a) * 20
        ca, sa = math.cos(a), math.sin(a)
        P = []
        for (px, py) in ((-28, -42), (28, -42), (28, 42), (-28, 42), (-28, -42)):
            P.append((cx + px * ca - py * sa, cy + px * sa + py * ca))
        M.ln(P, [CYAN, PINK, GOLD, LIME, CYAN][k], 1.8)
    for p in suit('spade')(640, 205, 14): M.ln(p, WHITE, 1.8)
    M.word(640, 420, 'Neon Pallas', PINK, 120, None, track=0, font=SCRIPT)
    # 밑줄 꼬리 (스우시)
    sw = [(330, 450), (500, 470), (700, 466), (900, 448), (980, 430)]
    M.ln(sw, CYAN, 2.4)
    # 작은 상자: POKER · OPEN
    M.rr(500, 500, 280, 60, 10, RED, 2.0)
    M.word(640, 543, 'POKER  ·  OPEN', RED, 30, None, track=4, font=BLOCK)
    M.word(640, 610, '포커의 여신 팔라스', GOLD, 20, None, track=4, font=KO)
    # 별 반짝 둘
    for (x, y) in ((300, 300), (990, 340)):
        M.ln([(x - 12, y), (x + 12, y)], WHITE, 1.4); M.ln([(x, y - 12), (x, y + 12)], WHITE, 1.4)
    return M.draw()


def logo_C():
    M = Sign()
    # 노을 반원 + 줄무늬 (위로 갈수록 노랑 → 아래 분홍)
    cx, cy, R = 640, 380, 190
    M.ln(arc(cx, cy, R, 180, 360, 48), ORANGE, 2.2)
    for k in range(7):
        y = cy - R + 30 + k * 24
        if y >= cy: break
        hw = math.sqrt(max(0.0, R * R - (cy - y) ** 2))
        c = GOLD if k < 2 else ORANGE if k < 4 else PINK
        M.ln([(cx - hw, y), (cx + hw, y)], c, 1.6, 0.9)
    # 원근 바닥 격자
    M.ln([(160, cy), (1120, cy)], VIOLET, 1.6)
    for k in range(-6, 7):
        M.ln([(cx + k * 40, cy), (cx + k * 160, 700)], VIOLET, 1.0, 0.55)
    for k in range(4):
        y = cy + 18 + k * k * 22
        M.ln([(160 - k * 60, y), (1120 + k * 60, y)], VIOLET, 1.0, 0.55)
    M.word(640, 330, 'NEON', CYAN, 88, None, track=24, font=BLOCK)
    M.word(640, 470, 'PALLAS', PINK, 120, None, track=14, font=BLOCK)
    M.word(640, 520, 'GODDESS OF POKER', GOLD, 26, None, track=6, font=BLOCK)
    spade_crown(M, 640, 120, 30, PINK)
    return M.draw()


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    logo_A(0).save(os.path.join(outdir, 'retro_A.png'))
    logo_A(0, 'short', 0.0).save(os.path.join(outdir, 'retro_A_short.png'))
    logo_B().save(os.path.join(outdir, 'retro_B.png'))
    logo_C().save(os.path.join(outdir, 'retro_C.png'))
    frames, durs = [], []
    seq = [('on', 18), ('flicker', 1), ('on', 2), ('flicker', 2), ('on', 10), ('short', 1), ('flicker', 1), ('short', 1), ('on', 14)]
    k, t = 0, 0.0
    for st, n in seq:
        for _ in range(n):
            frames.append(logo_A(k % 3, st, t).resize((640, 360), Image.LANCZOS).convert('P', palette=Image.ADAPTIVE, colors=128))
            durs.append(110)
            k += 1; t += 0.37
    frames[0].save(os.path.join(outdir, 'retro_A_anim.gif'), save_all=True, append_images=frames[1:], duration=durs, loop=0)
    print('ok', len(frames))


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
