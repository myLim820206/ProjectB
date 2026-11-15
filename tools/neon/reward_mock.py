"""칵테일 보상 화면 시안 (D-284) — 5웨이브마다 셋 중 하나 고르기, 바 느낌. 판 자리(262, 98, 768 × 480)

    python tools/neon/reward_mock.py 출력폴더

A 바 카운터: 카운터 선 위에 잔 셋이 서 있고(칵테일마다 잔 모양 다름, 즉시 보상은 칩 접시), 아래 코스터 카드에 이름 · 효과. 마우스 = 잔에 불 + 분홍
B 메뉴 텐트 카드: 'TODAY'S SPECIAL' 머리 + 잔 그림 + 이름 + 효과가 든 세운 메뉴 카드 셋, 금색 두 겹 틀
C 바텐더 추천: 위에 전구 두른 'BARTENDER'S CHOICE' 간판, 선반 위 잔 셋 + 가격표 꼬리표(효과), 고른 잔은 술이 따라지는 모습
"""
import math
import os
import sys

from PIL import Image

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, PINK, CYAN, GOLD, WHITE  # noqa: E402
from logo_retro import KO, BLOCK, bulbs  # noqa: E402
from neon import circle  # noqa: E402

LAV = (168, 150, 225)
FG = (207, 216, 230)
DIM = (95, 104, 120)
MINT = (138, 255, 196)
FX, FY, BW, BH = 262, 98, 768, 480
ITEMS = [  # 이름, 효과, 종류(잔), 색, 칵테일?
    ('블랙 스페이드 마티니', '♠ 타워 공격력 +40%', 'martini', (232, 238, 255), True),
    ('체인 펀치', '연쇄 보너스 +25% → +45%', 'highball', PINK, True),
    ('경험치', '경험치 +34 (바로)', 'chips', CYAN, False),
]
SEL = 1


def glass(M, kind, cx, by, c, b=1.0, fill=0.0):
    """잔 그림: by = 잔 바닥선"""
    if kind == 'martini':
        M.ln([(cx - 26, by - 70), (cx + 26, by - 70), (cx, by - 36), (cx - 26, by - 70)], c, 1.8, b)
        M.ln([(cx, by - 36), (cx, by - 4)], c, 1.6, b); M.ln([(cx - 16, by - 2), (cx + 16, by - 2)], c, 1.6, b)
        if fill:
            for k in range(5): hw = 22 * (1 - k * 5 / 30); M.ln([(cx - hw, by - 64 + k * 5), (cx + hw, by - 64 + k * 5)], c, 1.0, 0.5 * b)
            M.ln(circle(cx + 9, by - 66, 4, 12), MINT, 1.2, b)
    elif kind == 'highball':
        M.ln([(cx - 18, by - 78), (cx - 16, by - 2), (cx + 16, by - 2), (cx + 18, by - 78)], c, 1.8, b)
        M.ln([(cx + 6, by - 92), (cx - 4, by - 30)], WHITE, 1.2, 0.7 * b)  # 빨대
        if fill:
            for k in range(8): M.ln([(cx - 15, by - 60 + k * 7), (cx + 15, by - 60 + k * 7)], c, 1.0, 0.45 * b)
            for (dx, dy) in ((-6, -44), (5, -30), (-2, -18)): M.ln(circle(cx + dx, by + dy, 2.2, 8), WHITE, 0.8, 0.6 * b)
    elif kind == 'rocks':
        M.ln([(cx - 24, by - 46), (cx - 22, by - 2), (cx + 22, by - 2), (cx + 24, by - 46)], c, 1.8, b)
        if fill: M.ln([(cx - 12, by - 30), (cx - 2, by - 38), (cx + 8, by - 30), (cx - 2, by - 22), (cx - 12, by - 30)], WHITE, 1.0, 0.6 * b)
    elif kind == 'chips':  # 즉시 보상 = 칩 접시
        M.ln([(cx - 34, by - 2), (cx + 34, by - 2)], c, 1.6, b)
        for k in range(4):
            y = by - 10 - k * 9
            M.ln([(cx - 18, y), (cx + 18, y)], c, 3.0, b)
            M.ln(circle(cx - 18, y, 1.5, 8), c, 1.0, b)


def frame2(M, x, y, w, h, r, c, b, wd=2.0):
    from crt_mock import rr_pts
    M.ln(rr_pts(x, y, w, h, r), c, wd, b)
    M.ln(rr_pts(x + 5, y + 5, w - 10, h - 10, max(2, r - 5)), c, 1.0, 0.7 * b)


def board_dim(M):
    frame2(M, FX - 6, FY - 6, BW + 12, BH + 12, 10, LAV, 0.5, 1.4)


def title(M, s, y=150):
    M.word(FX + BW / 2, FY + y - 98 + 0, s, GOLD, 22, None, font=KO)


# ---------------------------------------------------------------- A 바 카운터
def mock_A():
    M = Sign(0.8); M.small_glow = 0.0
    board_dim(M)
    M.word(FX + BW / 2, FY + 52, '웨이브 5 — 바텐더가 셋을 내놓았다', GOLD, 20, None, font=KO)
    M.word(FX + BW / 2, FY + 76, '하나를 고른다 · 칵테일은 판 내내, 서비스는 바로', DIM, 11, None, font=KO)
    cy = FY + 280  # 카운터
    M.ln([(FX + 40, cy), (FX + BW - 40, cy)], (150, 120, 90), 2.2, 0.8)
    M.ln([(FX + 40, cy + 8), (FX + BW - 40, cy + 8)], (150, 120, 90), 1.0, 0.4)
    for i, (n, e, kind, c, ck) in enumerate(ITEMS):
        cx = FX + BW / 2 + (i - 1) * 240
        on = i == SEL
        glass(M, kind, cx, cy - 2, c, 1.0 if on else 0.55, fill=1.0 if on or ck else 0.0)
        if on: M.ln(circle(cx, cy - 40, 62, 40), PINK, 1.0, 0.25)
        # 코스터 카드
        frame2(M, cx - 100, cy + 22, 200, 112, 12, PINK if on else (GOLD if ck else CYAN), 1.0 if on else 0.6)
        M.word(cx - 86, cy + 44, '칵테일' if ck else '서비스', GOLD if ck else CYAN, 10, None, font=KO, anchor_center=False)
        M.word(cx + 86, cy + 44, str(i + 1), DIM, 10, None, font=BLOCK, anchor_center=False)
        M.word(cx, cy + 78, n, WHITE if on else FG, 16, None, font=KO)
        M.word(cx, cy + 104, e, FG, 11, [0.9] * len(e), font=KO)
        if on: M.word(cx, cy + 126, '클릭 · 1 · 2 · 3', PINK, 9, None, font=KO)
    M.word(FX + BW / 2, FY + BH - 14, 'S: 건너뛰기 · 칵테일 칸 2/5', DIM, 11, None, font=KO)
    return M.draw()


# ---------------------------------------------------------------- B 메뉴 텐트 카드
def mock_B():
    M = Sign(0.8); M.small_glow = 0.0
    board_dim(M)
    M.word(FX + BW / 2, FY + 60, "TODAY'S SPECIAL", GOLD, 26, None, track=4, font=BLOCK)
    M.word(FX + BW / 2, FY + 86, '웨이브 5 · 셋 중 하나', FG, 12, None, font=KO)
    for i, (n, e, kind, c, ck) in enumerate(ITEMS):
        x = FX + BW / 2 + (i - 1) * 240 - 105
        y = FY + 120
        on = i == SEL
        col = PINK if on else (GOLD if ck else CYAN)
        # 텐트 카드: 위가 살짝 좁은 사다리꼴 + 두 겹
        M.ln([(x + 12, y), (x + 198, y), (x + 210, y + 300), (x, y + 300), (x + 12, y)], col, 2.0, 1.0 if on else 0.65)
        M.ln([(x + 18, y + 6), (x + 192, y + 6), (x + 203, y + 294), (x + 7, y + 294), (x + 18, y + 6)], col, 1.0, 0.5 if on else 0.35)
        M.word(x + 105, y + 30, 'COCKTAIL' if ck else 'ON THE HOUSE', col, 10, None, track=3, font=BLOCK)
        M.ln([(x + 40, y + 40), (x + 170, y + 40)], col, 1.0, 0.5)
        glass(M, kind, x + 105, y + 150, c, 1.0 if on else 0.7, fill=1.0)
        M.word(x + 105, y + 196, n, WHITE if on else FG, 16, None, font=KO)
        M.word(x + 105, y + 226, e, FG, 11, None, font=KO)
        M.ln([(x + 40, y + 250), (x + 170, y + 250)], col, 1.0, 0.4)
        M.word(x + 105, y + 276, str(i + 1), col, 18, None, font=BLOCK)
        if on: M.ln(circle(x + 105, y - 14, 5, 12), PINK, 1.2) if False else None
    M.word(FX + BW / 2, FY + BH - 14, '1 · 2 · 3 또는 클릭 · S: 건너뛰기', DIM, 11, None, font=KO)
    return M.draw()


# ---------------------------------------------------------------- C 바텐더 추천
def mock_C():
    M = Sign(0.8); M.small_glow = 0.0
    board_dim(M)
    # 전구 간판
    sx, sy, sw, sh = FX + BW / 2 - 200, FY + 24, 400, 64
    frame2(M, sx, sy, sw, sh, 12, (255, 150, 70), 1.0)
    bulbs(M, [(sx + 14, sy + 12), (sx + sw - 14, sy + 12), (sx + sw - 14, sy + sh - 12), (sx + 14, sy + sh - 12), (sx + 14, sy + 12)], 16, 0)
    M.word(FX + BW / 2, sy + 42, "BARTENDER'S CHOICE", GOLD, 22, None, track=3, font=BLOCK)
    # 선반
    shy = FY + 270
    M.ln([(FX + 60, shy), (FX + BW - 60, shy)], LAV, 2.0, 0.8)
    for x in (FX + 90, FX + BW - 90): M.ln([(x, shy), (x - 10, shy + 18)], LAV, 1.4, 0.6)
    for i, (n, e, kind, c, ck) in enumerate(ITEMS):
        cx = FX + BW / 2 + (i - 1) * 240
        on = i == SEL
        glass(M, kind, cx, shy - 2, c, 1.0 if on else 0.6, fill=1.0 if on else 0.4)
        if on:  # 따르는 술줄기
            M.ln([(cx - 2, shy - 150), (cx - 2, shy - 66)], c, 2.0, 0.9)
            M.word(cx, shy - 160, '♥', PINK, 16, None, font=KO)
        # 가격표 꼬리표 (효과)
        tx, ty = cx - 92, shy + 34
        P = [(tx, ty + 20), (tx + 14, ty), (tx + 184, ty), (tx + 184, ty + 40), (tx + 14, ty + 40), (tx, ty + 20)]
        M.ln(P, PINK if on else (GOLD if ck else CYAN), 1.6, 1.0 if on else 0.6)
        M.ln(circle(tx + 12, ty + 20, 3, 10), FG, 1.0, 0.6)
        M.word(tx + 100, ty + 18, n, WHITE if on else FG, 13, None, font=KO)
        M.word(tx + 100, ty + 34, e, FG, 10, None, font=KO)
        M.word(cx, ty + 66, ('칵테일' if ck else '서비스') + f'  ·  {i + 1}', GOLD if ck else CYAN, 10, None, font=KO)
    M.word(FX + BW / 2, FY + BH - 14, 'S: 건너뛰기 · 칵테일 칸 2/5', DIM, 11, None, font=KO)
    return M.draw()


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    for name, fn in (('reward_A_counter', mock_A), ('reward_B_tent', mock_B), ('reward_C_choice', mock_C)):
        fn().crop((FX - 20, FY - 20, FX + BW + 20, FY + BH + 20)).save(os.path.join(outdir, name + '.png'))
        print(name)


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
