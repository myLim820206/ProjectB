"""합성 모드 '오른쪽 클릭 = 취소' 커서 힌트 시안 (네온 선, DOC/art/15)

    python tools/neon/rclick_hint_mock.py 출력폴더

왼쪽: 평소 — 커서 오른쪽 아래에 작은 마우스(오른쪽 버튼만 빛남) + '취소'
가운데: 헛클릭 — 합성할 수 없는 칸을 왼쪽 클릭하면 오른쪽 버튼이 크게 번쩍
오른쪽: 아이콘 실제 크기(1x) / 2x / 4x
장면은 화면 설계 좌표를 2배로 키워 그렸다.
"""
import math
import os
import sys

from PIL import Image

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, FONT_KO, CYAN, WHITE, GOLD  # noqa: E402

PURPLE = (190, 140, 255)   # 플러시 족보 색 근처
PINK = (255, 106, 170)
DIM = (120, 132, 150)
RED = (255, 110, 120)


def pill(x, y, w, h, n=10):
    """마우스 몸통: 위아래가 둥근 캡슐 (왼쪽 위 x, y)"""
    r = w / 2
    P = []
    for k in range(n + 1):   # 위 반원
        a = math.radians(180 + k * 180 / n)
        P.append((x + r + math.cos(a) * r, y + r + math.sin(a) * r * 0.9))
    for k in range(n + 1):   # 아래 반원
        a = math.radians(k * 180 / n)
        P.append((x + r + math.cos(a) * r, y + h - r + math.sin(a) * r))
    P.append(P[0])
    return P


def right_button(x, y, w, h, n=8):
    """오른쪽 버튼 윤곽: 몸통 위 오른쪽 1/4 (가운데 세로선 · 가름선 · 둥근 어깨)"""
    r = w / 2
    split = y + h * 0.42
    P = [(x + r, y)]
    for k in range(n + 1):
        a = math.radians(270 + k * 90 / n)
        P.append((x + r + math.cos(a) * r, y + r + math.sin(a) * r * 0.9))
    P += [(x + w, split), (x + r, split), (x + r, y)]
    return P


def mouse_icon(M, x, y, s, lit=1.0, flash=0.0):
    """s = 아이콘 높이(px). lit = 오른쪽 버튼 밝기, flash = 번쩍 (0~1)"""
    w, h = s * 0.66, s
    M.ln(pill(x, y, w, h), DIM, 1.2 * s / 20, 0.8)
    M.ln([(x, y + h * 0.42), (x + w, y + h * 0.42)], DIM, 1.0 * s / 20, 0.7)
    M.ln([(x + w / 2, y + 1), (x + w / 2, y + h * 0.42)], DIM, 1.0 * s / 20, 0.7)
    rb = right_button(x, y, w, h)
    col = RED if flash > 0 else GOLD
    M.ln(rb, col, (1.6 + 1.4 * flash) * s / 20, lit)
    if flash > 0:   # 바깥으로 튀는 빛줄 셋
        cx, cy = x + w * 0.78, y + h * 0.18
        for a in (-75, -35, 5):
            ra = math.radians(a)
            r0, r1 = s * 0.4, s * (0.4 + 0.4 * flash)
            M.ln([(cx + math.cos(ra) * r0, cy + math.sin(ra) * r0), (cx + math.cos(ra) * r1, cy + math.sin(ra) * r1)], RED, 1.4 * s / 20, flash)


def cursor(M, x, y, s):
    P = [(0, 0), (0, 1.0), (0.27, 0.75), (0.45, 1.12), (0.58, 1.06), (0.4, 0.7), (0.74, 0.7), (0, 0)]
    M.ln([(x + px * s, y + py * s) for px, py in P], WHITE, 1.3 * s / 18, 1.0)


def tile(M, x, y, sz, col, b):
    M.rr(x, y, sz, sz, sz * 0.14, col, 1.4, b)


def scene(M, F, chips, ox, oy, flash=0.0, Z=2.0):
    """설계 좌표 장면 하나 (Z 배 확대). M = 뒤(판), F = 앞(커서 꼬리표), chips = 꼬리표 바탕 상자"""
    T = lambda x, y: (ox + x * Z, oy + y * Z)
    CS = 44
    # 판 칸들: 대부분 어둡게, 합쳐질 칸 셋만 빛남
    for gy in range(4):
        for gx in range(4):
            lit = (gx, gy) in ((1, 1), (2, 1), (2, 2))
            x, y = T(12 + gx * (CS + 4), 70 + gy * (CS + 4))
            tile(M, x, y, CS * Z, PINK if lit else DIM, 1.0 if lit else 0.25)
    # 위쪽 안내 (짧게)
    M.word(ox + 100 * Z, oy + 52 * Z, '플러시 합성: 합쳐질 칸을 클릭', PURPLE, 12 * Z, None, track=0.5, font=FONT_KO)
    # 커서: 빛나지 않는 칸 위 (헛클릭 장면) / 빛나는 칸 근처 (평소)
    cx, cy = T(70, 175) if flash > 0 else T(96, 98)
    if flash > 0:   # 헛클릭 자리 작은 X
        M.ln([(cx - 6 * Z, cy - 6 * Z), (cx + 6 * Z, cy + 6 * Z)], RED, 1.2 * Z, 0.7)
        M.ln([(cx + 6 * Z, cy - 6 * Z), (cx - 6 * Z, cy + 6 * Z)], RED, 1.2 * Z, 0.7)
    cursor(F, cx, cy, 18 * Z)
    # 꼬리표: 어두운 둥근 상자 + 마우스 + '취소'
    bx, by, bw, bh = cx + 14 * Z, cy + 18 * Z, 56 * Z, 30 * Z
    col = RED if flash > 0 else GOLD
    chips.append((bx, by, bw, bh, 6 * Z))
    F.rr(bx, by, bw, bh, 6 * Z, col, 1.0, 0.45 + 0.4 * flash)
    mouse_icon(F, bx + 6 * Z, by + 4 * Z, 22 * Z, 1.0, flash)
    F.word(bx + 24 * Z, by + 20 * Z, '취소', col, 13 * Z, None, font=FONT_KO, anchor_center=False)


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    from PIL import ImageChops, ImageDraw
    M, F, chips = Sign(), Sign(), []
    scene(M, F, chips, 10, 0, 0.0)
    scene(M, F, chips, 430, 0, 1.0)
    M.word(220, 700, '평소', CYAN, 20, None, font=FONT_KO)
    M.word(640, 700, '헛클릭 → 번쩍', CYAN, 20, None, font=FONT_KO)
    # 크기 비교
    for i, (s, lbl) in enumerate(((20, '1x'), (40, '2x'), (80, '4x'))):
        x = 900 + [0, 60, 150][i]
        y = 300 - s / 2
        cursor(M, x - 4, y - s * 0.9, s * 0.9)
        mouse_icon(M, x + s * 0.4, y, s)
        M.word(x + s * 0.6, 470, lbl, DIM, 16, None, font=FONT_KO)
    M.word(1050, 700, '아이콘 크기', CYAN, 20, None, font=FONT_KO)
    img = M.draw()
    d = ImageDraw.Draw(img)
    for (x, y, w, h, r) in chips:   # 꼬리표 바탕이 판 선을 가린다
        d.rounded_rectangle([x, y, x + w, y + h], r, fill=(6, 6, 11))
    img = ImageChops.subtract(ImageChops.add(img, F.draw()), Image.new('RGB', img.size, (6, 6, 11)))
    img.save(os.path.join(outdir, 'rclick_hint.png'))
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
