"""새 UI 시안(사용자 그림, 2026-11-20 판)을 우리 화풍(docs/15)으로 — 자리 · 크기 · 그림을 시안과 똑같이 옮긴 인게임 시안

    python tools/neon/alice_ui_mock.py 출력폴더   → alice_ui_mock.png

시안 그림(2000 × 1125)의 좌표를 0.64배해 1280 × 720 에 그대로 놓았다.
- 왼쪽 긴 패널 (12~255): BAR 상자 + Alice 필기체 + 마티니, '족보 메뉴', 5장 / 2~4장 / 조커·먹이 묶음, 줄마다 왼쪽 족보 색 막대 ·
  이름 ····· 배수 · 아래 설명, 마우스를 올린 줄(풀하우스)은 금색, 맨 아래 연쇄 안내
- 위 가운데: 야자수 + 아래 네온 줄 셋, 분홍 타원 안 'WAVE', 파란 상자 '3/20'
- 판 (262~1029, 86~568): 칸 47, 길은 어두운 띠 + 가운데 점선 + 꺾이는 곳 점, 왼쪽 세로 길에 병정 줄, 맨 앞에서 ♥ 폭발
- 오른쪽 위 TV (1038~1270, 83~400): 토끼 귀 안테나, 화면 = ON AIR · CH 1 · '- 제 3화 -' · '병정들의 행진' · 적 구성 셋 ·
  남은 적 / 처치 · 놓침, 화면 아래 ×2 · ×4 버튼 · 분홍 다이얼 · 스피커 줄
- 칵테일 선반(잔 다섯, 둘 켜짐) + 아래 네온 줄들, 파란 스킬 버튼(노드 아이콘), 초록 시작 버튼
- 아래: '10 / 10' · 덱 38장, 손패 10장 (번호 · 무늬 · 숫자 · 공격 방식)
시안의 칠한 카드 · 버튼은 화풍대로 '채우지 않은 두 겹 선 + 옅은 채움'. 시안에 없는 목숨은 TV 머리줄에, 레벨은 넣지 않았다.
"""
import math
import os
import random
import sys

from PIL import ImageDraw

sys.path.insert(0, os.path.dirname(__file__))
import ingame_mock as G  # noqa: E402
from ingame_mock import Bar, tube, suit_shape, soldier, hx, circle  # noqa: E402
from ingame_mock import KO, KOR, BLOCK, WHITE, FG, GREY, DIMT, GOLD, PINK, RED, ORANGE, SUITC, ATK  # noqa: E402
from neon import bez  # noqa: E402
from logo_alice import draw_alice, martini  # noqa: E402

MAG = hx('e07aff')       # 패널 · TV 테두리 (시안의 자홍)
BLUE = hx('6a8cff')      # 스킬 버튼 · WAVE 상자
CYAN2 = hx('7fd6ff')
LIME = hx('9cffb0')      # 시작 버튼
LEAF = SUITC['club']
PHOS = (222, 232, 245)

FX, FY, CS = 267, 93, 47

# ---------------------------------------------------------------- 판 상태 (시안의 칸 그대로)
G.PATH_WP = [(0, 6), (1, 6), (1, 1), (4, 1), (4, 5), (8, 5), (8, 1), (11, 1), (11, 7), (15, 7)]
G.TOWERS = {
    (2, 2): ('4', 'heart', 1), (3, 2): ('5', 'diamond', 1), (2, 3): ('K', 'spade', 1), (3, 3): ('6', 'heart', 1),
    (5, 1): ('10', 'heart', 1), (5, 2): ('9', 'heart', 1), (5, 3): ('8', 'spade', 1),
    (6, 3): ('K', 'spade', 1), (5, 4): ('3', 'diamond', 1),
}
G.JOKER = None
G.FLOORS = [([(2, 2), (3, 2)], hx('b48cff')),                 # 시안의 4 · 5 보라 틀
            ([(5, 1), (5, 2), (5, 3)], hx('6a8cff'))]         # 8 · 9 · 10 스트레이트
# 왼쪽 세로 길에 줄지은 병정 (입구 둘 + 세로 줄), 맨 앞이 폭발을 맞는 중
G.ENEMIES = [(0.010, 'normal', 1.0), (0.032, 'normal', 1.0), (0.056, 'normal', 0.9), (0.075, 'fast', 0.8),
             (0.093, 'normal', 0.8), (0.111, 'normal', 0.7), (0.129, 'normal', 0.6), (0.147, 'fast', 0.5), (0.165, 'normal', 0.3)]

HANDS = {  # 이름, 배수, 설명, 색
    'royal': ('로열 스트레이트 플러시', '×2.5', '공격력 ×2.5 / 연사 ×2.5 / 넓은 범위', 'fff4d6'),
    'five': ('파이브 카드', '×5', '공격력 ×5 (와일드 또는 조커 필요)', 'ffd96a'),
    'sf': ('스트레이트 플러시', '×2', '공격력 ×2 / 연사 ×2 / 범위', 'ff9ad5'),
    'full': ('풀하우스', '×1.5', '공격력 ×1.5 / 연사 ×1.5 / 범위', 'ffa86a'),
    'flush': ('플러시 (3~5장)', '', '범위 × 카드 수', 'b48cff'),
    'straight': ('스트레이트', '×1.25', '연사 ×1.25 / ×1.5 / ×1.75 / ×2', '6a8cff'),
    'four': ('포카드', '×3.5', '공격력 ×3.5', 'ff6a6a'),
    'triple': ('트리플', '×2', '공격력 ×2', '9cffb0'),
    'twopair': ('투 페어', '×1.5', '공격력 ×1.5 / 범위 ×1.25', '7fd6ff'),
    'pair': ('원 페어', '×1.5', '공격력 ×1.5', 'cfd8e6'),
    'joker': ('조커', '×1', '클릭 시 생성 가능 조커 목록', 'ff9ad5'),
    'feed': ('먹이기', '', '클릭 시 조커에게 족보 먹이기', 'ff9ad5'),
}
# (묶음 이름, 머리글 y, 첫 줄 y, 줄들) — 시안 좌표 × 0.64
GROUPS = [('5장', 104, 123, ['royal', 'five', 'sf', 'full', 'flush', 'straight']),
          ('2~4장', 374, 392, ['four', 'triple', 'twopair', 'pair']),
          ('조커/먹이', 568, 586, ['joker', 'feed'])]
ROW_P, ROW_H = 37.4, 31
HOVER = 'full'
HAND = [('6', 'heart'), ('2', 'heart'), ('8', 'spade'), ('9', 'club'), ('6', 'spade'),
        ('5', 'diamond'), ('4', 'club'), ('3', 'club'), ('2', 'diamond'), ('A', 'diamond')]


# ---------------------------------------------------------------- 왼쪽 패널
def menu_row(M, x, y, w, key):
    n, mult, desc, hc = HANDS[key]
    c = hx(hc)
    hov = key == HOVER
    h = ROW_H
    if hov:
        tube(M, x, y, w, h, 6, GOLD, on=True)
    else:
        M.fill_rr(x, y, w, h, 5, (10, 10, 18), 0.9, layer=2)
        M.rr(x, y, w, h, 5, FG, 1.0, 0.32, glow=False)
        M.rr(x + 2.5, y + 2.5, w - 5, h - 5, 4, FG, 0.7, 0.18, glow=False)
    M.ln([(x + 5, y + 6), (x + 5, y + h - 6)], c, 2.6, 1.0)            # 왼쪽 족보 색 막대
    nw = M.t(x + 13, y + 14, n, WHITE if hov else FG, 11, 'ls', KO, maxw=w - 60)
    pw = M.t(x + w - 8, y + 14, mult, WHITE if hov else FG, 11, 'rs', BLOCK) if mult else 0
    for dx in range(int(x + 17 + nw), int(x + w - 12 - pw), 5):
        M.ln([(dx, y + 11), (dx + 1.3, y + 11)], DIMT, 1.0, 0.55, glow=False)
    M.t(x + 13, y + 26, desc, (255, 230, 170) if hov else GREY, 8.2, 'ls', KOR, maxw=w - 20)


def group_head(M, cx, y, title):
    tw = M.width(title, 13, KO)
    M.t(cx, y + 5, title, FG, 13, 'ms', KO)
    for s in (-1, 1):
        M.ln([(cx + s * (tw / 2 + 10), y), (cx + s * 100, y)], GOLD, 1.2, 0.5, glow=False)


def left_panel(M):
    tube(M, 12, 10, 243, 699, 14, MAG, b=0.62)
    # 로고: 기운 BAR 상자 + Alice + 마티니 (시안 왼쪽 위)
    M.rr(18, 21, 28, 13, 6, PINK, 1.2, 0.9)
    M.t(32, 31, 'BAR', CYAN2, 8, 'ms', BLOCK)
    draw_alice(M, 26, 52, 0.34, col=PINK)
    martini(M, 98, 24, 9, GOLD)
    M.t(133, 76, '족보 메뉴', PINK, 24, 'ms', KO, glow=True, hollow=True)
    for title, hy, ry, keys in GROUPS:
        group_head(M, 133, hy, title)
        for i, k in enumerate(keys):
            menu_row(M, 24, ry + i * ROW_P, 218, k)
    M.ln([(18, 671), (250, 671)], MAG, 1.0, 0.3, glow=False)
    M.t(28, 690, '연쇄', WHITE, 11, 'ls', KO)
    M.t(58, 690, '족보끼리 맞닿을 경우, 개당 +25%', GREY, 9, 'ls', KOR)


# ---------------------------------------------------------------- 위: 야자수 WAVE 간판
def palm(M):
    """야자수: 굽은 줄기(주황, 마디) + 잎 다섯(초록) + 코코넛 둘. 시안 (420~520, 12~120)"""
    base, top = (276, 77), (300, 22)
    L = bez(base, (296, 54), top, 16)
    R = [(px + 7, py + 1) for px, py in bez((283, 77), (304, 54), (303, 24), 16)]
    M.ln(L, ORANGE, 1.6, 0.95); M.ln(R, ORANGE, 1.6, 0.95)
    for i in range(2, 15, 3): M.ln([L[i], R[i]], ORANGE, 1.1, 0.75)
    tx, ty = 304, 22
    for (ex, ey, cx, cy) in ((-34, 16, -18, -10), (-24, -14, -10, -16), (8, -18, 2, -18), (32, -8, 18, -16), (30, 22, 24, -2)):
        a = bez((tx, ty), (tx + cx, ty + cy), (tx + ex, ty + ey), 14)
        b = bez((tx, ty), (tx + cx + 4, ty + cy + 7), (tx + ex, ty + ey), 14)
        M.ln(a, LEAF, 1.6, 1.0); M.ln(b, LEAF, 1.1, 0.65)
    for (ox, oy) in ((-4, 6), (5, 7)):
        M.ln(circle(tx + ox, ty + oy, 3.2, 12), ORANGE, 1.2, 0.9)


def wave_sign(M, wave=3, goal=20):
    palm(M)
    for (yy, x0, x1, c) in ((61, 316, 410, GOLD), (69, 330, 392, CYAN2), (69, 398, 419, CYAN2), (76, 318, 360, BLUE)):
        M.ln([(x0, yy), (x1, yy)], c, 1.7, 0.85)
    cx, cy = 485, 42
    M.ln(circle(cx, cy, 36, 60, sx=2.25), PINK, 2.4, 1.0, core=True)
    M.ln(circle(cx, cy, 30, 60, sx=2.55), PINK, 1.3, 0.75)
    M.t(cx - 2, cy + 14, 'WAVE', CYAN2, 38, 'ms', BLOCK, glow=True, hollow=True)
    M.rr(560, 10, 192, 70, 6, BLUE, 2.4, 1.0, core=True)
    M.rr(565, 15, 182, 60, 4, BLUE, 1.2, 0.7)
    M.t(656, 66, '%d/%d' % (wave, goal), BLUE, 44, 'ms', BLOCK, glow=True, hollow=True)


# ---------------------------------------------------------------- 오른쪽: TV · 칵테일 · 스킬 · 시작
def tv(M):
    X, Y, W, H = 1038, 83, 232, 317
    ax, ay = 1204, 80
    M.ln(bez((ax - 4, ay), (ax - 8, 40), (1184, 6), 10), MAG, 1.5, 0.8)
    M.ln(bez((ax + 6, ay), (ax + 14, 40), (1219, 6), 10), MAG, 1.5, 0.8)
    M.ln([(ax - 16, ay + 3)] + [(ax + 1 + math.cos(math.radians(a)) * 17, ay + 3 - math.sin(math.radians(a)) * 8) for a in range(180, -1, -15)], MAG, 1.4, 0.8)
    tube(M, X, Y, W, H, 16, MAG, b=0.85)
    SX, SY, SW, SH = 1053, 102, 201, 228
    M.fill_rr(SX, SY, SW, SH, 10, (4, 6, 10), 0.95, layer=3)
    M.rr(SX, SY, SW, SH, 10, MAG, 1.2, 0.55)
    M.ln(circle(1066, 114, 2.4, 12), RED, 1.4, 1.0)
    M.fill(circle(1066, 114, 2.4, 12), RED, 0.9, layer=3)
    M.t(1072, 118, 'ON AIR', RED, 9, 'ls', KO)
    M.t(1243, 118, 'CH 1', FG, 9, 'rs', KO)
    suit_shape(M, 'heart', 1146, 114, 4.2, PINK, 1.2)                  # 목숨 (시안에 없음)
    M.t(1153, 118, '18', PINK, 9.5, 'ls', BLOCK)
    M.t(1153, 135, '- 제 3화 -', FG, 9, 'ms', KO, 0.8)
    M.t(1153, 152, '병정들의 행진', WHITE, 13, 'ms', KO, glow=True)
    for i, (kind, n) in enumerate((('normal', '병정 x 18'), ('fast', '빠른 병정 x 6'), ('shield', '방패 병정 x 1'))):
        yy = 168 + i * 16
        soldier(M, 1070, yy + 1, kind, 1.0, 0.5)
        M.t(1085, yy + 4, n, FG, 9.5, 'ls', KO)
    M.ln([(1063, 306), (1244, 306)], GREY, 1.0, 0.35, glow=False)
    M.t(1063, 320, '남은 적 6/24', FG, 9.5, 'ls', KO)
    M.t(1244, 320, '처치 58 / 놓침 2', FG, 9.5, 'rs', KO)
    # 화면 아래: ×2 · ×4 · 다이얼 · 스피커
    for i, (lab, on) in enumerate((('x2', True), ('x4', False))):
        bx = 1053 + i * 43
        tube(M, bx, 342, 35, 45, 6, CYAN2, on=on, b=None if on else 0.55)
        M.t(bx + 17.5, 361, lab, WHITE if on else FG, 11, 'ms', BLOCK, 1.0 if on else 0.6, glow=on)
        for k in range(2):
            M.ln([(bx + 10 + k * 7, 368), (bx + 16 + k * 7, 373), (bx + 10 + k * 7, 378)], WHITE if on else FG, 1.3, 1.0 if on else 0.55)
    M.ln(circle(1175, 365, 22, 40), PINK, 2.0, 1.0, core=True)
    M.ln(circle(1175, 365, 16, 32), PINK, 1.1, 0.55)
    M.ln([(1175, 365), (1164, 355)], PINK, 2.0, 1.0)
    for k in range(6):
        M.ln([(1219, 345 + k * 8), (1254, 345 + k * 8)], MAG, 1.3, 0.5)
    return (SX, SY, SW, SH)


def cocktail(M, cx, cy, c, on):
    """시안의 마티니 잔: 넓은 V 잔 + 다리 + 받침, 레몬 조각 · 빨대"""
    b = 1.0 if on else 0.22
    M.ln([(cx - 15, cy - 18), (cx + 15, cy - 18), (cx + 3, cy - 2), (cx + 2, cy), (cx - 2, cy), (cx - 3, cy - 2), (cx - 15, cy - 18)], c, 1.5, b)
    M.ln([(cx, cy), (cx, cy + 16)], c, 1.4, b)
    M.ln([(cx - 9, cy + 16), (cx + 9, cy + 16)], c, 1.5, b)
    M.ln(circle(cx + 11, cy - 20, 5.5, 16), GOLD if on else c, 1.2, b)
    M.ln([(cx - 3, cy - 18), (cx - 9, cy - 30)], c, 1.0, b)
    if on: M.fill([(cx - 12, cy - 15), (cx + 12, cy - 15), (cx + 3, cy - 2), (cx - 3, cy - 2)], c, 0.14, layer=3)


def right_column(M):
    scr = tv(M)
    M.t(1153, 449, '칵테일', FG, 14, 'ms', KO, glow=True)
    M.ln([(1053, 443), (1123, 443)], FG, 1.0, 0.4, glow=False)
    M.ln([(1187, 443), (1254, 443)], FG, 1.0, 0.4, glow=False)
    M.t(1254, 465, '3웨이브 후 획득', FG, 8.5, 'rs', KO, 0.85)
    for i, x in enumerate((1067, 1110, 1154, 1197, 1240)):
        cocktail(M, x, 508, [PINK, CYAN2][i] if i < 2 else GREY, i < 2)
    M.ln([(1043, 534), (1265, 534)], BLUE, 1.6, 0.75)
    for yy, c in ((545, PINK), (552, MAG), (562, BLUE), (569, PINK)):
        M.ln([(1038, yy), (1270, yy)], c, 1.3, 0.6)
    # 스킬
    x, y, w, h = 1038, 575, 232, 51
    tube(M, x, y, w, h, 8, BLUE, on=True)
    nodes = [(1056, 600), (1072, 590), (1072, 610), (1086, 600)]
    for a, b_ in ((0, 1), (0, 2), (1, 3), (2, 3)):
        M.ln([nodes[a], nodes[b_]], CYAN2, 1.3, 0.95)
    for (nx, ny) in nodes: M.ln(circle(nx, ny, 3.4, 12), CYAN2, 1.5, 1.0)
    M.t(1162, 609, '스 킬', CYAN2, 24, 'ms', KO, glow=True, hollow=True)
    # 시작
    x, y, w, h = 1038, 636, 232, 73
    tube(M, x, y, w, h, 10, LIME, on=True)
    M.fill_rr(x, y, w, h, 10, LIME, 0.05, layer=3)
    M.ln([(1058, 655), (1090, 672), (1058, 689), (1058, 655)], LIME, 2.4, 1.0, core=True)
    M.t(1165, 688, '시 작', LIME, 38, 'ms', KO, glow=True, hollow=True)
    return scr


# ---------------------------------------------------------------- 아래: 덱 · 손패
def hand_card(M, x, y, idx, rank, sk, cw=59, ch=97):
    col = SUITC[sk]
    M.fill_rr(x, y, cw, ch, 7, (6, 6, 11), 0.92, layer=2)
    M.fill_rr(x, y, cw, ch, 7, col, 0.06, layer=3)
    M.rr(x, y, cw, ch, 7, col, 1.8, 0.95, core=True)
    M.rr(x + 3.5, y + 3.5, cw - 7, ch - 7, 5, col, 0.8, 0.45)
    M.t(x + 8, y + 14, str(idx + 1), FG, 8, 'ls', KO, 0.7)
    suit_shape(M, sk, x + cw / 2, y + 26, 5.5, col, 1.4)
    M.t(x + cw / 2, y + 63, rank, col, 30, 'ms', BLOCK, glow=True, hollow=True)
    M.t(x + cw / 2, y + ch - 10, ATK[sk], col, 8.5, 'ms', KO, 0.9)


def bottom(M):
    M.t(318, 646, '10 / 10', GOLD, 38, 'ms', BLOCK, glow=True, hollow=True)
    M.t(318, 676, '덱 38장', FG, 15, 'ms', KO)
    for i, (r, sk) in enumerate(HAND):
        hand_card(M, 388 + i * 63, 596, i, r, sk)


# ---------------------------------------------------------------- 판 위 폭발 · 화면 주사선
def burst(M, x, y):
    rnd = random.Random(3)
    M.ln(circle(x, y, 18, 32), ORANGE, 1.4, 0.75)
    M.ln(circle(x, y, 9, 24), GOLD, 1.2, 0.9)
    for k in range(16):
        a = k / 16 * 2 * math.pi + rnd.uniform(-0.15, 0.15)
        r0, r1 = rnd.uniform(12, 18), rnd.uniform(24, 40)
        M.ln([(x + math.cos(a) * r0, y + math.sin(a) * r0), (x + math.cos(a) * r1, y + math.sin(a) * r1)], ORANGE if k % 2 else GOLD, 1.3, 0.95)
    for _ in range(22):
        a, r = rnd.uniform(0, 2 * math.pi), rnd.uniform(16, 52)
        px, py = x + math.cos(a) * r, y + math.sin(a) * r
        M.ln([(px - 1.2, py), (px + 1.2, py)], GOLD, 2.4, 0.95)
    M.dot(x, y, 2.4)


def scanlines(img, box, a=55):
    sx, sy, sw, sh = box
    d = ImageDraw.Draw(img, 'RGBA')
    for yy in range(int(sy + 3), int(sy + sh - 3), 3):
        d.line([(sx + 4, yy), (sx + sw - 4, yy)], fill=(0, 0, 0, a))
    return img


def mock():
    M = Bar(gk=0.8)
    left_panel(M)
    wave_sign(M)
    G.board(M, FX, FY, CS, frame_c=CYAN2, frame_b=0.6, path_style='dash', ghost=False)
    burst(M, 360, 166)
    scr = right_column(M)
    img = scanlines(M.draw(), scr)
    P = Bar(gk=0.8)
    bottom(P)
    P.ln(circle(1251, 45, 14, 32), FG, 1.8, 0.85)
    P.t(1251, 52, '?', WHITE, 18, 'ms', BLOCK, glow=True)
    return P.draw(img)


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    mock().save(os.path.join(outdir, 'alice_ui_mock.png'))
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
