"""새 UI 시안(사용자 그림, 2026-11-20 판)을 자리 · 크기 · 색감 · 위쪽 간판 그림까지 그대로 옮긴 인게임 시안

    python tools/neon/alice_ui_mock.py 출력폴더   → alice_ui_mock.png

사용자 요청으로 이 시안은 docs/15 의 '채우지 않은 선' 규칙 대신 시안 색감을 따른다 (D-399):
칠한 카드(무늬 색 어두운 채움 + 굵은 네온 테두리), 칠한 버튼(진한 파랑 스킬 · 진한 초록 시작 · 회색 배속),
보랏빛 패널 바탕, 판 안은 검정, 무늬 색은 시안대로 ♥ 빨강 · ♠ 은회색 · ♣ 파랑 · ♦ 노랑.
시안 그림(2000 × 1125) 좌표 × 0.64 = 1280 × 720 좌표.
- 왼쪽 패널 (12~255): BAR 알약 + Alice 필기체 + 칵테일 잔, 금색 '족보 메뉴', 5장 / 2~4장 / 조커/먹이, 줄은 어두운 상자 +
  왼쪽 족보 색 막대 + 흰 이름 ···· 배수 + 회색 설명, 마우스를 올린 줄(풀하우스)은 금빛 채움 + 노란 테두리
- 위: 초록 잎 · 주황 줄기 야자수, 분홍 · 노랑 · 하늘 네온 줄, 분홍 타원 안 하늘색 기운 'WAVE', 파란 테 상자 안 속이 검은 '3/20'
- 판 (262~1029, 86~568): 하늘빛 테, 안은 검정, 길은 어두운 띠 + 점선
- TV: 흰 안테나, 보라 테 + 보랏빛 몸통, 지직거리는 화면, 회색 ×2 · ×4, 분홍 다이얼, 보라 스피커 줄
- 칵테일 선반, 스킬 · 시작 버튼, 금색 '10 / 10' · 덱 38장, 손패 10장
"""
import math
import os
import random
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

sys.path.insert(0, os.path.dirname(__file__))
import ingame_mock as G  # noqa: E402
from ingame_mock import Bar, soldier, hx, circle, rr_pts  # noqa: E402
from ingame_mock import KO, KOR, BLOCK, WHITE  # noqa: E402
from neon import bez, suit  # noqa: E402
from logo_alice import ALICE, catmull  # noqa: E402

W, H = G.W, G.H
COND = 'COND'   # 숫자 글꼴: Bahnschrift Bold Condensed (시안 카드 숫자)
BOLD = 'BOLD'   # Bahnschrift Bold

# ---------------------------------------------------------------- 시안 색
NAVY = (6, 9, 21)
PANEL_TOP, PANEL_BOT = (30, 11, 42), (14, 8, 35)
PURPLE = (190, 115, 255)
TEAL = (70, 200, 230)
TXT, SUB = (235, 235, 242), (165, 160, 180)
YEL = (255, 225, 110)
PINKN = (255, 70, 190)
CYANN = (90, 225, 255)
BLUEN = (45, 140, 255)
GREENN = (90, 240, 110)
ORANGEN = (255, 150, 40)
REDN = (255, 70, 90)
SUITR = {'heart': (255, 70, 115), 'spade': (210, 212, 220), 'club': (70, 160, 255), 'diamond': (255, 205, 50)}
SUITF = {'heart': (78, 14, 36), 'spade': (52, 53, 58), 'club': (8, 38, 70), 'diamond': (58, 48, 6)}
ATK = {'spade': '저격', 'heart': '폭발', 'diamond': '연사', 'club': '둔화'}

G.BG = NAVY
G.NEON_K = 1.0
G.SUITC.update(SUITR)
G.FRAME = PURPLE
_font0 = G.font


def _font(path, px):
    if path in (COND, BOLD):
        key = (path, int(px * G.S))
        if key not in G._FONTS:
            f = ImageFont.truetype(BLOCK, int(px * G.S))
            f.set_variation_by_name('Bold Condensed' if path == COND else 'Bold')
            G._FONTS[key] = f
        return G._FONTS[key]
    return _font0(path, px)


G.font = _font

FX, FY, CS = 267, 93, 47

# ---------------------------------------------------------------- 판 상태 (시안의 칸 그대로)
G.PATH_WP = [(0, 6), (1, 6), (1, 1), (4, 1), (4, 5), (8, 5), (8, 1), (11, 1), (11, 7), (15, 7)]
G.TOWERS = {
    (2, 2): ('4', 'heart', 1), (3, 2): ('5', 'diamond', 1), (2, 3): ('K', 'club', 1), (3, 3): ('6', 'heart', 1),
    (5, 1): ('10', 'heart', 1), (5, 2): ('9', 'heart', 1), (5, 3): ('8', 'club', 1),
    (6, 3): ('K', 'spade', 1), (5, 4): ('3', 'diamond', 1),
}
TOWER_COL = {(2, 2): (255, 80, 170)}   # 시안의 4♥ 는 분홍
G.JOKER = None
G.FLOORS = [([(2, 2), (3, 2)], (200, 110, 255)), ([(5, 1), (5, 2), (5, 3)], (160, 110, 255))]
G.ENEMIES = [(0.010, 'normal', 1.0), (0.032, 'normal', 1.0), (0.056, 'normal', 0.9), (0.075, 'fast', 0.8),
             (0.093, 'normal', 0.8), (0.111, 'normal', 0.7), (0.129, 'normal', 0.6), (0.147, 'fast', 0.5), (0.165, 'normal', 0.3)]

HANDS = {  # 이름, 배수, 설명, 막대 색
    'royal': ('로열 스트레이트 플러시', 'x2.5', '공격력 x2.5 / 연사 x2.5 / 넓은 범위', (200, 200, 215)),
    'five': ('파이브 카드', 'x5', '공격력 x5 (와일드 또는 조커 필요)', (150, 90, 40)),
    'sf': ('스트레이트 플러시', 'x2', '공격력 x2 / 연사 x2 / 범위', (150, 60, 170)),
    'full': ('풀하우스', 'x1.5', '공격력 x1.5 / 연사 x1.5 / 범위', (230, 200, 70)),
    'flush': ('플러시 (3~5장)', '', '범위 x 카드 수', (100, 70, 220)),
    'straight': ('스트레이트', 'x1.25', '연사 x1.25 / x1.5 / x1.75 / x2', (60, 90, 220)),
    'four': ('포카드', 'x3.5', '공격력 x3.5', (220, 60, 80)),
    'triple': ('트리플', 'x2', '공격력 x2', (70, 170, 110)),
    'twopair': ('투 페어', 'x1.5', '공격력 x1.5 / 범위 x1.25', (70, 150, 220)),
    'pair': ('원 페어', 'x1.5', '공격력 x1.5', (180, 185, 200)),
    'joker': ('조커', 'x1', '클릭 시 생성 가능 조커 목록', (230, 90, 170)),
    'feed': ('먹이기', '', '클릭 시 조커에게 족보 먹이기', (230, 90, 170)),
}
GROUPS = [('5장', 104, 123, ['royal', 'five', 'sf', 'full', 'flush', 'straight']),
          ('2~4장', 374, 392, ['four', 'triple', 'twopair', 'pair']),
          ('조커/먹이', 568, 586, ['joker', 'feed'])]
ROW_P, ROW_H = 37.4, 31
HOVER = 'full'
HAND = [('6', 'heart'), ('2', 'heart'), ('8', 'spade'), ('9', 'club'), ('6', 'spade'),
        ('5', 'diamond'), ('4', 'club'), ('3', 'club'), ('2', 'diamond'), ('1', 'diamond')]


def lerp(a, b, t): return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def grad_rr(M, x, y, w, h, r, c0, c1, layer=1, n=18):
    """세로 그라데이션 채움 (둥근 사각 안을 띠로)"""
    M.fill_rr(x, y, w, h, r, c1, 1.0, layer=layer)
    for i in range(n):
        t = i / n
        yy = y + r + (h - 2 * r) * t
        M.fill([(x + 1, yy), (x + w - 1, yy), (x + w - 1, yy + (h - 2 * r) / n + 1), (x + 1, yy + (h - 2 * r) / n + 1)], lerp(c0, c1, t), 1.0, layer=layer)
    M.fill_rr(x + 2, y, w - 4, r * 2, r, c0, 1.0, layer=layer)


def neon_box(M, x, y, w, h, r, c, fill=None, a=0.92, wd=2.4, inner=True, layer=2):
    if fill: M.fill_rr(x, y, w, h, r, fill, a, layer=layer)
    M.rr(x, y, w, h, r, c, wd, 1.0, core=True)
    if inner: M.rr(x + 4, y + 4, w - 8, h - 8, max(2, r - 4), c, 1.0, 0.45, glow=False)


def suit_line(M, k, x, y, r, c, wd=1.4, b=1.0):
    for p in suit(k)(x, y, r): M.ln(p, c, wd, b)


# ---------------------------------------------------------------- 판 위 타워 (칠한 둥근 사각 + 두 겹 테)
def ref_tower(M, cx, cy, CS_, rank, sk, lvl, b=1.0, col=None):
    cell = (round((cx - FX) / CS - 0.5), round((cy - FY) / CS - 0.5))
    c = TOWER_COL.get(cell, SUITR[sk])
    h = CS_ * 0.36
    M.fill_rr(cx - h, cy - h, 2 * h, 2 * h, 5, lerp(SUITF[sk], (0, 0, 0), 0.35), 1.0, layer=2)
    M.rr(cx - h, cy - h, 2 * h, 2 * h, 5, c, 1.6, 0.85)
    M.rr(cx - h + 3, cy - h + 3, 2 * h - 6, 2 * h - 6, 3, c, 0.8, 0.5, glow=False)
    M.t(cx, cy + 1, rank, c, 13, 'ms', COND)
    suit_line(M, sk, cx, cy + h * 0.55, 3.4, c, 1.0)


G.tower = ref_tower


# ---------------------------------------------------------------- 왼쪽 패널
def menu_row(M, x, y, w, key):
    n, mult, desc, bar = HANDS[key]
    hov = key == HOVER
    h = ROW_H
    if hov:
        M.fill_rr(x, y, w, h, 6, (88, 82, 48), 1.0, layer=2)
        M.rr(x, y, w, h, 6, (245, 230, 110), 2.2, 1.0, core=True)
    else:
        M.fill_rr(x, y, w, h, 6, (22, 16, 34), 1.0, layer=2)
        M.rr(x, y, w, h, 6, (150, 145, 165), 1.3, 0.75, glow=False)
    M.ln([(x + 6, y + 6), (x + 6, y + h - 6)], bar, 3.0, 1.0, glow=False)
    nw = M.t(x + 14, y + 14, n, TXT, 11, 'ls', KO, maxw=w - 60)
    pw = M.t(x + w - 8, y + 14, mult, TXT, 11.5, 'rs', KO) if mult else 0
    for dx in range(int(x + 20 + nw), int(x + w - 14 - pw), 7):
        M.ln([(dx, y + 10), (dx + 1.6, y + 10)], (200, 200, 210), 1.4, 0.8, glow=False)
    M.t(x + 14, y + 26, desc, SUB, 8.4, 'ls', KOR, maxw=w - 20)


def group_head(M, cx, y, title):
    tw = M.width(title, 13, KO)
    M.t(cx, y + 5, title, TXT, 13, 'ms', KO)
    for s in (-1, 1):
        M.ln([(cx + s * (tw / 2 + 10), y), (cx + s * 100, y)], (190, 170, 80), 1.6, 0.8)


def left_panel(M):
    grad_rr(M, 12, 10, 243, 699, 14, PANEL_TOP, PANEL_BOT, layer=0)
    M.rr(12, 10, 243, 699, 14, PURPLE, 2.0, 0.9, core=True)
    # 로고: 기운 BAR 알약 + Alice + 칵테일 잔
    M.rr(19, 22, 30, 13, 6.5, PINKN, 1.4, 1.0)
    M.t(34, 32, 'BAR', CYANN, 8.5, 'ms', BOLD)
    for letter in ALICE:  # Alice 필기체 (굵고 밝게)
        for st in letter:
            M.ln([(46 + px * 0.27, 46 + py * 0.27) for px, py in catmull(st)], PINKN, 1.8, 1.0, core=True)
    gx, gy = 104, 24
    M.ln([(gx - 9, gy - 6), (gx + 9, gy - 6), (gx, gy + 4), (gx - 9, gy - 6)], CYANN, 1.5, 1.0)
    M.ln([(gx, gy + 4), (gx, gy + 12)], CYANN, 1.3, 1.0)
    M.ln([(gx - 5, gy + 12), (gx + 5, gy + 12)], CYANN, 1.3, 1.0)
    M.ln(circle(gx + 8, gy - 9, 4, 14), ORANGEN, 1.3, 1.0)
    M.ln([(gx - 2, gy - 6), (gx + 4, gy - 16)], YEL, 1.0, 1.0)
    M.small_glow = 1.0
    M.t(133, 76, '족보 메뉴', YEL, 25, 'ms', KO, glow=True)
    for title, hy, ry, keys in GROUPS:
        group_head(M, 133, hy, title)
        for i, k in enumerate(keys):
            menu_row(M, 24, ry + i * ROW_P, 218, k)
    M.t(28, 690, '연쇄', TXT, 11.5, 'ls', KO)
    M.t(60, 690, '족보끼리 맞닿을 경우, 개당 +25%', SUB, 9, 'ls', KOR)


# ---------------------------------------------------------------- 위: 야자수 WAVE 간판
def palm(M):
    base, top = (278, 77), (300, 22)
    L = bez(base, (294, 54), top, 16)
    R = [(px + 7, py + 1) for px, py in bez((285, 77), (302, 54), (303, 24), 16)]
    M.ln(L, ORANGEN, 2.0, 1.0, core=True); M.ln(R, ORANGEN, 2.0, 1.0, core=True)
    for i in range(2, 15, 3): M.ln([L[i], R[i]], ORANGEN, 1.4, 0.9)
    tx, ty = 304, 21
    leaves = ((-30, 14, -16, -12, -22, 4), (-24, -12, -10, -16, -14, -6), (4, -16, 0, -18, 2, -10),
              (30, -8, 16, -18, 18, -4), (30, 20, 22, -4, 16, 8))
    for (ex, ey, cx, cy, ix, iy) in leaves:
        outer = bez((tx, ty), (tx + cx, ty + cy), (tx + ex, ty + ey), 14)
        inner = bez((tx + ex, ty + ey), (tx + ix, ty + iy + 4), (tx, ty + 2), 14)
        M.ln(outer + inner, GREENN, 1.8, 1.0, core=True)


def wave_sign(M, wave=3, goal=20):
    palm(M)
    M.ln([(328, 50), (408, 50)], PINKN, 2.2, 1.0, core=True)
    for (x0, x1) in ((314, 381), (390, 420)): M.ln([(x0, 64), (x1, 64)], YEL, 2.4, 1.0, core=True)
    for (x0, x1) in ((322, 344), (352, 420)): M.ln([(x0, 74), (x1, 74)], CYANN, 1.8, 0.9)
    cx, cy = 485, 42
    M.ln(circle(cx, cy, 35, 64, sx=2.3), PINKN, 3.0, 1.0, core=True)
    M.ln([(cx + math.cos(math.radians(a)) * 92, cy + math.sin(math.radians(a)) * 42) for a in range(195, 262, 6)], BLUEN, 2.0, 0.9)
    M.ln([(cx + math.cos(math.radians(a)) * 92, cy + math.sin(math.radians(a)) * 42) for a in range(15, 80, 6)], BLUEN, 2.0, 0.6)
    M.rr(562, 9, 191, 72, 4, BLUEN, 2.6, 1.0, core=True)


def wave_word(img):
    """'WAVE': 하늘색 굵은 기운 글자 + 넓은 빛 (시안처럼 이탤릭)"""
    S2 = 2
    big = img.resize((W * S2, H * S2), Image.BICUBIC)
    lay = Image.new('RGBA', (220 * S2, 90 * S2), (0, 0, 0, 0))
    d = ImageDraw.Draw(lay)
    f = ImageFont.truetype(BLOCK, 44 * S2); f.set_variation_by_name('Bold')
    d.text((110 * S2, 48 * S2), 'WAVE', font=f, anchor='mm', fill=(170, 245, 255, 255), stroke_width=3 * S2, stroke_fill=(170, 245, 255, 255))
    d.text((110 * S2, 48 * S2), 'WAVE', font=f, anchor='mm', fill=(60, 195, 245, 255))
    sh = 0.28
    lay = lay.transform(lay.size, Image.AFFINE, (1, sh, -sh * lay.size[1] / 2, 0, 1, 0), Image.BICUBIC)
    ox, oy = (485 - 110) * S2, (42 - 46) * S2
    glow = Image.new('RGB', big.size, (0, 0, 0))
    gl = Image.new('RGB', lay.size, (40, 200, 255))
    glow.paste(gl, (ox, oy), lay.split()[3])
    big = ImageChops.add(big, glow.filter(ImageFilter.GaussianBlur(9 * S2)).point(lambda v: int(v * 0.8)))
    big = ImageChops.add(big, glow.filter(ImageFilter.GaussianBlur(3 * S2)).point(lambda v: int(v * 0.5)))
    big.paste(lay.convert('RGB'), (ox, oy), lay.split()[3])
    return big.resize((W, H), Image.LANCZOS)


def decal(img, text, cx, cy, fp, px, fill, stroke, sw, glowc, shear=0.0):
    """글꼴 글자를 굵은 테두리 + 채움 + 넓은 빛으로 (시안의 '3/20': 속은 검고 테두리만 파랗게 빛남)"""
    S2 = 2
    big = img.resize((W * S2, H * S2), Image.BICUBIC)
    f = G.font(fp, px * S2 / G.S)
    tw = int(f.getlength(text)) + 40 * S2
    lay = Image.new('RGBA', (tw, int(px * 1.6 * S2)), (0, 0, 0, 0))
    d = ImageDraw.Draw(lay)
    mid = (tw // 2, lay.size[1] // 2)
    d.text(mid, text, font=f, anchor='mm', fill=stroke + (255,), stroke_width=sw * S2, stroke_fill=stroke + (255,))
    d.text(mid, text, font=f, anchor='mm', fill=fill + (255,))
    if shear: lay = lay.transform(lay.size, Image.AFFINE, (1, shear, -shear * lay.size[1] / 2, 0, 1, 0), Image.BICUBIC)
    ox, oy = cx * S2 - mid[0], cy * S2 - mid[1]
    glow = Image.new('RGB', big.size, (0, 0, 0))
    edge = Image.new('RGBA', lay.size, (0, 0, 0, 0))
    ImageDraw.Draw(edge).text(mid, text, font=f, anchor='mm', fill=(0, 0, 0, 0), stroke_width=sw * S2, stroke_fill=(255, 255, 255, 255))
    glow.paste(Image.new('RGB', lay.size, glowc), (ox, oy), edge.split()[3])
    big = ImageChops.add(big, glow.filter(ImageFilter.GaussianBlur(10 * S2)).point(lambda v: int(v * 0.9)))
    big = ImageChops.add(big, glow.filter(ImageFilter.GaussianBlur(3 * S2)).point(lambda v: int(v * 0.6)))
    big.paste(lay.convert('RGB'), (ox, oy), lay.split()[3])
    # 테두리 가운데 밝은 심
    core = Image.new('RGBA', lay.size, (0, 0, 0, 0))
    ImageDraw.Draw(core).text(mid, text, font=f, anchor='mm', fill=(0, 0, 0, 0), stroke_width=max(1, sw * S2 // 3), stroke_fill=lerp(stroke, WHITE, 0.5) + (200,))
    big.paste(core.convert('RGB'), (ox, oy), core.split()[3])
    d2 = ImageDraw.Draw(big)
    d2.text((cx * S2, cy * S2), text, font=f, anchor='mm', fill=fill)
    return big.resize((W, H), Image.LANCZOS)


# ---------------------------------------------------------------- 오른쪽: TV · 칵테일 · 스킬 · 시작
def tv(M):
    X, Y, Wd, Hd = 1038, 83, 232, 317
    for tip in ((1188, 5), (1218, 5)):
        M.ln([(1203, 84), tip], (215, 215, 230), 3.0, 1.0, core=True)
    M.fill_rr(1186, 74, 34, 14, 6, (60, 50, 80), 1.0, layer=2)
    grad_rr(M, X, Y, Wd, Hd, 16, (36, 14, 54), (22, 10, 40), layer=1)
    M.rr(X, Y, Wd, Hd, 16, PURPLE, 2.6, 1.0, core=True)
    M.rr(X + 5, Y + 5, Wd - 10, Hd - 10, 12, PURPLE, 1.0, 0.45, glow=False)
    SX, SY, SW, SH = 1053, 102, 201, 228
    M.fill_rr(SX, SY, SW, SH, 10, (7, 15, 20), 1.0, layer=2)
    M.rr(SX, SY, SW, SH, 10, (200, 150, 255), 1.6, 0.9)
    M.ln(circle(1065, 114, 2.0, 12), REDN, 1.4, 1.0)
    M.fill(circle(1065, 114, 2.0, 12), REDN, 1.0, layer=3)
    M.t(1070, 118, 'ON AIR', REDN, 9, 'ls', KO)
    M.t(1243, 118, 'CH 1', TXT, 9, 'rs', KO)
    M.fill(circle(1146, 114, 0.1, 4), REDN, 0.0, layer=3)
    for p in suit('heart')(1145, 114, 3.8): M.ln(p, (255, 110, 150), 1.1, 1.0, glow=False)   # 목숨 (시안에 없음)
    M.t(1151, 118, '18', (255, 110, 150), 9, 'ls', BOLD)
    M.t(1153, 135, '- 제 3화 -', TXT, 9, 'ms', KO, 0.85)
    M.t(1153, 152, '병정들의 행진', (255, 240, 170), 13, 'ms', KO)
    for i, (kind, n) in enumerate((('normal', '병정 x 18'), ('fast', '빠른 병정 x 6'), ('shield', '방패 병정 x 1'))):
        yy = 168 + i * 16
        soldier(M, 1070, yy + 1, 'normal', 1.0, 0.5)
        M.t(1085, yy + 4, n, TXT, 9.5, 'ls', KO)
    M.ln([(1061, 306), (1246, 306)], (90, 100, 110), 1.0, 0.7, glow=False)
    M.t(1063, 320, '남은 적 6/24', TXT, 9.5, 'ls', KO)
    M.t(1244, 320, '처치 58 / 놓침 2', TXT, 9.5, 'rs', KO)
    for i, lab in enumerate(('x2', 'x4')):
        bx = 1053 + i * 43
        M.fill_rr(bx, 342, 35, 45, 4, (52, 58, 60), 1.0, layer=2)
        M.rr(bx, 342, 35, 45, 4, (185, 190, 195), 1.8, 0.95)
        M.t(bx + 17.5, 362, lab, TXT, 11, 'ms', KO)
        for k in range(2):
            ax = bx + 11 + k * 6
            M.fill([(ax, 369), (ax + 6, 373.5), (ax, 378)], TXT, 1.0, layer=3)
    M.fill(circle(1175, 365, 22, 40), (45, 12, 34), 1.0, layer=2)
    M.ln(circle(1175, 365, 22, 40), PINKN, 3.0, 1.0, core=True)
    M.ln(circle(1175, 365, 15, 32), PINKN, 1.2, 0.6)
    M.ln([(1175, 365), (1162, 354)], PINKN, 2.4, 1.0)
    for k in range(6):
        M.ln([(1218, 345 + k * 8), (1254, 345 + k * 8)], PURPLE, 1.6, 0.85)
    return (SX, SY, SW, SH)


def cocktail(M, cx, cy, on):
    c = (255, 90, 190) if on else (95, 45, 100)
    o = ORANGEN if on else (110, 60, 60)
    b = 1.0 if on else 0.75
    M.ln([(cx - 15, cy - 18), (cx + 15, cy - 18), (cx + 2, cy), (cx - 2, cy), (cx - 15, cy - 18)], c, 1.7, b, glow=on)
    M.ln([(cx, cy), (cx, cy + 16)], c, 1.5, b, glow=on)
    M.ln([(cx - 9, cy + 16), (cx + 9, cy + 16)], c, 1.7, b, glow=on)
    if on: M.fill([(cx - 12, cy - 15), (cx + 12, cy - 15), (cx + 2, cy - 2), (cx - 2, cy - 2)], (255, 120, 60), 0.55, layer=3)
    M.fill(circle(cx - 9, cy - 21, 6, 16), o, 0.8 if on else 0.4, layer=3)
    M.ln(circle(cx - 9, cy - 21, 6, 16), (255, 200, 90) if on else o, 1.2, b, glow=on)
    M.ln([(cx + 2, cy - 18), (cx + 9, cy - 30)], c, 1.2, b, glow=on)


def right_column(M):
    scr = tv(M)
    for yy, c in ((410, PINKN), (418, PURPLE)): M.ln([(1038, yy), (1270, yy)], c, 1.8, 0.9, core=True)
    grad_rr(M, 1038, 425, 232, 112, 4, (32, 12, 44), (20, 9, 34), layer=1)
    M.t(1153, 449, '칵테일', (255, 230, 190), 14, 'ms', KO)
    M.ln([(1053, 443), (1123, 443)], (150, 130, 160), 1.0, 0.7, glow=False)
    M.ln([(1187, 443), (1254, 443)], (150, 130, 160), 1.0, 0.7, glow=False)
    M.t(1254, 465, '3웨이브 후 획득', TXT, 8.5, 'rs', KO, 0.9)
    for i, x in enumerate((1067, 1110, 1154, 1197, 1240)):
        cocktail(M, x, 508, i < 2)
    M.ln([(1043, 534), (1265, 534)], BLUEN, 2.0, 1.0, core=True)
    for yy, c in ((552, PINKN), (561, BLUEN)): M.ln([(1038, yy), (1270, yy)], c, 1.8, 0.9, core=True)
    # 스킬
    x, y, w, h = 1038, 575, 232, 51
    grad_rr(M, x, y, w, h, 6, (10, 60, 160), (2, 40, 120), layer=2)
    M.rr(x, y, w, h, 6, (60, 135, 255), 2.6, 1.0, core=True)
    M.rr(x + 4, y + 4, w - 8, h - 8, 4, (60, 135, 255), 1.0, 0.45, glow=False)
    nodes = [(1056, 600), (1072, 590), (1072, 610), (1088, 600)]
    for a, b_ in ((0, 1), (0, 2), (1, 3), (2, 3)): M.ln([nodes[a], nodes[b_]], CYANN, 1.6, 1.0)
    for (nx, ny) in nodes:
        M.fill(circle(nx, ny, 3.4, 12), CYANN, 1.0, layer=3)
    M.t(1162, 609, '스 킬', (130, 205, 255), 24, 'ms', KO, glow=True)
    # 시작
    x, y, w, h = 1038, 636, 232, 73
    grad_rr(M, x, y, w, h, 8, (20, 90, 30), (8, 60, 18), layer=2)
    M.rr(x, y, w, h, 8, GREENN, 3.0, 1.0, core=True)
    M.rr(x + 5, y + 5, w - 10, h - 10, 5, GREENN, 1.0, 0.5, glow=False)
    M.fill([(1058, 655), (1090, 672), (1058, 689)], (130, 255, 140), 1.0, layer=3)
    M.ln([(1058, 655), (1090, 672), (1058, 689), (1058, 655)], GREENN, 1.6, 1.0)
    M.t(1165, 688, '시 작', (130, 240, 140), 38, 'ms', KO, glow=True)
    return scr


# ---------------------------------------------------------------- 아래: 덱 · 손패
def hand_card(M, x, y, idx, rank, sk, cw=59, ch=97):
    c, f = SUITR[sk], SUITF[sk]
    M.fill_rr(x, y, cw, ch, 7, f, 1.0, layer=2)
    M.rr(x, y, cw, ch, 7, c, 1.9, 0.75)
    M.rr(x + 2, y + 2, cw - 4, ch - 4, 5, lerp(c, WHITE, 0.3), 0.8, 0.6, glow=False)
    M.t(x + 7, y + 13, str(idx + 1), (190, 190, 200), 8, 'ls', KOR)
    suit_line(M, sk, x + cw / 2, y + 26, 5.5, c, 1.5)
    M.t(x + cw / 2, y + 66, rank, c, 33, 'ms', COND)
    M.t(x + cw / 2, y + ch - 10, ATK[sk], c, 8.5, 'ms', KO, 0.8)


def bottom(M):
    M.small_glow = 1.0
    M.t(318, 648, '10 / 10', (255, 210, 80), 40, 'ms', COND, glow=True)
    M.t(318, 676, '덱 38장', TXT, 15, 'ms', KO)
    for i, (r, sk) in enumerate(HAND):
        hand_card(M, 388 + i * 63, 596, i, r, sk)
    M.fill(circle(1251, 45, 14, 32), (55, 58, 66), 1.0, layer=2)
    M.ln(circle(1251, 45, 14, 32), (225, 225, 235), 2.0, 1.0)
    M.t(1251, 52, '?', WHITE, 18, 'ms', BOLD)


# ---------------------------------------------------------------- 판 위 폭발 · TV 지직
def burst(M, x, y):
    rnd = random.Random(3)
    M.fill(circle(x, y, 13, 24), (255, 220, 150), 0.55, layer=3)
    M.ln(circle(x, y, 9, 24), (255, 240, 200), 1.6, 1.0)
    for k in range(20):
        a = k / 20 * 2 * math.pi + rnd.uniform(-0.15, 0.15)
        r0, r1 = rnd.uniform(10, 16), rnd.uniform(24, 44)
        M.ln([(x + math.cos(a) * r0, y + math.sin(a) * r0), (x + math.cos(a) * r1, y + math.sin(a) * r1)], ORANGEN if k % 2 else (255, 225, 120), 1.4, 1.0)
    for _ in range(30):
        a, r = rnd.uniform(0, 2 * math.pi), rnd.uniform(14, 56)
        px, py = x + math.cos(a) * r, y + math.sin(a) * r
        M.ln([(px - 1.3, py), (px + 1.3, py)], (255, 200, 110), 2.6, 1.0)
    M.dot(x, y, 3.0)


def static(img, box):
    sx, sy, sw, sh = box
    d = ImageDraw.Draw(img, 'RGBA')
    rnd = random.Random(7)
    for yy in range(int(sy + 3), int(sy + sh - 3), 2):
        d.line([(sx + 3, yy), (sx + sw - 3, yy)], fill=(0, 0, 0, 40))
    for _ in range(900):
        x, y = rnd.uniform(sx + 4, sx + sw - 4), rnd.uniform(sy + 4, sy + sh - 4)
        g = rnd.randint(30, 80)
        d.point((x, y), fill=(g, g + 6, g + 6, 150))
    return img


def mock():
    M = Bar(gk=0.6)
    left_panel(M)
    wave_sign(M)
    G.board(M, FX, FY, CS, frame_c=TEAL, frame_b=0.9, path_style='dash', ghost=False)
    M.fill_rr(FX - 4, FY - 4, 16 * CS + 8, 10 * CS + 8, 10, (0, 0, 0), 0.75, layer=0)
    burst(M, 360, 166)
    scr = right_column(M)
    img = static(M.draw(), scr)
    img = wave_word(img)
    img = decal(img, '3/20', 657, 46, BOLD, 58, (6, 10, 24), (40, 140, 255), 4, (40, 130, 255))
    P = Bar(gk=0.6)
    bottom(P)
    return P.draw(img)


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    mock().save(os.path.join(outdir, 'alice_ui_mock.png'))
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
