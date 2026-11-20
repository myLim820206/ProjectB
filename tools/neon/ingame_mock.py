"""NEON PALLAS 게임 화면 시안 — 대문 시안 B(벽돌 벽 · 젖은 바닥 반사 · 한 색 튜브) 분위기로 판 화면 다시 꾸미기

    python tools/neon/ingame_mock.py 출력폴더

A 벽 위 바 테이블: 지금 배치(왼쪽 족보 · 가운데 판 · 오른쪽 위력/버튼/묘약 · 아래 손패) 그대로, 판은 빛나는 테이블, 패널은 흐린 튜브
B 걸린 간판: 위쪽 띠 대신 천장에 매단 간판(WAVE · 남은 적 전구 · 목숨 · 레벨), 족보는 바 메뉴판, 웨이브 버튼은 전구 마키, 묘약은 병 선반
C 판이 주인공: 판을 크게(칸 52), 족보는 접힌 튜브 줄(마우스를 올린 줄만 켜짐), 오른쪽은 좁은 열과 동그란 아이콘 버튼, 툴팁 · 판매 안내

버튼과 버튼 같은 패널은 네온 간판 테두리: 바깥 튜브 + 5px 안쪽의 가는 튜브(같은 색, 조금 흐리게) + 양 끝 회색 볼트 둘.
고르지 않은 것은 두 튜브 모두 흐리게, 고른 것은 둘 다 켜진다.
상황: 웨이브 12 진행 중, 판에 카드 타워 몇 개 · 조커 하나 · 적 몇, 손패 5장 중 하나(Q♥)를 골라 판에 놓을 자리를 보는 중.
"""
import math
import os
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

sys.path.insert(0, os.path.dirname(__file__))
from neon import suit, circle, joker_cards  # noqa: E402
from logo_strokes import draw_logo  # noqa: E402
from logo_retro import bulbs  # noqa: E402

W, H, S = 1280, 720, 2
BG = (6, 6, 11)
KO = 'C:/Windows/Fonts/malgunbd.ttf'
KOR = 'C:/Windows/Fonts/malgun.ttf'
BLOCK = 'C:/Windows/Fonts/bahnschrift.ttf'

CYAN, PINK, GOLD, ORANGE = (110, 220, 255), (255, 106, 170), (255, 210, 110), (255, 150, 70)
WHITE = (255, 255, 255)
FG, GREY, DIMT = (207, 216, 230), (138, 148, 166), (95, 104, 120)
BOLT = (138, 148, 166)
GREEN, RED, MINT = (156, 255, 176), (255, 106, 106), (106, 255, 200)
JPINK = (255, 154, 213)
BR = (60, 48, 58)
# D-260: 테두리는 하늘(무늬 ♠ 색과 겹침) 대신 연보라, 네온 선 밝기 · 빛번짐은 30% 죽인다 (정신없음)
FRAME = (168, 150, 225)
NEON_K = 0.7
GRID = (120, 140, 180)
SUITC = {'spade': (127, 184, 255), 'heart': (255, 106, 138), 'diamond': (255, 217, 106), 'club': (138, 255, 196)}
SYM = {'spade': '♠', 'heart': '♥', 'diamond': '◆', 'club': '♣'}
ATK = {'spade': '저격', 'heart': '폭발', 'diamond': '연사', 'club': '둔화'}
ENEMYC = {'normal': (232, 240, 255), 'fast': (255, 168, 106), 'tank': (196, 154, 255), 'shield': (159, 195, 232)}


def hx(h): return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


def dim(c, b): return tuple(max(0, min(255, int(v * b))) for v in c)


def rr_pts(x, y, w, h, r):
    r = max(1.0, min(r, w / 2, h / 2))
    P = []
    for (cx, cy, a0) in ((x + w - r, y + r, -90), (x + w - r, y + h - r, 0), (x + r, y + h - r, 90), (x + r, y + r, 180)):
        for k in range(7):
            a = math.radians(a0 + k * 15)
            P.append((cx + math.cos(a) * r, cy + math.sin(a) * r))
    P.append(P[0])
    return P


_FONTS = {}


def font(path, px):
    key = (path, int(px * S))
    if key not in _FONTS: _FONTS[key] = ImageFont.truetype(path, int(px * S))
    return _FONTS[key]


class Bar:
    """선(빛번짐) · 글자 · 옅은 채움 · 바닥 무늬를 모았다가 한 번에 그린다. b = 밝기 (0 ~ 1)"""
    def __init__(self, gk=0.75):
        self.lines, self.texts, self.dots, self.fills, self.bgl = [], [], [], [], []
        self.gk = gk
        self.small_glow = 0.0  # 26px 아래 글자 빛번짐 (사용자 규칙: 없음)

    def ln(self, pts, c, wd=2.0, b=1.0, glow=True, core=False): self.lines.append((pts, c, wd, b, glow, core))
    def rr(self, x, y, w, h, r, c, wd=2.0, b=1.0, glow=True, core=False): self.ln(rr_pts(x, y, w, h, r), c, wd, b, glow, core)
    def fill(self, pts, c, a, layer=1): self.fills.append((layer, 'poly', pts, c, a, 0))
    def fill_rr(self, x, y, w, h, r, c, a, layer=1): self.fill(rr_pts(x, y, w, h, r), c, a, layer)
    def fline(self, pts, c, a, wd=1.0, layer=1): self.fills.append((layer, 'line', pts, c, a, wd))
    def bg(self, pts, c, a, wd=1.0): self.bgl.append((pts, c, a, wd))
    def dot(self, x, y, r): self.dots.append((x, y, r))

    def width(self, s, px, f=KOR): return font(f, px).getlength(s) / S

    def t(self, x, y, s, c, px, anchor='ls', f=KOR, b=1.0, glow=False, hollow=False, maxw=None):
        if maxw:
            while px > 7 and self.width(s, px, f) > maxw: px -= 0.5
        self.texts.append((x, y, s, c, px, anchor, f, b, glow, hollow))
        return self.width(s, px, f)

    def draw(self, base=None):
        size = (W * S, H * S)
        img = Image.new('RGB', size, BG) if base is None else base.resize(size, Image.BICUBIC)
        bgl = Image.new('RGB', size, (0, 0, 0))
        d = ImageDraw.Draw(bgl)
        for (pts, c, a, wd) in self.bgl:
            d.line([(px * S, py * S) for px, py in pts], fill=dim(c, a), width=max(1, int(wd * S)))
        img = ImageChops.add(img, bgl)
        img = img.convert('RGBA')
        for layer in sorted(set(f[0] for f in self.fills)):
            ov = Image.new('RGBA', size, (0, 0, 0, 0))
            od = ImageDraw.Draw(ov)
            for (lay, kind, pts, c, a, wd) in self.fills:
                if lay != layer: continue
                P = [(px * S, py * S) for px, py in pts]
                if kind == 'poly': od.polygon(P, fill=c + (int(255 * a),))
                else: od.line(P, fill=c + (int(255 * a),), width=max(1, int(wd * S)))
            img = Image.alpha_composite(img, ov)
        img = img.convert('RGB')
        glow = Image.new('RGB', size, (0, 0, 0))
        core = Image.new('RGB', size, (0, 0, 0))
        gd, cd = ImageDraw.Draw(glow), ImageDraw.Draw(core)
        for (pts, c, wd, b, g, wc) in self.lines:
            P = [(px * S, py * S) for px, py in pts]
            b = b * NEON_K if b > 0.25 else b
            if g and b > 0.25 * NEON_K: gd.line(P, fill=dim(c, b), width=max(1, int(wd * S * 4)))
            cd.line(P, fill=dim(c, max(0.16, b)), width=max(1, int(wd * S)), joint='curve')
        for (pts, c, wd, b, g, wc) in self.lines:  # 켜진 튜브 가운데 흰 심
            if wc and b > 0.5:
                bb = b * NEON_K
                cd.line([(px * S, py * S) for px, py in pts], fill=tuple(min(255, int((v * 0.45 + 255 * 0.55 * b) * bb)) for v in c), width=max(1, int(wd * S * 0.4)), joint='curve')
        for (x, y, s, c, px, anchor, fp, b, g, hollow) in self.texts:
            f = font(fp, px)
            tw = max(2, int(px * S * 0.055))
            if g and b > 0.25 and (px >= 26 or self.small_glow > 0):  # 작은 글자는 빛번짐 없이 (읽기 쉽게)
                gd.text((x * S, y * S), s, font=f, fill=dim(c, b), anchor=anchor, stroke_width=tw * 2, stroke_fill=dim(c, b))
            if hollow:
                cd.text((x * S, y * S), s, font=f, fill=dim(c, b), anchor=anchor, stroke_width=tw, stroke_fill=dim(c, b))
                cd.text((x * S, y * S), s, font=f, fill=(0, 0, 0), anchor=anchor)
                if b > 0.6:
                    cd.text((x * S, y * S), s, font=f, fill=None, anchor=anchor, stroke_width=max(1, tw // 3), stroke_fill=dim(WHITE, 0.35 * b))
                    cd.text((x * S, y * S), s, font=f, fill=(0, 0, 0), anchor=anchor)
            else:
                cd.text((x * S, y * S), s, font=f, fill=dim(c, b), anchor=anchor)
        for (x, y, r) in self.dots:
            gd.ellipse([(x - r * 3) * S, (y - r * 3) * S, (x + r * 3) * S, (y + r * 3) * S], fill=(255, 220, 150))
            cd.ellipse([(x - r) * S, (y - r) * S, (x + r) * S, (y + r) * S], fill=WHITE)
        gk = self.gk * NEON_K
        img = ImageChops.add(img, glow.filter(ImageFilter.GaussianBlur(10 * S)).point(lambda v: int(v * 0.5 * gk)))
        img = ImageChops.add(img, glow.filter(ImageFilter.GaussianBlur(3 * S)).point(lambda v: int(v * 0.45 * gk)))
        img = ImageChops.add(img, core)
        return img.resize((W, H), Image.LANCZOS)


def reflect(img, box, floor_y, k=0.16):
    """box 영역을 floor_y 아래로 뒤집어 흐리게 더한다 (젖은 바닥 반사)"""
    x0, y0, x1, y1 = box
    part = img.crop(box).transpose(Image.FLIP_TOP_BOTTOM).point(lambda v: int(v * k))
    h = min(part.height, img.height - floor_y)
    if h <= 0: return img
    out = img.copy()
    region = out.crop((x0, floor_y, x1, floor_y + h))
    # 아래로 갈수록 옅게
    fade = Image.linear_gradient('L').resize((x1 - x0, h)).point(lambda v: 255 - v)
    add = Image.composite(part.crop((0, 0, x1 - x0, h)), Image.new('RGB', (x1 - x0, h)), fade)
    out.paste(ImageChops.add(region, add), (x0, floor_y))
    return out


# ================================================================ 공통 부품
def tube(M, x, y, w, h, r, c, on=False, b=None, wd=None, fill=True):
    """네온 간판 테두리 (사용자 요청, title-front/ingame_B_buttons.png): 바깥 튜브 + 5px 안쪽 튜브, 같은 색, 둘 다 켜져
    빛번짐 + 흰 심. 안 고른 것도 0.8 밝기로 켜져 있고, 고른 것(on)은 더 밝고 굵다. 양 끝 안쪽에 작은 회색 볼트"""
    if c == CYAN: c = FRAME  # 테두리 색 (D-260)
    bo = b if b is not None else (1.0 if on else 0.8)
    bi = bo * 0.85
    if fill:
        M.fill_rr(x, y, w, h, r, BG, 0.85, layer=2)
        M.fill_rr(x, y, w, h, r, c, 0.10 if on else 0.04, layer=3)
    M.rr(x, y, w, h, r, c, wd or (2.4 if on else 2.0), bo, core=True)
    M.rr(x + 5, y + 5, w - 10, h - 10, max(2.0, r - 5), c, 1.5 if on else 1.3, bi, core=True)
    inset = max(8.0, min(14.0, w * 0.05)) if w > 44 else w / 2
    if w > 44:
        for bx in (x + inset, x + w - inset):
            M.ln(circle(bx, y + h / 2, 1.6, 10), BOLT, 0.9, 0.6, glow=False)


def bricks(M, y0, y1, x0=0, x1=W, a=0.11, holes=()):
    """흐린 벽돌 벽. holes = 비울 사각형 (판 뒤 등)"""
    def blocked(px, py):
        return any(hx0 <= px <= hx1 and hy0 <= py <= hy1 for (hx0, hy0, hx1, hy1) in holes)
    r = 0
    y = y0
    while y < y1:
        yb = min(y1, y + 34)
        # 가로줄: 구멍 밖 구간만
        segs, cur = [], None
        for xx in range(int(x0), int(x1) + 1, 8):
            if not blocked(xx, y):
                cur = cur or [xx, xx]
                cur[1] = xx
            elif cur: segs.append(cur); cur = None
        if cur: segs.append(cur)
        for s0, s1 in segs: M.bg([(s0, y), (s1, y)], BR, a * 2.4)
        for c in range(-1, int((x1 - x0) / 64) + 2):
            x = x0 + c * 64 + (32 if r % 2 else 0)
            if x0 <= x <= x1 and not blocked(x, (y + yb) / 2):
                M.bg([(x, y), (x, yb)], BR, a * 2.4)
        y += 34
        r += 1


def floor_line(M, y, x0=0, x1=W):
    M.ln([(x0, y), (x1, y)], (120, 110, 140), 1.4, 0.5)


def heart_cursor(M, x, y, r=7):
    for p in suit('heart')(x, y, r): M.ln(p, PINK, 1.4)


def suit_shape(M, k, x, y, r, c=None, wd=1.4, b=1.0):
    for p in suit(k)(x, y, r): M.ln(p, c or SUITC[k], wd, b)


# ---------------------------------------------------------------- 판 상태
PATH_WP = [(0, 1), (4, 1), (4, 7), (9, 7), (9, 2), (13, 2), (13, 8), (15, 8)]
TOWERS = {
    (5, 3): ('7', 'diamond', 1), (6, 3): ('8', 'spade', 3), (7, 3): ('9', 'heart', 2), (8, 3): ('10', 'club', 2),
    (6, 4): ('K', 'spade', 1), (7, 4): ('K', 'heart', 1),
    (10, 4): ('2', 'heart', 1), (11, 4): ('5', 'heart', 1), (12, 4): ('J', 'heart', 2),
    (2, 3): ('A', 'club', 1), (2, 5): ('4', 'club', 1), (14, 5): ('3', 'diamond', 1), (6, 8): ('Q', 'spade', 1),
}
JOKER = (11, 5)
FLOORS = [  # 족보 장판 (칸, 색)
    ([(5, 3), (6, 3), (7, 3), (8, 3)], hx('6a8cff')),   # 스트레이트
    ([(6, 4), (7, 4)], hx('cfd8e6')),                   # 원 페어
    ([(10, 4), (11, 4), (12, 4)], hx('b48cff')),        # 플러시
]
ENEMIES = [(0.06, 'normal', 1.0), (0.13, 'normal', 0.9), (0.27, 'fast', 0.6), (0.40, 'tank', 0.75), (0.52, 'normal', 0.45),
           (0.61, 'shield', 0.8), (0.74, 'fast', 0.3), (0.83, 'normal', 0.55)]
GHOST = (10, 5)  # 고른 Q♥ 를 놓아 보는 칸

HANDS = [  # 이름, 효과, 색, 개수, 합성 가능
    ('로열 스트레이트 플러시', '공격력 ×2.5 · 연사 ×2.5 · 넓은 범위', 'fff4d6', 0, 0),
    ('파이브 카드', '공격력 ×5 (와일드·조커가 있어야)', 'ffd96a', 0, 0),
    ('스트레이트 플러시', '공격력 ×2 · 연사 ×2 · 범위', 'ff9ad5', 0, 0),
    ('포카드', '공격력 ×3.5', 'ff6a6a', 0, 0),
    ('풀하우스', '공격력 ×1.5 · 연사 ×1.5 · 범위', 'ffa86a', 0, 0),
    ('플러시 (3~5장)', '범위 공격 (장수만큼 넓게)', 'b48cff', 1, 0),
    ('스트레이트 (2~5장)', '연사 ×1.25 / ×1.5 / ×1.75 / ×2', '6a8cff', 1, 1),
    ('트리플', '공격력 ×2', '9cffb0', 0, 0),
    ('투 페어', '공격력 ×1.5 · 사거리 ×1.25', '7fd6ff', 0, 0),
    ('원 페어', '공격력 ×1.5', 'cfd8e6', 1, 0),
]
PREVIEW_ROW = 5  # Q♥ 를 놓으면 플러시가 늘어남 → 금색 미리보기
HAND = [('K', 'club'), ('Q', 'heart'), ('6', 'diamond'), ('3', 'club'), ('A', 'heart')]
SEL = 1
POWER = [('할리퀸 Lv3', JPINK, 0.32, '1.1k'), ('8♠ Lv3', SUITC['spade'], 0.18, '620'), ('J♥ Lv2', SUITC['heart'], 0.14, '480'),
         ('9♥ Lv2', SUITC['heart'], 0.12, '410'), ('10♣ Lv2', SUITC['club'], 0.10, '330'), ('K♠ Lv1', SUITC['spade'], 0.08, '260'),
         ('7◆ Lv1', SUITC['diamond'], 0.06, '210')]
CHARMS = [('하트 애호가', '♥ 타워 공격력 +40%'), ('연결고리', '연쇄 보너스 +25% → +45%')]


def path_cells():
    cells = []
    for (a, b) in zip(PATH_WP, PATH_WP[1:]):
        x, y = a
        dx, dy = (b[0] > a[0]) - (b[0] < a[0]), (b[1] > a[1]) - (b[1] < a[1])
        while (x, y) != b:
            if (x, y) not in cells: cells.append((x, y))
            x += dx; y += dy
    if PATH_WP[-1] not in cells: cells.append(PATH_WP[-1])
    return cells


def outline_segs(cells, FX, FY, CS, ins=0.0):
    """칸 묶음의 바깥 변만 (ins 만큼 안쪽으로)"""
    st = set(cells)
    out = []
    for (cx, cy) in cells:
        x, y = FX + cx * CS, FY + cy * CS
        L, R, U, D = (cx - 1, cy) not in st, (cx + 1, cy) not in st, (cx, cy - 1) not in st, (cx, cy + 1) not in st
        if U: out.append([(x + (ins if L else 0), y + ins), (x + CS - (ins if R else 0), y + ins)])
        if D: out.append([(x + (ins if L else 0), y + CS - ins), (x + CS - (ins if R else 0), y + CS - ins)])
        if L: out.append([(x + ins, y + (ins if U else 0)), (x + ins, y + CS - (ins if D else 0))])
        if R: out.append([(x + CS - ins, y + (ins if U else 0)), (x + CS - ins, y + CS - (ins if D else 0))])
    return out


def hatch(M, x0, y0, s, c, a, step=8):
    for t in range(1, int(2 * s)):
        if (int(x0 + y0) + t) % step: continue
        if t <= s: p, q = (x0 + t, y0), (x0, y0 + t)
        else: p, q = (x0 + s, y0 + t - s), (x0 + t - s, y0 + s)
        M.fline([p, q], c, a, 1.0)


def tower(M, cx, cy, CS, rank, sk, lvl, b=1.0, col=None):
    h = CS * 0.4
    col = col or SUITC[sk]
    M.fill_rr(cx - h, cy - h, 2 * h, 2 * h, h * 0.22, BG, 0.8, layer=2)
    M.rr(cx - h, cy - h, 2 * h, 2 * h, h * 0.22, col, 1.5, b)
    M.t(cx, cy + CS * 0.06, rank, col, CS * 0.34, 'ms', BLOCK, b)
    suit_shape(M, sk, cx, cy + h * 0.55, CS * 0.085, col, 1.1, b)
    for k in range(lvl - 1):  # 레벨 점
        M.dots.append((cx + h - 5, cy - h + 5 + k * 5, 0.0))
        M.ln(circle(cx + h - 5, cy - h + 5 + k * 5, 1.2, 8), col, 0.9, b)


def joker_tower(M, cx, cy, CS, b=1.0):
    h = CS * 0.4
    M.fill_rr(cx - h, cy - h, 2 * h, 2 * h, h * 0.22, BG, 0.8, layer=2)
    for p in joker_cards()[4][0](cx, cy, h / 0.98):
        if isinstance(p, tuple): continue
        M.ln(p, JPINK, 1.4, b)
    M.ln(circle(cx + h - 5, cy - h + 5, 1.2, 8), JPINK, 0.9, b); M.ln(circle(cx + h - 5, cy - h + 10, 1.2, 8), JPINK, 0.9, b)


def soldier(M, x, y, kind, hp, s=1.0):
    c = ENEMYC[kind]
    sc = s * (1.25 if kind == 'tank' else 1.0)
    M.rr(x - 6 * sc, y - 7 * sc, 12 * sc, 15 * sc, 2.5 * sc, c, 1.3, 0.95)
    M.ln(circle(x, y - 11 * sc, 3.2 * sc, 14), c, 1.2, 0.95)
    M.ln([(x - 3 * sc, y + 8 * sc), (x - 4.5 * sc, y + 13 * sc)], c, 1.2, 0.9)
    M.ln([(x + 3 * sc, y + 8 * sc), (x + 4.5 * sc, y + 13 * sc)], c, 1.2, 0.9)
    M.ln([(x - 6 * sc, y - 3 * sc), (x - 10 * sc, y + 2 * sc)], c, 1.1, 0.9)
    M.ln([(x + 6 * sc, y - 3 * sc), (x + 10 * sc, y + 2 * sc)], c, 1.1, 0.9)
    if kind == 'shield': M.ln([(x + 11 * sc, y - 6 * sc), (x + 11 * sc, y + 6 * sc)], c, 2.0, 0.95)
    suit_shape(M, 'diamond' if kind == 'fast' else 'spade', x, y, 2.6 * sc, c, 0.9, 0.9)
    bw, by = 20 * s, y - 19 * sc
    M.ln([(x - bw / 2, by), (x + bw / 2, by)], (60, 64, 80), 2.0, 0.6, glow=False)
    M.ln([(x - bw / 2, by), (x - bw / 2 + bw * hp, by)], GREEN if hp > 0.5 else ORANGE if hp > 0.25 else RED, 2.0, 0.9, glow=False)


def board(M, FX, FY, CS, frame_c=FRAME, frame_b=0.55, show_speed=False, path_style='chev', enemies=True, ghost=True):
    """path_style: 'chev' = 옅은 채움 + 벽 + 꺾쇠, 'dash' = 어두운 띠 + 가운데 점선 + 꺾이는 곳 점 (참고 화면, ingame_ref_mock)"""
    BW, BH = 16 * CS, 10 * CS
    cen = lambda c: (FX + (c[0] + 0.5) * CS, FY + (c[1] + 0.5) * CS)
    M.fill_rr(FX - 8, FY - 8, BW + 16, BH + 16, 14, BG, 0.92, layer=0)
    # 테이블 테두리 (두 겹 튜브, 볼트 넷)
    M.rr(FX - 8, FY - 8, BW + 16, BH + 16, 14, frame_c, 1.8, frame_b)
    M.rr(FX - 3, FY - 3, BW + 6, BH + 6, 10, frame_c, 0.9, frame_b * 0.55)
    for (bx, by) in ((FX - 5.5, FY + BH / 2), (FX + BW + 5.5, FY + BH / 2), (FX + BW / 2, FY - 5.5), (FX + BW / 2, FY + BH + 5.5)):
        M.ln(circle(bx, by, 1.6, 10), BOLT, 0.8, 0.55, glow=False)
    for i in range(1, 16): M.bg([(FX + i * CS, FY), (FX + i * CS, FY + BH)], GRID, 0.07)
    for j in range(1, 10): M.bg([(FX, FY + j * CS), (FX + BW, FY + j * CS)], GRID, 0.07)
    # 적 길: 옅은 채움 + 벽 + 방향 꺾쇠
    pc = path_cells()
    for (x, y) in pc: M.fill([(FX + x * CS, FY + y * CS), (FX + (x + 1) * CS, FY + y * CS), (FX + (x + 1) * CS, FY + (y + 1) * CS), (FX + x * CS, FY + (y + 1) * CS)], CYAN, 0.045 if path_style == 'chev' else 0.03, layer=1)
    if path_style == 'chev':
        for sg in outline_segs(pc, FX, FY, CS, 3): M.ln(sg, FRAME, 1.0, 0.32)
    poly = [(FX + (x + 0.5) * CS, FY + (y + 0.5) * CS) for x, y in PATH_WP]
    poly[0] = (FX - 2, poly[0][1]); poly[-1] = (FX + BW + 2, poly[-1][1])
    Ls = [0.0]
    for i in range(1, len(poly)): Ls.append(Ls[-1] + math.dist(poly[i - 1], poly[i]))

    def at(t):
        d = t * Ls[-1]
        for i in range(1, len(poly)):
            if Ls[i] >= d:
                u = (d - Ls[i - 1]) / max(1e-6, Ls[i] - Ls[i - 1])
                p, q = poly[i - 1], poly[i]
                return (p[0] + (q[0] - p[0]) * u, p[1] + (q[1] - p[1]) * u), (q[0] - p[0], q[1] - p[1])
        return poly[-1], (1, 0)
    if path_style == 'dash':
        for i in range(1, len(poly)):
            p, q = poly[i - 1], poly[i]
            L = math.dist(p, q); n = int(L // 9)
            for j in range(n):
                if j % 2: continue
                u0, u1 = j / n, min(1.0, (j + 1) / n)
                M.ln([(p[0] + (q[0] - p[0]) * u0, p[1] + (q[1] - p[1]) * u0), (p[0] + (q[0] - p[0]) * u1, p[1] + (q[1] - p[1]) * u1)], CYAN, 1.0, 0.4, glow=False)
        for (x, y) in poly[1:-1]: M.ln(circle(x, y, 2.4, 10), CYAN, 1.4, 0.75)
    k = 0.02 if path_style == 'chev' else 9.0
    while k < 1.0:
        (x, y), (dx, dy) = at(k)
        n = math.hypot(dx, dy) or 1
        ux, uy = dx / n, dy / n
        s = CS * 0.09
        M.ln([(x - ux * s - uy * s, y - uy * s + ux * s), (x, y), (x - ux * s + uy * s, y - uy * s - ux * s)], CYAN, 1.0, 0.22, glow=False)
        k += 1.4 * CS / Ls[-1]
    # 입구 · 출구 표시
    for (x, y), lab in ((poly[0], '입구'), (poly[-1], '출구')):
        M.t(x + (14 if lab == '입구' else -14), y - CS * 0.36, lab, CYAN, 9, 'ls' if lab == '입구' else 'rs', KOR, 0.55)
    # 족보 장판
    for cells, c in FLOORS:
        for (x, y) in cells:
            M.fill([(FX + x * CS, FY + y * CS), (FX + (x + 1) * CS, FY + y * CS), (FX + (x + 1) * CS, FY + (y + 1) * CS), (FX + x * CS, FY + (y + 1) * CS)], c, 0.11, layer=1)
            hatch(M, FX + x * CS, FY + y * CS, CS, c, 0.22)
        for sg in outline_segs(cells, FX, FY, CS, 2.5): M.ln(sg, c, 1.4, 0.8)
    # 연쇄 번개 (스트레이트 ↔ 원 페어)
    zx, zy = FX + 6.5 * CS, FY + 4 * CS
    M.ln([(zx - CS * 0.5, zy), (zx - CS * 0.3, zy - 4), (zx - CS * 0.1, zy + 4), (zx + CS * 0.1, zy - 4), (zx + CS * 0.3, zy + 4), (zx + CS * 0.5, zy)], (255, 240, 200), 1.1, 0.85)
    # 사거리 고리 + 놓을 자리 미리보기 (고른 카드 = 분홍)
    gx, gy = cen(GHOST)
    if ghost: M.ln(circle(gx, gy, CS * 2.6, 72), PINK, 1.0, 0.3, glow=False)
    h = CS * 0.4
    if ghost:
        M.fill(circle(gx, gy, CS * 2.6, 72), PINK, 0.025, layer=1)
        pts = rr_pts(gx - h, gy - h, 2 * h, 2 * h, h * 0.22)
        for i in range(0, len(pts) - 1, 2): M.ln([pts[i], pts[i + 1]], PINK, 1.5, 0.9)
        M.t(gx, gy + CS * 0.06, 'Q', PINK, CS * 0.34, 'ms', BLOCK, 0.7)
        suit_shape(M, 'heart', gx, gy + h * 0.55, CS * 0.085, PINK, 1.1, 0.7)
    # 타워
    for (c, (r, sk, lv)) in TOWERS.items():
        x, y = cen(c)
        tower(M, x, y, CS, r, sk, lv)
    if JOKER: joker_tower(M, *cen(JOKER), CS)
    # 적
    pos = []
    if not enemies: return cen
    for (t, kind, hp) in ENEMIES:
        (x, y), _ = at(t)
        soldier(M, x, y, kind, hp, CS / 48)
        pos.append((x, y))

    def nearest(c):
        tx, ty = cen(c)
        return min(pos, key=lambda p: math.dist(p, (tx, ty)))
    # 발사체: ♠ 레이저, ♦ 탄환, 조커 분홍 레이저
    sx, sy = cen((6, 3)); ex, ey = nearest((6, 3))
    M.ln([(sx, sy - CS * 0.3), (ex, ey)], SUITC['spade'], 1.6, 1.0); M.ln([(sx, sy - CS * 0.3), (ex, ey)], WHITE, 0.6, 0.8, glow=False)
    M.dot(ex, ey, 1.6)
    sx, sy = cen((5, 3)); ex, ey = nearest((5, 3))
    for u in (0.35, 0.55, 0.75):
        px, py = sx + (ex - sx) * u, sy + (ey - sy) * u
        dx, dy = (ex - sx), (ey - sy); n = math.hypot(dx, dy) or 1
        M.ln([(px - dx / n * 4, py - dy / n * 4), (px + dx / n * 4, py + dy / n * 4)], SUITC['diamond'], 1.6, 1.0)
    if JOKER:
        sx, sy = cen(JOKER); ex, ey = nearest(JOKER)
        M.ln([(sx, sy), (ex, ey)], JPINK, 1.3, 0.9)
    if show_speed:  # 배속 표시 (판 오른쪽 위)
        rx, yb = FX + BW - 16, FY + 34
        w = M.t(rx, yb, '×2 배속', CYAN, 24, 'rs', KO, 0.55, glow=True)
        ax = rx - w - 26
        for kk in range(2): M.ln([(ax + kk * 10, yb - 17), (ax + kk * 10 + 9, yb - 9), (ax + kk * 10, yb - 1)], CYAN, 1.8, 0.55)
        M.t(rx, yb + 16, 'F: 배속 바꾸기', CYAN, 9, 'rs', KOR, 0.4)
    return cen


def hand_card(M, x, y, rank, sk, idx, sel, cw=70, ch=98):
    col = PINK if sel else SUITC[sk]
    M.fill_rr(x, y, cw, ch, 10, BG, 0.9, layer=2)
    if sel: M.fill_rr(x, y, cw, ch, 10, PINK, 0.08, layer=3)
    M.rr(x, y, cw, ch, 10, col, 2.2 if sel else 1.3, 1.0 if sel else 0.5)
    M.rr(x + 4, y + 4, cw - 8, ch - 8, 7, col, 0.8, 0.55 if sel else 0.2)
    M.t(x + cw / 2, y + ch * 0.47, rank, WHITE if sel else col, 32, 'ms', BLOCK, 1.0 if sel else 0.8, glow=sel)
    suit_shape(M, sk, x + cw / 2, y + ch * 0.63, 8, col, 1.5, 1.0 if sel else 0.7)
    M.t(x + cw / 2, y + ch - 10, ATK[sk], WHITE if sel else col, 10, 'ms', KO if sel else KOR, 1.0 if sel else 0.75)
    M.t(x + 9, y + 17, str(idx + 1), DIMT, 10, 'ls', KOR)
    if sel: heart_cursor(M, x + cw / 2, y - 14, 7)


def bolt(M, x, y): M.ln(circle(x, y, 1.6, 10), BOLT, 0.8, 0.55, glow=False)


def wires(M, x0, x1, y):
    for x in (x0, x1):
        M.ln([(x, 0), (x, y)], (100, 108, 125), 0.8, 0.3, glow=False)
        bolt(M, x, y)


# ---------------------------------------------------------------- 패널 내용 (A · B 공통으로 쓰는 것)
def power_block(M, x, y, w, rows=7, rowh=20, big=28):
    M.t(x + 14, y + 22, '판 위력', CYAN, 13, 'ls', KO)
    M.t(x + w - 14, y + 22, '초당 피해 어림', DIMT, 9, 'rs')
    M.t(x + 14, y + 58, '3,412', WHITE, big, 'ls', BLOCK, glow=True)
    M.t(x + 14, y + 76, '+12%', GREEN, 10, 'ls', KO)
    M.t(x + w - 14, y + 44, '족보 덤 ×1.8', FG, 10, 'rs')
    M.t(x + w - 14, y + 60, '족보 3 · 조커 1', GREY, 10, 'rs')
    for i, (n, c, sh, v) in enumerate(POWER[:rows]):
        yy = y + 96 + i * rowh
        M.t(x + 14, yy + 4, n, c, 10, 'ls', KO, maxw=82)
        bx = x + 100
        M.fill([(bx, yy - 4), (bx + 44 * sh / 0.32 * 0.9, yy - 4), (bx + 44 * sh / 0.32 * 0.9, yy + 4), (bx, yy + 4)], c, 0.35, layer=3)
        M.ln([(bx, yy - 4), (bx + 44, yy - 4), (bx + 44, yy + 4), (bx, yy + 4), (bx, yy - 4)], c, 0.8, 0.4, glow=False)
        M.t(x + w - 14, yy + 4, '%s · %d%%' % (v, round(sh * 100)), FG, 10, 'rs')


def hand_rows(M, x, y, w, rowh, effect=True, leader=False):
    for i, (n, e, hc, cnt, fuse) in enumerate(HANDS):
        yy = y + i * rowh
        c = hx(hc)
        pv = i == PREVIEW_ROW
        if pv: M.fill_rr(x + 8, yy + 2, w - 16, rowh - 4, 6, GOLD, 0.1, layer=3)
        elif cnt: M.fill_rr(x + 8, yy + 2, w - 16, rowh - 4, 6, c, 0.07, layer=3)
        if fuse: M.rr(x + 8, yy + 2, w - 16, rowh - 4, 6, GOLD, 1.2, 0.75)
        nc = GOLD if pv else c if cnt else dim(GREY, 0.95)
        nw = M.t(x + 18, yy + 16, n, nc, 12, 'ls', KO, maxw=w - 80)
        if effect: M.t(x + 18, yy + 30, e, GOLD if pv else DIMT, 9, 'ls', KOR, maxw=w - 36)
        if fuse:
            M.rr(x + w - 62, yy + 6, 48, 16, 8, GOLD, 1.1, 0.7)
            M.t(x + w - 38, yy + 18, '합성 ×%d' % fuse, GOLD, 9, 'ms', KO)
        elif cnt:
            M.rr(x + w - 42, yy + 6, 28, 16, 8, nc, 1.1, 0.6)
            M.t(x + w - 28, yy + 18, '×%d' % cnt, nc, 10, 'ms', KO)
        elif leader:
            M.t(x + w - 18, yy + 16, '·', DIMT, 10, 'rs')
        if leader:  # 메뉴판 점선
            for dx in range(int(x + 24 + nw), int(x + w - (70 if (fuse or cnt) else 26)), 6):
                M.ln([(dx, yy + 14), (dx + 1.5, yy + 14)], DIMT, 1.0, 0.4, glow=False)
    yy = y + 10 * rowh
    M.fill_rr(x + 8, yy + 2, w - 16, rowh - 4, 6, JPINK, 0.08, layer=3)
    M.t(x + 18, yy + 16, '조커', JPINK, 12, 'ls', KO)
    if effect: M.t(x + 18, yy + 30, '클릭: 만들 수 있는 조커 목록', DIMT, 9, 'ls')
    M.rr(x + w - 42, yy + 6, 28, 16, 8, JPINK, 1.1, 0.6); M.t(x + w - 28, yy + 18, '×1', JPINK, 10, 'ms', KO)
    yy += rowh
    M.fill_rr(x + 8, yy + 2, w - 16, rowh - 4, 6, JPINK, 0.1, layer=3)
    M.t(x + 18, yy + 16, '먹이기', JPINK, 12, 'ls', KO)
    if effect: M.t(x + 18, yy + 30, '클릭: 조커에 족보 먹이기', DIMT, 9, 'ls')
    M.rr(x + w - 62, yy + 6, 48, 16, 8, JPINK, 1.1, 0.6); M.t(x + w - 38, yy + 18, '먹이 ×1', JPINK, 9, 'ms', KO)


def suits_legend(M, x, y, cols=2, dx=108, dy=22):
    M.t(x, y, '무늬 = 공격 방식', GREY, 10)
    for i, k in enumerate(('spade', 'heart', 'diamond', 'club')):
        xx, yy = x + (i % cols) * dx, y + 20 + (i // cols) * dy
        suit_shape(M, k, xx + 6, yy, 6, None, 1.3, 0.7)
        M.t(xx + 18, yy + 4, ATK[k], SUITC[k], 11)


def hud_buttons(M, x0, y, s=30, gap=8):
    """일시정지 · 배속 · 설정 (모두 네온 간판 테두리, 배속 2배는 켜짐)"""
    xs = [x0, x0 + s + gap, x0 + 2 * (s + gap)]
    tube(M, xs[0], y, s, s, 8, CYAN)
    mx, my = xs[0] + s / 2, y + s / 2
    M.ln([(mx - 3, my - 4), (mx - 3, my + 4)], WHITE, 1.6, 0.9); M.ln([(mx + 3, my - 4), (mx + 3, my + 4)], WHITE, 1.6, 0.9)
    tube(M, xs[1], y, s, s, 8, CYAN, b=1.0)
    M.t(xs[1] + s / 2, y + s / 2 + 4, '×2', WHITE, 11, 'ms', BLOCK, glow=True)
    tube(M, xs[2], y, s, s, 8, CYAN)
    gx, gy = xs[2] + s / 2, y + s / 2
    M.ln(circle(gx, gy, 2.0, 14), WHITE, 1.0, 0.9)
    for k in range(8):
        a = k * math.pi / 4
        M.ln([(gx + math.cos(a) * 3.6, gy + math.sin(a) * 3.6), (gx + math.cos(a) * 5.4, gy + math.sin(a) * 5.4)], WHITE, 1.2, 0.9)


def level_pill(M, x, y, w=170, h=34):
    """대문 칩 알약 자리 → 레벨 · 경험치 (게임 안 재화는 경험치, D-228)"""
    tube(M, x, y, w, h, h / 2, GOLD, b=0.85)
    suit_shape(M, 'diamond', x + 20, y + h / 2, 6, GOLD, 1.2, 0.9)
    M.t(x + 32, y + h / 2 + 2, '레벨 7', GOLD, 14, 'ls', KO, glow=True)
    bx = x + 90
    M.fill([(bx, y + h / 2 - 2), (bx + 52, y + h / 2 - 2), (bx + 52, y + h / 2 + 2), (bx, y + h / 2 + 2)], CYAN, 0.12, layer=3)
    M.fill([(bx, y + h / 2 - 2), (bx + 32, y + h / 2 - 2), (bx + 32, y + h / 2 + 2), (bx, y + h / 2 + 2)], CYAN, 0.7, layer=3)
    M.t(bx + 58, y + h / 2 + 4, '62%', FG, 10, 'ls', KO)


def charm_bottle(M, cx, cy, c, on):
    b = 0.95 if on else 0.22
    M.ln([(cx - 3, cy - 14), (cx - 3, cy - 8), (cx - 8, cy - 2), (cx - 8, cy + 10), (cx + 8, cy + 10), (cx + 8, cy - 2), (cx + 3, cy - 8), (cx + 3, cy - 14)], c, 1.3, b)
    M.ln([(cx - 4.5, cy - 15), (cx + 4.5, cy - 15)], c, 1.6, b)
    if on: M.ln([(cx - 7, cy + 3), (cx - 3, cy + 1), (cx + 2, cy + 4), (cx + 7, cy + 2)], c, 1.0, 0.7)


# ================================================================ A 벽 위 바 테이블 (지금 배치 그대로)
def mock_A(left=None):
    """left = 왼쪽 족보 패널을 그리는 함수 (족보 메뉴 시안). 없으면 지금 모양"""
    M = Bar()
    FX, FY, CS = 262, 74, 48
    FLOOR = 670
    bricks(M, 0, FLOOR, holes=((FX - 10, FY - 10, FX + 778, FY + 490),))
    floor_line(M, FLOOR)
    # 위쪽 띠: 긴 흐린 튜브
    tube(M, 16, 10, 1248, 54, 27, CYAN, b=0.6)
    M.t(40, 42, 'WAVE 12', CYAN, 27, 'ls', BLOCK, glow=True, hollow=True)
    M.t(160, 42, '/ 30', CYAN, 14, 'ls', BLOCK, 0.7)
    M.t(40, 55, '다음 보스  20웨이브 · 돌진 기사', ORANGE, 10, 'ls', KO)
    EX, EW = 330, 420
    M.t(EX, 32, '남은 적 23 / 64', FG, 11, 'ls', KO)
    M.t(EX + EW, 32, '처치 41 · 놓침 2', GREY, 10, 'rs')
    M.fill_rr(EX, 39, EW * 23 / 64, 9, 4, CYAN, 0.35, layer=3)
    M.rr(EX, 39, EW, 9, 4, CYAN, 1.0, 0.35)
    heart_cursor(M, 800, 33, 7)
    M.t(814, 40, '목숨 18', WHITE, 17, 'ls', KO, glow=True)
    level_pill(M, 940, 22, 176, 32)
    hud_buttons(M, 1134, 23, 28, 7)
    # 왼쪽 족보 패널
    if left: left(M)
    else:
        tube(M, 16, 74, 232, 584, 12, CYAN, b=0.6)
        M.t(30, 100, '족보', CYAN, 15, 'ls', KO, glow=True)
        M.t(234, 100, '클릭: 그 족보 합성', DIMT, 9, 'rs')
        hand_rows(M, 16, 110, 232, 34)
        M.t(30, 536, '연쇄', WHITE, 12, 'ls', KO)
        M.t(30, 552, '족보끼리 맞닿으면 하나에 +45%씩', FG, 10)
        suits_legend(M, 30, 580)
    # 판
    cen = board(M, FX, FY, CS, show_speed=True)
    # 오른쪽: 판 위력 · 웨이브 · 버튼 · 묘약
    RX, PW = 1044, 220
    tube(M, RX, 74, PW, 250, 12, CYAN, b=0.6)
    power_block(M, RX, 74, PW)
    tube(M, RX, 334, PW, 46, 23, CYAN)
    M.t(RX + PW / 2, 356, '웨이브 12 진행 중', WHITE, 14, 'ms', KO)
    M.t(RX + PW / 2, 371, '배속 ×2 (F)', CYAN, 9, 'ms', KO)
    tube(M, RX, 390, PW, 36, 18, PINK, on=True)
    heart_cursor(M, RX - 10, 408, 6)
    M.t(RX + PW / 2, 413, '스킬 트리', WHITE, 14, 'ms', KO, glow=True)
    M.t(RX + PW - 20, 412, 'K', PINK, 9, 'rs', KO)
    M.t(RX + 20, 412, '포인트 1', GOLD, 9, 'ls', KO, glow=True)
    tube(M, RX, 434, 106, 36, 18, CYAN)
    M.t(RX + 53, 457, '조커 도감', WHITE, 13, 'ms', KO)
    tube(M, RX + 114, 434, 106, 36, 18, CYAN)
    M.t(RX + 167, 457, '메뉴', WHITE, 13, 'ms', KO)
    tube(M, RX, 482, PW, 176, 12, CYAN, b=0.6)
    M.t(RX + 14, 506, '묘약 2/5', CYAN, 12, 'ls', KO)
    M.t(RX + PW - 14, 506, '다음 15웨이브', DIMT, 9, 'rs')
    for i, (n, d) in enumerate(CHARMS):
        y = 536 + i * 34
        M.t(RX + 14, y, n, WHITE, 11, 'ls', KO)
        M.t(RX + 14, y + 15, d, GREY, 10)
    for i in range(3):
        y = 604 + i * 16
        M.ln([(RX + 14, y), (RX + 90, y)], DIMT, 1.0, 0.25, glow=False)
    M.t(RX + 100, 608, '빈 칸 3', DIMT, 9)
    # 손패 줄 (바닥에 세운 카드)
    HY = 570
    M.t(FX, HY + 22, '손패 5/7', CYAN, 13, 'ls', KO)
    M.t(FX, HY + 42, '덱 34장', GREY, 11)
    tube(M, FX, HY + 54, 86, 30, 15, CYAN)
    M.t(FX + 43, HY + 73, '자리바꿈 ×2', WHITE, 10, 'ms', KO)
    img = M.draw()
    # 판매 안내 (오른쪽 클릭한 3♦): 금색 고리 + 버튼 → 위층
    P = Bar()
    sx, sy = cen((14, 5))
    P.ln(circle(sx, sy, CS * 0.62, 40), GOLD, 1.5, 0.95)
    tube(P, sx - 62, sy + CS * 0.62 + 6, 124, 28, 14, PINK, on=True)
    P.t(sx, sy + CS * 0.62 + 25, '판매 +12 경험치', WHITE, 11, 'ms', KO, glow=True)
    for i, (r, sk) in enumerate(HAND):
        x = 358 + 80 * i
        hand_card(P, x, HY - (10 if i == SEL else 0), r, sk, i, i == SEL)
    P.t(1264, 711, '?  도움말', DIMT, 10, 'rs')
    img = P.draw(img)
    img = reflect(img, (0, FLOOR - 104, W, FLOOR), FLOOR + 3, 0.2)
    return img


# ================================================================ 족보 패널 = 메뉴 고르기 시안 (A 화면 안, x 16 · y 74 · 232 × 584)
# 마우스를 올린(고른) 줄 = 스트레이트 (합성 ×1 가능), Q♥ 를 놓아 보는 중이라 플러시는 금색 미리보기
HOVER = 6
MULT = ['×2.5', '×5', '×2', '×3.5', '×1.5', '범위', '연사', '×2', '×1.5', '×1.5']  # 효과 글의 첫 배수 (개수가 없을 때 오른쪽)
ICON = ['royal', 'five', 'sf', 'four', 'full', 'flush', 'straight', 'triple', 'twopair', 'pair']
PX, PY, PWL, PHL = 16, 74, 232, 584


def hand_icon(M, kind, cx, cy, c, b=0.85):
    """무늬 없는 작은 족보 기호: 카드 장 수 · 모양 (18 × 14 안)"""
    def card(x, y, ang=0.0):
        P = [(-2.3, -3.6), (2.3, -3.6), (2.3, 3.6), (-2.3, 3.6), (-2.3, -3.6)]
        ca, sa = math.cos(math.radians(ang)), math.sin(math.radians(ang))
        M.ln([(x + px * ca - py * sa, y + px * sa + py * ca) for px, py in P], c, 0.9, b, glow=False)
    rows = {'pair': [-3, 3], 'triple': [-5.5, 0, 5.5], 'twopair': [-8, -3, 3, 8], 'four': [-8.1, -2.7, 2.7, 8.1],
            'five': [-9.2, -4.6, 0, 4.6, 9.2], 'full': [-9.5, -5, -0.5, 5, 9.5]}
    if kind in rows:
        for x in rows[kind]: card(cx + x, cy)
    elif kind in ('straight', 'sf'):
        n = 4 if kind == 'straight' else 5
        for i in range(n): card(cx + (i - (n - 1) / 2) * 4.6, cy + 3 - i * 6 / (n - 1))
        if kind == 'sf': M.ln([(cx + 9, cy - 8), (cx + 11, cy - 6)], c, 0.9, b, glow=False)
    elif kind == 'flush':
        for a in (-24, 0, 24): card(cx + math.sin(math.radians(a)) * 4, cy + 1 - math.cos(math.radians(a)) * 2, a)
    elif kind == 'royal':
        M.ln([(cx - 8, cy + 4), (cx - 8, cy - 4), (cx - 4, cy), (cx, cy - 6), (cx + 4, cy), (cx + 8, cy - 4), (cx + 8, cy + 4), (cx - 8, cy + 4)], c, 1.0, b, glow=False)
    elif kind == 'joker':
        from neon import jester_hat
        for p in jester_hat(cx, cy + 4, 8): M.ln(p, c, 0.9, b, glow=False)
    elif kind == 'feed':  # 족보 → 조커 화살표
        M.ln([(cx - 9, cy), (cx + 2, cy)], c, 1.0, b, glow=False)
        M.ln([(cx - 2, cy - 4), (cx + 2, cy), (cx - 2, cy + 4)], c, 1.0, b, glow=False)
        M.ln(circle(cx + 6.5, cy, 3, 12), c, 1.0, b, glow=False)


def entries():
    """족보 10줄 + 조커 · 먹이기: (key, 이름, 효과, 색, 개수, 합성, 아이콘)"""
    E = [(i, n, e, hx(hc), cnt, fz, ICON[i]) for i, (n, e, hc, cnt, fz) in enumerate(HANDS)]
    E.append(('joker', '조커', '클릭: 만들 수 있는 조커 목록', JPINK, 1, 0, 'joker'))
    E.append(('feed', '먹이기', '클릭: 조커에 족보 먹이기', JPINK, 1, 0, 'feed'))
    return {e[0]: e for e in E}


def badge(M, xr, y, key, cnt, fz, c):
    """오른쪽 끝 xr 에 맞춘 배지. 돌려준 값 = 배지 왼쪽 x"""
    if fz: txt, bc = '합성 ×%d' % fz, GOLD
    elif key == 'feed': txt, bc = '먹이 ×%d' % cnt, c
    elif cnt: txt, bc = '×%d' % cnt, c
    else: return xr
    w = M.width(txt, 9, KO) + 14
    M.rr(xr - w, y - 8, w, 16, 8, bc, 1.0, 0.75, glow=False)
    M.t(xr - w / 2, y + 3.5, txt, bc, 9, 'ms', KO)
    return xr - w


def footer(M, y, one_row=False):
    M.t(30, y, '연쇄', WHITE, 12, 'ls', KO)
    M.t(30, y + 16, '족보끼리 맞닿으면 하나에 +45%씩', FG, 10)
    if one_row:
        M.t(30, y + 40, '무늬 = 공격 방식', GREY, 9)
        for i, k in enumerate(('spade', 'heart', 'diamond', 'club')):
            xx = 36 + i * 52
            suit_shape(M, k, xx, y + 56, 5.5, None, 1.2, 0.7)
            M.t(xx + 9, y + 60, ATK[k], SUITC[k], 10)
    else:
        suits_legend(M, 30, y + 44)


def panel_head(M, title='족보', hint='클릭: 그 족보 합성'):
    tube(M, PX, PY, PWL, PHL, 12, CYAN, b=0.6)
    M.t(30, 100, title, CYAN, 15, 'ls', KO)
    M.t(234, 100, hint, DIMT, 9, 'rs')


# ---- 족보 메뉴판 시안 공통: 고르는 방식은 지금 그대로 (줄 클릭 = 그 족보 합성, 마우스를 올린 줄이 켜짐)
#      모양만 바 메뉴판처럼: 간판 머리글, 묶음 머리글, "이름 ····· ×배수" 점선, 이름 아래 작은 설명, 추천 꼬리표(합성 · 먹이)
CREAM = (238, 232, 218)
GROUPS = [('5장 족보', [0, 1, 2, 4, 5, 6]), ('2~4장 족보', [3, 7, 8, 9]), ('조커 · 먹이', ['joker', 'feed'])]


def tag(M, x, y, txt, c, rec=False):
    """가격표 꼬리표 (왼쪽이 뾰족, 구멍 하나). rec = 추천 (합성 · 먹이): 작은 별을 단다"""
    w = M.width(txt, 9, KO) + (24 if rec else 16)
    P = [(x, y), (x + 6, y - 7.5), (x + w, y - 7.5), (x + w, y + 7.5), (x + 6, y + 7.5), (x, y)]
    M.fill(P, c, 0.12 if rec else 0.06, layer=3)
    M.ln(P, c, 1.0, 0.85, glow=False)
    M.ln(circle(x + 5, y, 1.2, 8), c, 0.8, 0.8, glow=False)
    tx = x + 10
    if rec:
        for p in suit('wild')(x + 14, y, 3.6): M.ln(p, c, 0.9, 0.95, glow=False)
        tx += 9
    M.t(tx, y + 3.5, txt, c, 9, 'ls', KO)
    return w


def menu_items(M, E, y, x0, xr, style):
    """메뉴 항목들. style: neon / letter / marquee. 돌려준 값 = 끝 y"""
    for gi, (title, keys) in enumerate(GROUPS):
        cx = (x0 + xr) / 2
        if style == 'letter':
            M.t(cx, y + 12, title, CREAM, 10, 'ms', KO)
            w = M.width(title, 10, KO)
            for s in (-1, 1):
                for k in range(3): M.ln([(cx + s * (w / 2 + 8 + k * 7), y + 8), (cx + s * (w / 2 + 11 + k * 7), y + 8)], CREAM, 1.0, 0.55, glow=False)
        elif style == 'marquee':
            w = M.t(x0, y + 12, title, ORANGE, 10, 'ls', KO)
            M.ln([(x0 + w + 8, y + 8), (xr, y + 8)], ORANGE, 1.0, 0.4, glow=False)
        else:
            w = M.width(title, 10, KO)
            M.t(cx, y + 12, title, FRAME, 10, 'ms', KO)
            for s in (-1, 1):
                M.ln([(cx + s * (w / 2 + 8), y + 8), (cx + s * (w / 2 + 40), y + 8)], FRAME, 1.0, 0.45, glow=False)
                suit_shape(M, 'spade', cx + s * (w / 2 + 46), y + 8, 3, FRAME, 0.9, 0.6)
        y += 19
        for k in keys:
            key, n, e, c, cnt, fz, ic = E[k]
            hov, pv = key == HOVER, key == PREVIEW_ROW
            h = 40 if hov else 28
            if hov: tube(M, x0 - 8, y - 1, xr - x0 + 16, h + 1, 10, PINK, on=True, fill=True)
            elif pv: M.fill_rr(x0 - 6, y, xr - x0 + 12, h, 6, GOLD, 0.09, layer=3)
            ny = y + (18 if hov else 13)
            dy = y + (32 if hov else 25)
            nx = x0
            col = WHITE if hov else GOLD if pv else c if cnt else GREY
            if style == 'letter':  # 족보 색은 이름 앞 작은 칸으로 (글자는 모두 흰 플라스틱 글자)
                if not hov: M.fill([(x0, ny - 7), (x0 + 5, ny - 7), (x0 + 5, ny + 1), (x0, ny + 1)], c, 0.9 if cnt else 0.35, layer=3)
                nx = x0 + 10
                col = WHITE if hov else CREAM if (cnt or pv) else dim(CREAM, 0.7)
            elif style == 'marquee':
                if not hov:
                    M.rr(x0, y + 3, 20, 20, 4, c, 0.9, 0.8 if cnt else 0.4, glow=False)
                    hand_icon(M, ic, x0 + 10, y + 13, c, 0.95 if cnt else 0.5)
                nx = x0 + 26
            if hov:  # 마우스를 올린 줄: 앞 표시 자리에 하트
                heart_cursor(M, x0 + (10 if style == 'marquee' else 3), ny - 4, 4.5)
                if style == 'neon': nx += 12
            # 오른쪽 가격 = 배수 (조커 · 먹이는 개수), 추천 꼬리표는 가격 아래 (설명 줄 오른쪽)
            price = MULT[key] if isinstance(key, int) else '×%d' % cnt
            pc = WHITE if hov else (CREAM if style == 'letter' else GOLD if style == 'marquee' else FG)
            isx = price.startswith('×')
            pw = M.t(xr, ny + 1, price, pc, (12 if style != 'marquee' else 13) if isx else 10.5, 'rs', BLOCK if isx else KO)
            tw_ = 0
            if fz: tw_ = tag(M, xr - M.width('합성 ×%d' % fz, 9, KO) - 24, dy - 3, '합성 ×%d' % fz, GOLD, True)
            elif key == 'feed': tw_ = tag(M, xr - M.width('먹이 ×%d' % cnt, 9, KO) - 24, dy - 3, '먹이 ×%d' % cnt, JPINK, True)
            nw = M.t(nx, ny + 1, n, col, 11.5, 'ls', KO, maxw=xr - nx - pw - 44)
            tx = nx + nw + 6
            if cnt and isinstance(key, int) and not fz: tx += tag(M, tx, ny - 3, '판 %d' % cnt, c) + 4
            for dx in range(int(tx + 2), int(xr - pw - 5), 5):
                M.ln([(dx, ny - 1), (dx + 1.3, ny - 1)], CREAM if style == 'letter' else DIMT, 1.0, 0.4, glow=False)
            dc = (255, 200, 225) if hov else GOLD if pv else (dim(CREAM, 0.62) if style == 'letter' else GREY)
            M.t(nx, dy, e, dc, 9, 'ls', KOR, maxw=xr - nx - (tw_ + 5 if tw_ else 0))
            y += h
        y += 2
    return y


# ---- 1. 네온 메뉴 간판: 금색 튜브 "족 보" + MENU 글자 + 스페이드 장식
def menu_neon(M):
    tube(M, PX, PY, PWL, PHL, 12, CYAN, b=0.6)
    M.t(132, 118, '족 보', GOLD, 28, 'ms', KO, glow=True, hollow=True)
    for s in (-1, 1):
        suit_shape(M, 'spade', 132 + s * 62, 108, 6, GOLD, 1.3, 0.9)
    M.t(132, 134, 'H A N D S   ·   M E N U', FRAME, 9, 'ms', BLOCK)
    M.ln([(36, 142), (228, 142)], GOLD, 1.0, 0.5)
    M.t(228, 154, '클릭: 그 족보 합성', DIMT, 8.5, 'rs')
    y = menu_items(M, entries(), 156, 38, 232, 'neon')
    footer(M, y + 18, one_row=True)


# ---- 2. 레터보드: 홈 파인 판 + 흰 플라스틱 글자, 족보 색은 이름 앞 작은 칸
def menu_letter(M):
    tube(M, PX, PY, PWL, PHL, 12, CYAN, b=0.6)
    for yy in range(150, 560, 8):  # 레터보드 홈
        M.bg([(30, yy), (234, yy)], GRID, 0.09)
    M.t(132, 108, 'H A N D S', CREAM, 10, 'ms', BLOCK)
    M.t(132, 134, '족보 메뉴', CREAM, 20, 'ms', KO)
    M.ln([(36, 143), (228, 143)], CREAM, 1.0, 0.45, glow=False)
    M.t(228, 155, '클릭: 그 족보 합성', dim(CREAM, 0.55), 8.5, 'rs')
    y = menu_items(M, entries(), 157, 38, 232, 'letter')
    footer(M, y + 18, one_row=True)


# ---- 3. 마키 간판: 전구 두른 머리글, 항목마다 작은 족보 기호 칸, 가격은 금색 숫자
def menu_marquee(M):
    tube(M, PX, PY, PWL, PHL, 12, CYAN, b=0.6)
    M.rr(34, 86, 196, 58, 10, ORANGE, 1.6, 0.75)
    bulbs(M, [(42, 93), (222, 93)], 15, 0)
    bulbs(M, [(42, 137), (222, 137)], 15, 1)
    M.t(132, 127, '족 보', GOLD, 26, 'ms', KO, glow=True, hollow=True)
    for s in (-1, 1): suit_shape(M, 'spade', 132 + s * 58, 115, 5.5, GOLD, 1.2, 0.9)
    M.t(228, 160, '클릭: 그 족보 합성', DIMT, 8.5, 'rs')
    y = menu_items(M, entries(), 162, 34, 232, 'marquee')
    footer(M, y + 18, one_row=True)



# ================================================================ B 걸린 간판
def mock_B():
    M = Bar()
    FX, FY, CS = 256, 104, 48
    FLOOR = 598
    bricks(M, 0, FLOOR, holes=((FX - 10, FY - 10, FX + 778, FY + 490),))
    floor_line(M, FLOOR)
    # 로고 배지 (왼쪽 위)
    draw_logo(M, 112, 6, 0.27)
    # 남은 적 간판: 전구 줄
    wires(M, 286, 436, 22)
    tube(M, 270, 20, 182, 68, 10, CYAN)
    M.t(286, 44, '남은 적', FG, 11, 'ls', KO)
    M.t(436, 44, '23 / 64', WHITE, 13, 'rs', BLOCK)
    n = 16
    for i in range(n):
        x = 287 + i * 9.8
        on = i < round(16 * 23 / 64)
        M.ln(circle(x, 60, 3.2, 10), (255, 225, 150), 1.1, 0.95 if on else 0.15)
        if on: M.dots.append((x, 60, 1.2))
    M.t(361, 80, '처치 41 · 놓침 2', GREY, 8, 'ms')
    # WAVE 간판 (가운데)
    wires(M, 500, 780, 14)
    tube(M, 470, 14, 340, 74, 14, CYAN, on=True)
    M.t(612, 60, 'WAVE 12', CYAN, 38, 'ms', BLOCK, glow=True, hollow=True)
    M.t(735, 60, '/ 30', CYAN, 18, 'ms', BLOCK, 0.75)
    M.t(640, 79, '다음 보스  20웨이브 · 돌진 기사', ORANGE, 10, 'ms', KO)
    # 목숨 간판
    wires(M, 842, 918, 22)
    tube(M, 826, 22, 108, 60, 12, PINK, b=0.9)
    heart_cursor(M, 852, 52, 11)
    M.t(910, 62, '18', PINK, 26, 'ms', BLOCK, glow=True, hollow=True)
    # 레벨 간판
    wires(M, 962, 1106, 22)
    level_pill(M, 948, 38, 172, 34)
    # 버튼 셋
    hud_buttons(M, 1146, 38, 30, 8)
    # 왼쪽: 족보 메뉴판
    wires(M, 40, 210, 66)
    tube(M, 14, 66, 226, 528, 12, GOLD, b=0.65)
    M.t(127, 104, '족 보', GOLD, 24, 'ms', KO, glow=True, hollow=True)
    M.ln([(40, 114), (214, 114)], GOLD, 1.0, 0.4)
    M.t(127, 128, '클릭: 그 족보 합성', DIMT, 9, 'ms')
    hand_rows(M, 14, 134, 226, 33, effect=False, leader=True)
    M.ln([(40, 538), (214, 538)], GOLD, 1.0, 0.25, glow=False)
    M.t(28, 556, '연쇄  족보끼리 맞닿으면 +45%씩', FG, 10, maxw=200)
    for i, k in enumerate(('spade', 'heart', 'diamond', 'club')):
        xx = 34 + i * 50
        suit_shape(M, k, xx, 576, 5.5, None, 1.2, 0.7)
        M.t(xx + 9, 580, ATK[k], SUITC[k], 10)
    # 판
    board(M, FX, FY, CS, frame_c=FRAME, frame_b=0.5)
    # 오른쪽 열
    RX, PW = 1042, 222
    tube(M, RX, 104, PW, 228, 12, CYAN, b=0.6)
    power_block(M, RX, 104, PW, rows=6, rowh=20)
    # 웨이브 마키 (진행 중이라 전구는 꺼짐)
    tube(M, RX, 342, PW, 58, 14, CYAN)
    bulbs(M, [(RX + 22, 354), (RX + PW - 22, 354)], 16, 0, c=(120, 105, 80))
    M.t(RX + PW / 2, 378, '웨이브 12 진행 중', WHITE, 14, 'ms', KO)
    M.t(RX + PW / 2, 392, '배속 ×2 (F) · 끝나면 전구가 켜진다', CYAN, 8, 'ms', KOR, 0.8)
    tube(M, RX, 410, PW, 34, 17, PINK, on=True)
    heart_cursor(M, RX - 10, 427, 6)
    M.t(RX + PW / 2, 432, '스킬 트리', WHITE, 13, 'ms', KO, glow=True)
    M.t(RX + 20, 431, '포인트 1', GOLD, 9, 'ls', KO, glow=True)
    M.t(RX + PW - 20, 431, 'K', PINK, 9, 'rs', KO)
    tube(M, RX, 452, 107, 34, 17, CYAN)
    M.t(RX + 53.5, 474, '조커 도감', WHITE, 12, 'ms', KO)
    tube(M, RX + 115, 452, 107, 34, 17, CYAN)
    M.t(RX + 168.5, 474, '메뉴', WHITE, 12, 'ms', KO)
    # 묘약 선반
    M.t(RX + 6, 512, '묘약 2/5', CYAN, 12, 'ls', KO)
    M.t(RX + PW - 6, 512, '다음 15웨이브', DIMT, 9, 'rs')
    for i in range(5):
        cx = RX + 24 + i * 43
        charm_bottle(M, cx, 538, [PINK, CYAN][i] if i < 2 else GREY, i < 2)
    M.ln([(RX, 552), (RX + PW, 552)], (120, 110, 140), 1.2, 0.45)
    for i, (nm, d) in enumerate(CHARMS):
        M.t(RX + 6, 570 + i * 16, nm, [PINK, CYAN][i], 10, 'ls', KO)
        M.t(RX + 80, 570 + i * 16, d, GREY, 9, maxw=PW - 84)
    img = M.draw()
    img = reflect(img, (FX - 10, FY + 380, FX + 778, FLOOR - 4), FLOOR + 3, 0.22)
    img = reflect(img, (RX - 4, 500, RX + PW + 4, 556), FLOOR + 3, 0.12)
    # 바닥에 놓인 손패 (반사 위에 그림)
    P = Bar()
    HY = 610
    x0 = 640 - (4 * 80 + 70) / 2
    for i, (r, sk) in enumerate(HAND):
        hand_card(P, x0 + 80 * i, HY - (8 if i == SEL else 0), r, sk, i, i == SEL, 70, 96)
    P.t(300, HY + 30, '손패 5/7', CYAN, 13, 'ls', KO)
    P.t(300, HY + 50, '덱 34장', GREY, 11)
    tube(P, 300, HY + 60, 90, 30, 15, CYAN)
    P.t(345, HY + 79, '자리바꿈 ×2', WHITE, 10, 'ms', KO)
    P.t(1264, 711, '?  도움말', DIMT, 10, 'rs')
    return P.draw(img)


# ================================================================ C 판이 주인공 (접힌 튜브 패널)
def icon_btn(M, x, y, kind, label, c, hot):
    tube(M, x - 26, y - 26, 52, 52, 26, PINK if hot else CYAN, on=hot)
    b, c = 1.0, WHITE
    if kind == 'tree':
        M.ln([(x, y + 12), (x, y - 4)], c, 1.4, b)
        M.ln([(x, y - 4), (x - 8, y - 10)], c, 1.4, b); M.ln([(x, y - 4), (x + 8, y - 10)], c, 1.4, b)
        for (px, py) in ((x - 8, y - 10), (x + 8, y - 10), (x, y - 14)):
            M.ln(circle(px, py, 2.6, 10), c, 1.2, b)
        M.ln([(x, y - 4), (x, y - 11)], c, 1.4, b)
    elif kind == 'codex':
        M.ln(rr_pts(x - 10, y - 12, 20, 24, 3), c, 1.3, b)
        M.ln([(x - 5, y - 5), (x + 5, y - 5)], c, 1.1, b); M.ln([(x - 5, y + 1), (x + 5, y + 1)], c, 1.1, b)
    else:
        for dy in (-6, 0, 6): M.ln([(x - 9, y + dy), (x + 9, y + dy)], c, 1.5, b)
    M.t(x, y + 44, label, WHITE, 11, 'ms', KO, glow=hot)


def mock_C():
    M = Bar(gk=0.75)
    FX, FY, CS = 224, 64, 52
    FLOOR = 704
    bricks(M, 0, FLOOR, a=0.08, holes=((FX - 10, FY - 10, FX + 16 * CS + 10, FY + 10 * CS + 10),))
    floor_line(M, FLOOR)
    # 위쪽: 얇은 줄 (상자 없음)
    draw_logo(M, 46, 6, 0.17)
    M.t(100, 30, 'WAVE 12', CYAN, 20, 'ls', BLOCK, glow=True)
    M.t(184, 30, '/ 30', CYAN, 12, 'ls', BLOCK, 0.7)
    M.t(100, 46, '다음 보스  20웨이브 · 돌진 기사', ORANGE, 9, 'ls', KO)
    EX, EW = 330, 330
    M.t(EX, 26, '남은 적 23 / 64', FG, 11, 'ls', KO)
    M.t(EX + EW, 26, '처치 41 · 놓침 2', GREY, 9, 'rs')
    M.fill_rr(EX, 33, EW * 23 / 64, 8, 4, CYAN, 0.35, layer=3)
    M.rr(EX, 33, EW, 8, 4, CYAN, 1.0, 0.35)
    heart_cursor(M, 712, 30, 7)
    M.t(726, 37, '목숨 18', WHITE, 16, 'ls', KO, glow=True)
    level_pill(M, 900, 16, 176, 32)
    hud_buttons(M, 1134, 17, 30, 7)
    # 왼쪽: 접힌 족보 튜브 (마우스를 올린 줄만 분홍으로 켜지고 효과가 펼쳐짐)
    HOV = 6
    LX, LW = 16, 192
    M.t(LX + 4, FY + 4, '족보', CYAN, 13, 'ls', KO)
    M.t(LX + LW, FY + 4, '클릭: 합성', DIMT, 9, 'rs')
    y = FY + 14
    rows = [(n, e, hx(hc), cnt, fz) for (n, e, hc, cnt, fz) in HANDS] + [('조커', '클릭: 만들 수 있는 조커 목록', JPINK, 1, 0), ('먹이기', '클릭: 조커에 족보 먹이기', JPINK, 1, 0)]
    for i, (n, e, c, cnt, fz) in enumerate(rows):
        hov = i == HOV
        h = 50 if hov else 30
        pv = i == PREVIEW_ROW
        col = PINK if hov else GOLD if pv else CYAN
        tube(M, LX, y, LW, h, 10, col, on=hov, b=None if (hov or pv or cnt) else 0.62)
        M.t(LX + 14, y + 20, n, WHITE if (hov or cnt or pv) else FG, 11, 'ls', KO, glow=hov, maxw=LW - 70)
        badge = ('합성 ×%d' % fz) if fz else ('먹이 ×1' if n == '먹이기' else '×%d' % cnt if cnt else '')
        if badge:
            bc = GOLD if fz else c
            bw = 46 if len(badge) > 2 else 26
            M.rr(LX + LW - 12 - bw, y + 8, bw, 15, 7, bc, 1.0, 0.75)
            M.t(LX + LW - 12 - bw / 2, y + 19.5, badge, bc, 9, 'ms', KO)
        if hov:
            M.t(LX + 14, y + 38, e, PINK, 9, 'ls', KOR, maxw=LW - 28)
            heart_cursor(M, LX + LW + 9, y + h / 2, 6)
        y += h + 5
    M.t(LX + 4, y + 14, '연쇄 · 맞닿은 족보마다 +45%', FG, 10, maxw=LW)
    for i, k in enumerate(('spade', 'heart', 'diamond', 'club')):
        xx = LX + 8 + i * 48
        suit_shape(M, k, xx, y + 34, 5.5, None, 1.2, 0.7)
        M.t(xx + 9, y + 38, ATK[k], SUITC[k], 10)
    # 판 (크게)
    cen = board(M, FX, FY, CS, frame_c=FRAME, frame_b=0.6)
    # 오른쪽 좁은 열
    RX, PW = 1072, 192
    tube(M, RX, FY, PW, 168, 12, CYAN, b=0.6)
    M.t(RX + 14, FY + 22, '판 위력', CYAN, 12, 'ls', KO)
    M.t(RX + PW - 14, FY + 22, '초당 피해', DIMT, 9, 'rs')
    M.t(RX + 14, FY + 56, '3,412', WHITE, 28, 'ls', BLOCK, glow=True)
    M.t(RX + PW - 14, FY + 42, '+12%', GREEN, 10, 'rs', KO)
    M.t(RX + PW - 14, FY + 56, '족보 덤 ×1.8', FG, 9, 'rs')
    for i, (n, c, sh, v) in enumerate(POWER[:4]):
        yy = FY + 80 + i * 18
        M.t(RX + 14, yy, n, c, 10, 'ls', KO, maxw=90)
        M.t(RX + PW - 14, yy, '%s · %d%%' % (v, round(sh * 100)), FG, 10, 'rs')
    M.t(RX + PW / 2, FY + 158, '▾ 7위까지 보기', DIMT, 9, 'ms')
    tube(M, RX, FY + 180, PW, 48, 24, CYAN)
    M.t(RX + PW / 2, FY + 203, '웨이브 12 진행 중', WHITE, 13, 'ms', KO)
    M.t(RX + PW / 2, FY + 217, '배속 ×2 (F)', CYAN, 8, 'ms', KOR, 0.85)
    for i, (kind, lab, c, hot) in enumerate((('tree', '트리 K', GOLD, False), ('codex', '도감 J', JPINK, False), ('menu', '메뉴 Esc', GREY, False))):
        icon_btn(M, RX + 32 + i * 64, FY + 272, kind, lab, c, hot)
    M.t(RX + 32 + 18, FY + 250, '1', GOLD, 10, 'ls', KO, glow=True)
    tube(M, RX, FY + 344, PW, 176, 12, CYAN, b=0.6)
    M.t(RX + 14, FY + 368, '묘약 2/5', CYAN, 12, 'ls', KO)
    M.t(RX + PW - 14, FY + 368, '다음 15', DIMT, 9, 'rs')
    for i in range(5):
        cx = RX + 26 + i * 35
        charm_bottle(M, cx, FY + 400, [PINK, CYAN][i] if i < 2 else GREY, i < 2)
    for i, (nm, d) in enumerate(CHARMS):
        M.t(RX + 14, FY + 440 + i * 34, nm, [PINK, CYAN][i], 11, 'ls', KO)
        M.t(RX + 14, FY + 455 + i * 34, d, GREY, 9, maxw=PW - 28)
    img = M.draw()
    # 위층: 손패 · 조커 툴팁
    P = Bar(gk=0.75)
    HY = 598
    x0 = FX + 16 * CS / 2 - (4 * 82 + 70) / 2
    for i, (r, sk) in enumerate(HAND):
        hand_card(P, x0 + 82 * i, HY - (8 if i == SEL else 0), r, sk, i, i == SEL, 70, 96)
    P.t(FX, HY + 26, '손패 5/7', CYAN, 13, 'ls', KO)
    P.t(FX, HY + 46, '덱 34장', GREY, 11)
    tube(P, FX, HY + 58, 90, 30, 15, CYAN)
    P.t(FX + 45, HY + 77, '자리바꿈 ×2', WHITE, 10, 'ms', KO)
    P.t(1264, 694, '?  도움말', DIMT, 10, 'rs')
    jx, jy = cen(JOKER)
    bw, bh = 236, 92
    bx, by = jx - 30 - bw, jy + 34
    P.fill_rr(bx, by, bw, bh, 10, (10, 11, 20), 0.95, layer=2)
    P.rr(bx, by, bw, bh, 10, JPINK, 1.4, 0.8)
    P.ln(circle(jx, jy, CS * 0.55, 36), JPINK, 1.2, 0.8)
    P.t(bx + 14, by + 22, '할리퀸 · Lv3', JPINK, 13, 'ls', KO, glow=True)
    P.t(bx + 14, by + 40, '위력 1.1k/초 (판의 32%)', GREEN, 10, 'ls', KO)
    P.t(bx + 14, by + 56, '조커 레벨 3 · 경험치 40/90 · 위력 +30%', FG, 9, maxw=bw - 28)
    P.t(bx + 14, by + 78, '오른쪽 클릭: 판매 (+18 경험치)', GOLD, 9, 'ls', KO)
    img = P.draw(img)
    img = reflect(img, (FX, FLOOR - 40, FX + 16 * CS, FLOOR), FLOOR + 3, 0.18)
    return img


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    for name, fn in (('ingame_A_table', mock_A), ('ingame_B_signs', mock_B), ('ingame_C_board', mock_C),
                     ('hands_menu_1_neon', lambda: mock_A(menu_neon)), ('hands_menu_2_letterboard', lambda: mock_A(menu_letter)),
                     ('hands_menu_3_marquee', lambda: mock_A(menu_marquee))):
        fn().save(os.path.join(outdir, name + '.png'))
        print(name)


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
