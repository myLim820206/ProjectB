"""판 위력 카드 — 옛날 CRT TV 시안 (D-277). 오른쪽 열 위 (1044, 98, 220 × 226) 자리

    python tools/neon/crt_mock.py 출력폴더

A 앰버 모니터: 둥근 TV 틀 + 토끼 귀 안테나, 호박색 형광 글자, 큰 숫자, 순위는 블록 막대 '█████░░', 주사선, 오른쪽 아래 손잡이 둘, CH 03
B 초록 오실로스코프: 초록 형광, 큰 숫자 아래 초당 피해 파형(웨이브 동안 흐름), 눈금 격자, 순위는 짧은 막대
C 컬러 TV: 위에 색 막대(테스트 패턴) 띠, 숫자는 빨강 · 하늘 어긋난 색번짐, 순위는 무늬 색, 'ON AIR' 빨간 불, 화면 가장자리 어둡게
"""
import math
import os
import random
import sys

from PIL import Image, ImageDraw

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, PINK, CYAN, GOLD, WHITE  # noqa: E402
from logo_retro import KO, BLOCK  # noqa: E402
from neon import circle  # noqa: E402

LAV = (168, 150, 225)
FG = (207, 216, 230)
DIM = (95, 104, 120)
X0, Y0, W, H = 1044, 98, 220, 226
POWER = [('할리퀸 Lv3', 0.32, '1.1k'), ('8♠ Lv3', 0.18, '620'), ('J♥ Lv2', 0.14, '480'), ('9♥ Lv2', 0.12, '410'), ('10♣ Lv2', 0.10, '330'), ('K♠ Lv1', 0.08, '260')]
SUITC = [(255, 154, 213), (232, 238, 255), (255, 106, 138), (255, 106, 138), (143, 233, 255), (232, 238, 255)]


def rr_pts(x, y, w, h, r, n=6):
    P = []
    for (cx, cy, a0) in ((x + w - r, y + r, -90), (x + w - r, y + h - r, 0), (x + r, y + h - r, 90), (x + r, y + r, 180)):
        for k in range(n + 1):
            a = math.radians(a0 + k * 90 / n)
            P.append((cx + math.cos(a) * r, cy + math.sin(a) * r))
    P.append(P[0])
    return P


def cabinet(M, scr, col=LAV, ant=True, knobs=True):
    """TV 틀: 바깥 둥근 상자 + 볼록한 화면(모서리 크게 둥근) + 안테나 + 손잡이"""
    M.ln(rr_pts(X0, Y0, W, H, 16), col, 2.0, 0.8)
    M.ln(rr_pts(X0 + 5, Y0 + 5, W - 10, H - 10, 12), col, 1.0, 0.45)
    sx, sy, sw, sh = scr
    M.ln(rr_pts(sx, sy, sw, sh, 22, 8), col, 1.6, 0.85)
    if ant:  # 토끼 귀 안테나 (위로 짧게)
        bx, by = X0 + W - 54, Y0
        M.ln([(bx - 10, by), (bx + 10, by), (bx + 6, by - 6), (bx - 6, by - 6), (bx - 10, by)], col, 1.4, 0.8)
        M.ln([(bx - 3, by - 6), (bx - 22, by - 30)], col, 1.2, 0.8)
        M.ln([(bx + 3, by - 6), (bx + 26, by - 28)], col, 1.2, 0.8)
        M.dots.append((bx - 22, by - 30, 1.2)); M.dots.append((bx + 26, by - 28, 1.2))
    if knobs:  # 오른쪽 아래 손잡이 둘 + 스피커 구멍
        for i, ky in enumerate((Y0 + H - 50, Y0 + H - 24)):
            kx = X0 + W - 22
            M.ln(circle(kx, ky, 8, 20), col, 1.3, 0.8)
            a = math.radians(-60 + i * 110)
            M.ln([(kx, ky), (kx + math.cos(a) * 6, ky + math.sin(a) * 6)], col, 1.2, 0.8)
        for i in range(4):
            M.ln([(X0 + W - 30 + i * 5, Y0 + 30), (X0 + W - 30 + i * 5, Y0 + 70)], col, 1.0, 0.35)


def scanlines(img, scr, k=0.35, step=3):
    """주사선 + 가장자리 어둡게 (그린 뒤 화면 영역에 곱한다)"""
    sx, sy, sw, sh = scr
    d = ImageDraw.Draw(img, 'RGBA')
    for y in range(int(sy), int(sy + sh), step):
        d.line([(sx + 4, y), (sx + sw - 4, y)], fill=(0, 0, 0, int(255 * k)))
    # 가장자리 어둡게
    for i in range(10):
        a = int(18 * (10 - i) / 10)
        d.rounded_rectangle([sx + i, sy + i, sx + sw - i, sy + sh - i], radius=22, outline=(0, 0, 0, a), width=1)
    return img


def screen_text(M, scr, col, big_font=BLOCK, bars='block', title='판 위력'):
    sx, sy, sw, sh = scr
    M.word(sx + 12, sy + 18, title, col, 11, None, font=KO, anchor_center=False)
    M.word(sx + sw - 10, sy + 18, 'CH 03', col, 8, [0.7] * 5, font=BLOCK, anchor_center=False) if False else None
    M.word(sx + 12, sy + 52, '3,412', col, 30, None, track=1, font=big_font, anchor_center=False)
    M.word(sx + 120, sy + 46, '▲ +12%', col, 10, [0.85] * 6, font=KO, anchor_center=False)
    for i, (n, sh_, v) in enumerate(POWER):
        y = sy + 78 + i * 18
        M.word(sx + 12, y, n, col, 10, [0.9] * len(n), font=KO, anchor_center=False)
        if bars == 'block':  # 블록 칸 막대 (켜진 칸 = 굵게, 꺼진 칸 = 흐린 틀)
            cells = 8
            on = max(1, min(cells, round(sh_ * 25)))
            for c in range(cells):
                cx = sx + 74 + c * 7
                if c < on: M.ln([(cx, y - 4), (cx + 4, y - 4)], col, 5.0, 0.9)
                else: M.ln(rr_pts(cx - 0.5, y - 7, 5, 6, 1, 1), col, 0.8, 0.3)
        else:
            M.ln([(sx + 72, y - 3), (sx + 72 + 50 * sh_ / 0.32, y - 3)], col, 3.0, 0.8)
        M.word(sx + sw - 12 - len(v) * 6, y, v, col, 10, [0.9] * len(v), font=BLOCK, anchor_center=False)
    M.word(sx + 12, sy + sh - 14, '초당 피해 어림', col, 9, [0.6] * 8, font=KO, anchor_center=False)


def crt_A():
    M = Sign(0.8); M.small_glow = 0.0
    AMB = (255, 176, 64)
    scr = (X0 + 12, Y0 + 12, W - 52, H - 24)
    cabinet(M, scr)
    screen_text(M, scr, AMB)
    M.word(X0 + W - 22, Y0 + 92, 'CH', LAV, 7, [0.8, 0.8], font=BLOCK)
    M.word(X0 + W - 22, Y0 + 104, '03', AMB, 9, None, font=BLOCK)
    M.dots.append((X0 + W - 22, Y0 + H - 74, 1.4))  # 전원 불
    img = M.draw()
    return scanlines(img, scr)


def crt_B():
    M = Sign(0.8); M.small_glow = 0.0
    GRN = (110, 255, 150)
    scr = (X0 + 12, Y0 + 12, W - 52, H - 24)
    cabinet(M, scr, ant=False)
    sx, sy, sw, sh = scr
    # 눈금 격자
    for gx in range(int(sx + 14), int(sx + sw - 8), 16): M.ln([(gx, sy + 84), (gx, sy + 140)], GRN, 1.0, 0.18)
    for gy in (sy + 84, sy + 112, sy + 140): M.ln([(sx + 10, gy), (sx + sw - 10, gy)], GRN, 1.0, 0.18)
    M.word(sx + 12, sy + 18, '판 위력', GRN, 11, None, font=KO, anchor_center=False)
    M.word(sx + 12, sy + 52, '3,412', GRN, 30, None, track=1, font=BLOCK, anchor_center=False)
    M.word(sx + 118, sy + 46, '+12%', GRN, 10, None, font=BLOCK, anchor_center=False)
    # 초당 피해 파형
    rnd = random.Random(4)
    pts = []
    for i in range(60):
        x = sx + 12 + i * (sw - 24) / 59
        v = 0.5 + 0.3 * math.sin(i * 0.35) + 0.15 * math.sin(i * 1.3) + rnd.uniform(-0.05, 0.05)
        pts.append((x, sy + 140 - v * 52))
    M.ln(pts, GRN, 1.6, 1.0)
    M.dots.append((pts[-1][0], pts[-1][1], 1.6))
    for i, (n, sh_, v) in enumerate(POWER[:4]):
        y = sy + 158 + i * 14
        M.word(sx + 12, y, n, GRN, 9.5, [0.9] * len(n), font=KO, anchor_center=False)
        M.ln([(sx + 74, y - 3), (sx + 74 + 48 * sh_ / 0.32, y - 3)], GRN, 2.6, 0.8)
        M.word(sx + sw - 12 - len(v) * 6, y, v, GRN, 9.5, [0.9] * len(v), font=BLOCK, anchor_center=False)
    for i, ky in enumerate((Y0 + 40, Y0 + 70)):
        M.word(X0 + W - 22, ky - 12, ('TIME', 'AMP')[i], LAV, 6, [0.7] * (4 - i), font=BLOCK)
    img = M.draw()
    return scanlines(img, scr, 0.3)


def crt_C():
    M = Sign(0.8); M.small_glow = 0.0
    scr = (X0 + 12, Y0 + 12, W - 52, H - 24)
    cabinet(M, scr, col=LAV)
    sx, sy, sw, sh = scr
    bars = [(235, 235, 235), (255, 217, 106), (110, 220, 255), (138, 255, 196), (255, 106, 170), (255, 90, 90), (110, 120, 255)]
    bw = (sw - 24) / len(bars)
    for i, c in enumerate(bars):
        for k in range(5): M.ln([(sx + 12 + i * bw + 1, sy + 10 + k * 2), (sx + 12 + (i + 1) * bw - 1, sy + 10 + k * 2)], c, 1.2, 0.7)
    M.word(sx + 12, sy + 36, '판 위력', FG, 11, None, font=KO, anchor_center=False)
    # 색번짐 숫자: 빨강 · 하늘을 어긋나게 + 흰 글자
    M.word(sx + 10, sy + 70, '3,412', (255, 70, 90), 30, [0.7] * 5, track=1, font=BLOCK, anchor_center=False)
    M.word(sx + 14, sy + 70, '3,412', (80, 220, 255), 30, [0.7] * 5, track=1, font=BLOCK, anchor_center=False)
    M.word(sx + 12, sy + 70, '3,412', WHITE, 30, None, track=1, font=BLOCK, anchor_center=False)
    M.word(sx + 120, sy + 64, '+12%', (156, 255, 176), 10, None, font=BLOCK, anchor_center=False)
    for i, (n, sh_, v) in enumerate(POWER[:5]):
        y = sy + 94 + i * 17
        c = SUITC[i]
        M.word(sx + 12, y, n, c, 10, None, font=KO, anchor_center=False)
        M.ln([(sx + 72, y - 3), (sx + 72 + 50 * sh_ / 0.32, y - 3)], c, 3.0, 0.7)
        M.word(sx + sw - 12 - len(v) * 6, y, v, FG, 10, None, font=BLOCK, anchor_center=False)
    # ON AIR 불
    M.ln(rr_pts(X0 + W - 34, Y0 + 86, 26, 14, 4), (255, 80, 90), 1.2, 1.0)
    M.word(X0 + W - 21, Y0 + 96, 'ON AIR', (255, 80, 90), 5.5, None, font=BLOCK)
    img = M.draw()
    return scanlines(img, scr, 0.28)


def context(img):
    """오른쪽 열 + 위 간판 일부를 잘라 1.6배"""
    return img.crop((960, 40, 1280, 420)).resize((512, 608), Image.LANCZOS)


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    for name, fn in (('crt_A_amber', crt_A), ('crt_B_scope', crt_B), ('crt_C_color', crt_C)):
        context(fn()).save(os.path.join(outdir, name + '.png'))
        print(name)


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
