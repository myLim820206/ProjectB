"""새 UI 시안(2026-11-19 사용자 그림)을 우리 화풍(docs/15)으로 옮긴 인게임 시안

    python tools/neon/alice_ui_mock.py 출력폴더   → alice_ui_mock.png

배치 (시안 그대로):
- 왼쪽 긴 패널: BAR Alice 로고 + "족보 메뉴" 간판, 5장 / 2~4장 / 조커·먹이 묶음, 줄마다 "이름 ····· ×배수" + 설명, 연쇄 안내
- 가운데 판 (위쪽 띠 없음), 아래 왼쪽 큰 "10 / 10 · 덱 38장", 아래 손패 10장 (번호 · 무늬 · 숫자 · 공격 방식)
- 오른쪽 위 배속 ×2 · ×4 · 도움말, 그 아래 TV (ON AIR · 제 n화 · 적 구성 · 남은 적 · 처치/놓침 · 다이얼 · 채널 점)
- 칵테일 선반 5칸, 스킬 버튼(양옆 줄무늬), 맨 아래 큰 초록 시작 버튼
시안의 채운 버튼 · 그라데이션 · 반짝이는 플라스틱은 화풍대로 '채우지 않은 두 겹 튜브 + 옅은 채움'으로 바꿨다.
시안에 없던 목숨은 TV 화면 맨 위에 넣었다.
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
import ingame_mock as G  # noqa: E402
from ingame_mock import Bar, tube, suit_shape, soldier, rr_pts, hx, dim, circle  # noqa: E402
from ingame_mock import KO, KOR, BLOCK, WHITE, FG, GREY, DIMT, GOLD, PINK, JPINK, GREEN, RED, SUITC, ATK  # noqa: E402
from logo_alice import draw_bar, draw_alice, martini  # noqa: E402

MAG = hx('e07aff')       # 패널 테두리 (시안의 자홍)
BLUE = hx('6a8cff')      # 스킬 버튼
LIME = hx('9cffb0')      # 시작 버튼
CYAN2 = hx('7fd6ff')

# ---------------------------------------------------------------- 판 상태 (시안 배치 비슷하게)
G.PATH_WP = [(0, 7), (1, 7), (1, 1), (4, 1), (4, 5), (8, 5), (8, 1), (11, 1), (11, 7), (15, 7)]
G.TOWERS = {
    (2, 2): ('4', 'heart', 1), (3, 2): ('5', 'diamond', 2), (3, 3): ('6', 'heart', 1), (2, 3): ('K', 'spade', 1),
    (5, 1): ('10', 'heart', 2), (5, 2): ('9', 'heart', 1), (5, 3): ('8', 'heart', 1),
    (6, 3): ('K', 'club', 1), (5, 4): ('3', 'diamond', 1),
}
G.JOKER = (7, 2)
G.FLOORS = [([(2, 2), (3, 2), (3, 3)], hx('6a8cff')),     # 스트레이트 4-5-6
            ([(5, 1), (5, 2), (5, 3)], hx('b48cff'))]     # 플러시 ♥ 셋
G.ENEMIES = [(0.02, 'normal', 1.0), (0.05, 'normal', 0.9), (0.08, 'fast', 0.7), (0.11, 'normal', 0.6),
             (0.15, 'normal', 0.8), (0.19, 'fast', 0.5), (0.23, 'normal', 0.4)]
G.GHOST = (9, 3)

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
    'joker': ('조커', '×1', '클릭: 만들 수 있는 조커 목록', 'ff9ad5'),
    'feed': ('먹이기', '', '클릭: 조커에 족보 먹이기', 'ff9ad5'),
}
GROUPS = [('5장', ['royal', 'five', 'sf', 'full', 'flush', 'straight']), ('2~4장', ['four', 'triple', 'twopair', 'pair']),
          ('조커 / 먹이', ['joker', 'feed'])]
ON_BOARD = {'flush', 'straight', 'joker'}   # 판에 있는 족보 (색이 켜짐)
HOVER = 'full'                               # 마우스를 올린 줄 (시안의 금색 줄)
HAND = [('6', 'heart'), ('2', 'heart'), ('8', 'spade'), ('9', 'club'), ('6', 'spade'),
        ('5', 'diamond'), ('Q', 'heart'), ('3', 'club'), ('2', 'diamond'), ('A', 'diamond')]
SEL = 6


def group_head(M, cx, y, title, w=200):
    tw = M.width(title, 12, KO)
    M.t(cx, y + 4, title, FG, 12, 'ms', KO)
    for s in (-1, 1):
        M.ln([(cx + s * (tw / 2 + 10), y), (cx + s * (w / 2), y)], MAG, 1.0, 0.45, glow=False)


def menu_row(M, x, y, w, key):
    n, mult, desc, hc = HANDS[key]
    c = hx(hc)
    hov, on = key == HOVER, key in ON_BOARD
    h = 33
    if hov:
        tube(M, x, y, w, h, 7, GOLD, on=True)
    else:
        M.fill_rr(x, y, w, h, 6, (10, 10, 18), 0.9, layer=2)
        M.rr(x, y, w, h, 6, c if on else GREY, 1.0, 0.55 if on else 0.3, glow=on)
    # 왼쪽 족보 색 막대
    M.ln([(x + 6, y + 7), (x + 6, y + h - 7)], c, 2.2, 1.0 if (on or hov) else 0.45)
    nc = WHITE if hov else c if on else FG
    nw = M.t(x + 14, y + 15, n, nc, 11, 'ls', KO, maxw=w - 60)
    pw = M.t(x + w - 8, y + 15, mult, GOLD if hov else FG, 11, 'rs', BLOCK) if mult else 0
    for dx in range(int(x + 18 + nw), int(x + w - 12 - pw), 5):
        M.ln([(dx, y + 12), (dx + 1.3, y + 12)], DIMT, 1.0, 0.5, glow=False)
    M.t(x + 14, y + 28, desc, (255, 230, 170) if hov else GREY, 8.5, 'ls', KOR, maxw=w - 22)
    return h


def left_panel(M):
    X, Y, W, H = 12, 8, 232, 704
    tube(M, X, Y, W, H, 14, MAG, b=0.62)
    # 로고 (BAR 작게 + Alice 필기체 + 마티니)
    draw_bar(M, 30, 22, 0.19, col=CYAN2, wk=0.5)
    draw_alice(M, 52, 60, 0.36, col=PINK)
    martini(M, 148, 38, 9, GOLD)
    M.t(128, 92, '족보 메뉴', GOLD, 22, 'ms', KO, glow=True, hollow=True)
    y = 112
    for gi, (title, keys) in enumerate(GROUPS):
        group_head(M, 128, y, title)
        y += 12
        for k in keys:
            y += menu_row(M, 24, y, 208, k) + 4
        y += 14
    M.ln([(28, 676), (228, 676)], MAG, 1.0, 0.3, glow=False)
    M.t(30, 697, '연쇄', WHITE, 11, 'ls', KO)
    M.t(62, 697, '족보끼리 맞닿으면 하나에 +25%씩', GREY, 9, 'ls', KOR)


def speed_btn(M, x, y, w, h, label, on):
    tube(M, x, y, w, h, 8, CYAN2, on=on, b=None if on else 0.6)
    ax, ay = x + 18, y + h / 2
    for k in range(2):
        M.ln([(ax + k * 9, ay - 7), (ax + k * 9 + 8, ay), (ax + k * 9, ay + 7)], WHITE if on else FG, 1.6, 1.0 if on else 0.6)
    M.t(x + w - 16, y + h / 2 + 6, label, WHITE if on else FG, 17, 'rs', BLOCK, 1.0 if on else 0.6, glow=on)


def cocktail(M, cx, cy, c, on):
    b = 1.0 if on else 0.22
    M.ln([(cx - 13, cy - 16), (cx + 13, cy - 16), (cx + 3, cy - 2), (cx + 2, cy), (cx - 2, cy), (cx - 3, cy - 2), (cx - 13, cy - 16)], c, 1.4, b)
    M.ln([(cx, cy), (cx, cy + 14)], c, 1.4, b)
    M.ln([(cx - 8, cy + 14), (cx + 8, cy + 14)], c, 1.4, b)
    M.ln(circle(cx + 10, cy - 18, 5, 16), GOLD if on else c, 1.2, b)      # 레몬 조각
    M.ln([(cx - 3, cy - 16), (cx - 9, cy - 28)], c, 1.0, b)               # 빨대
    if on:
        M.fill([(cx - 11, cy - 13), (cx + 11, cy - 13), (cx + 3, cy - 2), (cx - 3, cy - 2)], c, 0.12, layer=3)


def tv(M, X, Y, W, H):
    # 안테나
    M.ln([(X + 60, Y), (X + 42, Y - 14)], MAG, 1.4, 0.6)
    M.ln([(X + 60, Y), (X + 84, Y - 12)], MAG, 1.4, 0.6)
    tube(M, X, Y, W, H, 16, MAG, b=0.85)
    # 화면
    SX, SY, SW, SH = X + 14, Y + 14, W - 28, 240
    M.fill_rr(SX, SY, SW, SH, 12, (4, 6, 10), 0.95, layer=3)
    for yy in range(int(SY + 4), int(SY + SH - 4), 3):
        M.bg([(SX + 6, yy), (SX + SW - 6, yy)], (120, 140, 180), 0.035)
    M.rr(SX, SY, SW, SH, 12, MAG, 1.2, 0.55)
    M.ln(circle(SX + 14, SY + 17, 2.6, 12), RED, 1.4, 1.0)
    M.fill(circle(SX + 14, SY + 17, 2.6, 12), RED, 0.9, layer=3)
    M.t(SX + 22, SY + 21, 'ON AIR', RED, 9, 'ls', KO)
    M.t(SX + SW - 12, SY + 21, 'CH 1', FG, 9, 'rs', KO)
    # 목숨 (시안에 없던 것)
    suit_shape(M, 'heart', SX + SW / 2 - 22, SY + 17, 4.5, PINK, 1.2)
    M.t(SX + SW / 2 - 14, SY + 21, '목숨 18', PINK, 10, 'ls', KO)
    M.t(SX + SW / 2, SY + 46, '- 제 3화 -', GREY, 10, 'ms', KO)
    M.t(SX + SW / 2, SY + 68, '병정들의 행진', WHITE, 17, 'ms', KO, glow=True)
    for i, (kind, n, cnt) in enumerate((('normal', '병정', 18), ('fast', '빠른 병정', 6))):
        yy = SY + 104 + i * 30
        soldier(M, SX + 32, yy + 4, kind, 1.0, 0.75)
        M.t(SX + 58, yy + 9, '%s  × %d' % (n, cnt), FG, 12, 'ls', KO)
    # 진행 막대 + 숫자
    by = SY + SH - 36
    M.ln([(SX + 12, by), (SX + SW - 12, by)], GREY, 1.0, 0.35, glow=False)
    M.ln([(SX + 12, by), (SX + 12 + (SW - 24) * 18 / 24, by)], CYAN2, 1.6, 0.8)
    M.t(SX + 12, SY + SH - 14, '남은 적 6/24', FG, 10, 'ls', KO)
    M.t(SX + SW - 12, SY + SH - 14, '처치 58 / 놓침 2', FG, 10, 'rs', KO)
    # 아래: 스피커 줄 · 다이얼 · 채널 점
    ky = SY + SH + 38
    for k in range(6):
        M.ln([(SX + 6, ky - 15 + k * 6), (SX + 52, ky - 15 + k * 6)], MAG, 1.0, 0.4, glow=False)
    M.ln(circle(X + W / 2, ky, 21, 40), PINK, 1.6, 0.9)
    M.ln(circle(X + W / 2, ky, 16, 36), PINK, 1.0, 0.5)
    M.ln([(X + W / 2, ky), (X + W / 2 - 11, ky - 10)], PINK, 1.6, 0.9)
    for k in range(3):
        cx = X + W - 60 + k * 18
        M.ln(circle(cx, ky, 5.5, 18), MAG, 1.5 if k == 0 else 1.0, 1.0 if k == 0 else 0.4)


def right_column(M):
    X, W = 1048, 222
    speed_btn(M, X, 22, 94, 34, '×2', True)
    speed_btn(M, X + 102, 22, 94, 34, '×4', False)
    M.ln(circle(1253, 39, 15, 32), FG, 1.6, 0.8)
    M.t(1253, 46, '?', WHITE, 18, 'ms', KO, glow=True)
    tv(M, X, 76, W, 330)
    # 칵테일 선반
    y = 420
    M.t(X + W / 2, y + 14, '칵테일', FG, 13, 'ms', KO)
    for s in (-1, 1):
        M.ln([(X + W / 2 + s * 30, y + 9), (X + W / 2 + s * 100, y + 9)], MAG, 1.0, 0.45, glow=False)
    M.t(X + W - 6, y + 34, '3웨이브 후 획득', GREY, 9, 'rs', KO)
    for i in range(5):
        cocktail(M, X + 24 + i * 43.5, y + 70, [PINK, CYAN2][i] if i < 2 else GREY, i < 2)
    M.ln([(X + 4, y + 90), (X + W - 4, y + 90)], hx('7f8cff'), 1.6, 0.7)
    # 스킬 버튼 (양옆 줄무늬)
    sy = 532
    tube(M, X + 44, sy, W - 88, 50, 25, BLUE, on=True)
    M.t(X + W / 2, sy + 34, '스 킬', WHITE, 22, 'ms', KO, glow=True)
    for s, x0 in ((-1, X + 4), (1, X + W - 36)):
        for k, c in enumerate((PINK, CYAN2, PINK)):
            M.ln([(x0, sy + 17 + k * 8), (x0 + 32, sy + 17 + k * 8)], c, 1.4, 0.75)
    # 시작 버튼
    gy = 604
    tube(M, X, gy, W, 82, 14, LIME, on=True)
    M.fill_rr(X, gy, W, 82, 14, LIME, 0.05, layer=3)
    M.ln([(X + 30, gy + 24), (X + 58, gy + 41), (X + 30, gy + 58), (X + 30, gy + 24)], LIME, 2.2, 1.0)
    M.t(X + 140, gy + 54, '시 작', WHITE, 34, 'ms', KO, glow=True)
    M.t(X + W - 12, gy + 74, 'Space', DIMT, 9, 'rs', KO)


def hand_card(M, x, y, idx, rank, sk, sel, cw=58, ch=94):
    col = PINK if sel else SUITC[sk]
    M.fill_rr(x, y, cw, ch, 8, (6, 6, 11), 0.92, layer=2)
    M.fill_rr(x, y, cw, ch, 8, col, 0.10 if sel else 0.04, layer=3)
    M.rr(x, y, cw, ch, 8, col, 2.2 if sel else 1.5, 1.0 if sel else 0.75)
    M.rr(x + 4, y + 4, cw - 8, ch - 8, 5, col, 0.8, 0.5 if sel else 0.3)
    M.t(x + 9, y + 16, str(idx + 1), DIMT if not sel else FG, 8, 'ls', KO)
    suit_shape(M, sk, x + cw / 2, y + 26, 6, col, 1.4)
    M.t(x + cw / 2, y + 64, rank, WHITE if sel else col, 30, 'ms', BLOCK, glow=sel)
    M.t(x + cw / 2, y + ch - 9, ATK[sk], WHITE if sel else col, 8.5, 'ms', KO, 1.0 if sel else 0.8)
    if sel:
        suit_shape(M, 'heart', x + cw / 2, y - 12, 6, PINK, 1.4)


def bottom(M):
    M.t(322, 668, '10 / 10', GOLD, 36, 'ms', BLOCK, glow=True, hollow=True)
    M.t(322, 696, '덱 38장', FG, 14, 'ms', KO)
    x0, gap, cw = 402, 5, 58
    for i, (r, sk) in enumerate(HAND):
        hand_card(M, x0 + i * (cw + gap), 612 - (10 if i == SEL else 0), i, r, sk, i == SEL)


def mock():
    M = Bar(gk=0.8)
    left_panel(M)
    G.board(M, 264, 92, 48, frame_c=CYAN2, frame_b=0.55)
    right_column(M)
    img = M.draw()
    P = Bar(gk=0.8)
    bottom(P)
    return P.draw(img)


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    mock().save(os.path.join(outdir, 'alice_ui_mock.png'))
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
