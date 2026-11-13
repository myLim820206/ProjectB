"""네온 선 화풍 미리보기 (docs/15-art-style.md)

언리얼 빌드 없이 같은 화풍으로 그림을 그려 PNG로 확인한다.
그림은 '선 목록'(점 목록의 목록)을 돌려주는 함수로 만든다. 좌표는 중심 (cx, cy), 크기 R 기준.

    python3 tools/neon/neon.py joker 출력.png      # 조커 광대 머리
    python3 tools/neon/neon.py suits 출력.png      # 카드 무늬 4종 + 별
    python3 tools/neon/neon.py towers 출력.png     # 타워 (틀 + 숫자 + 무늬 기호)
    python3 tools/neon/neon.py floors 출력.png     # 족보 장판 10색 (옅은 채움 + 빗금 + 테두리)
    python3 tools/neon/neon.py suitcolors 출력.png # 무늬 색 비교: 위 4색 / 아래 2색 (♥♦ 빨강, ♠♣ 흰빛)
    python3 tools/neon/neon.py jokers 출력.png     # 조커 카드 후보 10종
    python3 tools/neon/neon.py shots 출력.png      # 발사체 5종 (♠ 레이저, ♥ 미사일, ♦ 탄환, ♣ 서리 구체, ★ 별)
    python3 tools/neon/neon.py tree 출력.png tools/tree/tree.json   # 스킬 트리 화면 (tree.json 은 tools/tree/gen_tree.py 가 만듦)

새 그림은 아래 SHAPES 에 함수를 더하면 된다. 큰 크기 / 손패 크기 / 타워 크기 세 가지로 나란히 그린다.
"""
import math
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

BG = (6, 6, 11)
def _font_path():
    import os
    for p in ['/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf', 'C:/Windows/Fonts/malgunbd.ttf', 'C:/Windows/Fonts/arialbd.ttf']:
        if os.path.exists(p):
            return p
    return None


_FP = _font_path()
def _kfont_path():  # 한글 글꼴 (리눅스는 unifont 라도)
    import os
    for p in ['C:/Windows/Fonts/malgunbd.ttf', '/System/Library/Fonts/AppleSDGothicNeo.ttc', '/usr/share/fonts/opentype/unifont/unifont.otf']:
        if os.path.exists(p):
            return p
    return _FP


KFONT = lambda px: ImageFont.truetype(_kfont_path(), max(6, int(px * 1.33)))
FONT = lambda px: ImageFont.truetype(_FP, max(6, int(px * 1.33))) if _FP else ImageFont.load_default()
COL = {'spade': (127, 184, 255), 'heart': (255, 106, 138), 'diamond': (255, 217, 106), 'club': (138, 255, 196),
       'pink': (255, 154, 213), 'white': (255, 255, 255), 'cyan': (127, 214, 255),
       'red': (255, 84, 104), 'ice': (220, 232, 255), 'gold': (255, 217, 106)}


def bez(p0, p1, p2, n=18):
    """2차 베지어를 꺾은선으로"""
    return [((1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * p1[0] + t * t * p2[0],
             (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * p1[1] + t * t * p2[1]) for t in [i / n for i in range(n + 1)]]


def circle(cx, cy, r, n=36, sx=1.0):
    return [(cx + math.cos(k / n * 2 * math.pi) * r * sx, cy + math.sin(k / n * 2 * math.pi) * r) for k in range(n + 1)]


# ---------------------------------------------------------------- 그림들
def heart_pts(cx, cy, R, flip, off):
    P = []
    for i in range(49):
        t = i / 48 * 2 * math.pi
        hx = 16 * math.sin(t) ** 3
        hy = -(13 * math.cos(t) - 5 * math.cos(2 * t) - 2 * math.cos(3 * t) - math.cos(4 * t))
        P.append((cx + hx / 17 * R, cy + (hy / 17 - 0.15) * R * flip + off))
    return P


def club_pts(cx, cy, R):
    O = (cx, cy - 0.06 * R)
    cs = [(cx, cy - 0.44 * R), (cx - 0.46 * R, cy + 0.1 * R), (cx + 0.46 * R, cy + 0.1 * R)]
    rad, P = 0.4 * R, []
    for i in range(73):
        a = i / 72 * 2 * math.pi
        u = (math.cos(a), math.sin(a))
        far = 0
        for c in cs:
            dx, dy = c[0] - O[0], c[1] - O[1]
            b = dx * u[0] + dy * u[1]
            disc = b * b - (dx * dx + dy * dy - rad * rad)
            if disc >= 0:
                far = max(far, b + math.sqrt(disc))
        P.append((O[0] + u[0] * far, O[1] + u[1] * far))
    return P


def suit(kind):
    def f(cx, cy, R):
        if kind == 'spade':
            return [heart_pts(cx, cy, R, -1, -R * 0.12), [(cx, cy + R * .42), (cx + R * .3, cy + R), (cx - R * .3, cy + R), (cx, cy + R * .42)]]
        if kind == 'heart':
            return [heart_pts(cx, cy, R, 1, 0)]
        if kind == 'diamond':
            return [[(cx, cy - R), (cx + R * .72, cy), (cx, cy + R), (cx - R * .72, cy), (cx, cy - R)]]
        if kind == 'club':
            return [club_pts(cx, cy, R), [(cx, cy + R * .5), (cx + R * .28, cy + R), (cx - R * .28, cy + R), (cx, cy + R * .5)]]
        star = []
        for i in range(11):
            a = -math.pi / 2 + i * 2 * math.pi / 10
            rr = R * .45 if i % 2 else R
            star.append((cx + math.cos(a) * rr, cy + math.sin(a) * rr))
        return [star]
    return f


def joker(cx, cy, R):
    """조커 광대 머리: 방울 달린 세 갈래 모자(양옆 처짐) + 웃는 얼굴 + 뾰족한 옷깃"""
    P = lambda x, y: (cx + x * R, cy + y * R)
    by = -0.12
    hat = []
    hat += bez(P(-0.5, by), P(-0.95, -0.95), P(-1.0, -0.22))
    hat += bez(P(-1.0, -0.22), P(-0.6, -0.5), P(-0.22, by - 0.22))[1:]
    hat += bez(P(-0.22, by - 0.22), P(-0.2, -0.75), P(0, -1.05))[1:]
    hat += bez(P(0, -1.05), P(0.2, -0.75), P(0.22, by - 0.22))[1:]
    hat += bez(P(0.22, by - 0.22), P(0.6, -0.5), P(1.0, -0.22))[1:]
    hat += bez(P(1.0, -0.22), P(0.95, -0.95), P(0.5, by))[1:]
    hat += bez(P(0.5, by), P(0, by + 0.14), P(-0.5, by))[1:]
    L = [hat]
    for tx, ty in [(-1.0, -0.22), (0, -1.05), (1.0, -0.22)]:
        L.append(circle(*P(tx, ty + 0.1), 0.1 * R, 20))
    fy, fr = 0.3, 0.34
    L.append(circle(*P(0, fy), fr * R, 36, 0.92))
    for ex in (-0.13, 0.13):
        L.append(bez(P(ex - 0.07, fy - 0.04), P(ex, fy - 0.13), P(ex + 0.07, fy - 0.04), 8))
    L.append(bez(P(-0.16, fy + 0.1), P(0, fy + 0.27), P(0.16, fy + 0.1), 10))
    L.append([P(-0.55 + i * (1.1 / 10), 0.6 + (0.24 if i % 2 else 0)) for i in range(11)])
    return L


def rrect(cx, cy, h, r, n=6):
    """둥근 사각형 (중심, 반 너비, 모서리 반지름)"""
    P = []
    for (ox, oy, a0) in [(h - r, -h + r, -90), (h - r, h - r, 0), (-h + r, h - r, 90), (-h + r, -h + r, 180)]:
        for k in range(n + 1):
            a = math.radians(a0 + k * 90 / n)
            P.append((cx + ox + math.cos(a) * r, cy + oy + math.sin(a) * r))
    P.append(P[0])
    return P


def tower(kind, rank):
    """타워: 둥근 사각 틀 + 숫자 + 작은 무늬 기호 (언리얼 CardShape 와 같은 배치)"""
    def f(cx, cy, R):
        L = [rrect(cx, cy, R * 0.98, R * 0.22)]
        if kind == 'wild':
            L += suit('wild')(cx, cy, R * 0.6)
            return L
        L += suit(kind)(cx, cy + R * 0.5, R * 0.3)
        L.append(('text', cx, cy - R * 0.24, rank, R * (0.62 if rank == '10' else 0.74)))
        return L
    return f


# ---------------------------------------------------------------- 조커 카드 후보 10종 (틀 + 기호)
def arc(cx, cy, r, a0, a1, n=16, sx=1.0):
    return [(cx + math.cos(math.radians(a0 + (a1 - a0) * k / n)) * r * sx, cy + math.sin(math.radians(a0 + (a1 - a0) * k / n)) * r) for k in range(n + 1)]


def scaled(fn, k, dx=0.0, dy=0.0):
    """그림 함수를 R 배율 k, 위치 (dx, dy)·R 로 옮긴다"""
    return lambda cx, cy, R: fn(cx + dx * R, cy + dy * R, R * k)


def jester_hat(cx, cy, R, band=True):
    P = lambda x, y: (cx + x * R, cy + y * R)
    by = -0.12
    hat = []
    hat += bez(P(-0.5, by), P(-0.95, -0.95), P(-1.0, -0.22))
    hat += bez(P(-1.0, -0.22), P(-0.6, -0.5), P(-0.22, by - 0.22))[1:]
    hat += bez(P(-0.22, by - 0.22), P(-0.2, -0.75), P(0, -1.05))[1:]
    hat += bez(P(0, -1.05), P(0.2, -0.75), P(0.22, by - 0.22))[1:]
    hat += bez(P(0.22, by - 0.22), P(0.6, -0.5), P(1.0, -0.22))[1:]
    hat += bez(P(1.0, -0.22), P(0.95, -0.95), P(0.5, by))[1:]
    hat += bez(P(0.5, by), P(0, by + 0.14), P(-0.5, by))[1:]
    L = [hat]
    for tx, ty in [(-1.0, -0.22), (0, -1.05), (1.0, -0.22)]:
        L.append(circle(*P(tx, ty + 0.1), 0.1 * R, 20))
    return L


def joker_cards():
    def frame(inner):
        def f(cx, cy, R):
            return [rrect(cx, cy, R * 0.98, R * 0.22)] + inner(cx, cy, R)
        return f

    def j1(cx, cy, R):  # 광대 머리
        return joker(cx, cy + 0.04 * R, R * 0.6)

    def j2(cx, cy, R):  # 광대 모자
        return jester_hat(cx, cy + 0.38 * R, R * 0.72)

    def j3(cx, cy, R):  # 웃는 가면 (눈가리개 + 미소)
        P = lambda x, y: (cx + x * R, cy + y * R)
        mask = bez(P(-0.68, -0.12), P(0, -0.52), P(0.68, -0.12)) + bez(P(0.68, -0.12), P(0.35, 0.22), P(0, 0.02))[1:] + bez(P(0, 0.02), P(-0.35, 0.22), P(-0.68, -0.12))[1:]
        eyeL = bez(P(-0.45, -0.12), P(-0.3, -0.26), P(-0.15, -0.12), 8) + bez(P(-0.15, -0.12), P(-0.3, -0.04), P(-0.45, -0.12), 8)[1:]
        eyeR = [(2 * cx - x, y) for x, y in eyeL]
        tails = [[P(-0.68, -0.12), P(-0.82, 0.12)], [P(-0.68, -0.12), P(-0.88, -0.02)], [P(0.68, -0.12), P(0.82, 0.12)], [P(0.68, -0.12), P(0.88, -0.02)]]
        smile = bez(P(-0.3, 0.32), P(0, 0.6), P(0.3, 0.32), 12)
        return [mask, eyeL, eyeR, smile] + tails

    def j4(cx, cy, R):  # 희비극 가면 (웃음 + 울음)
        P = lambda x, y: (cx + x * R, cy + y * R)
        L = [circle(*P(-0.24, -0.1), 0.38 * R, 32), circle(*P(0.26, 0.14), 0.38 * R, 32)]
        L += [bez(P(-0.38, -0.2), P(-0.32, -0.28), P(-0.26, -0.2), 6), bez(P(-0.18, -0.2), P(-0.12, -0.28), P(-0.06, -0.2), 6)]
        L += [bez(P(-0.42, 0.0), P(-0.24, 0.18), P(-0.06, 0.0), 10)]                 # 웃는 입
        L += [bez(P(0.14, 0.06), P(0.2, 0.0), P(0.26, 0.06), 6), bez(P(0.32, 0.06), P(0.38, 0.0), P(0.44, 0.06), 6)]
        L += [bez(P(0.12, 0.34), P(0.28, 0.18), P(0.44, 0.34), 10)]                  # 우는 입
        return L

    def j5(cx, cy, R):  # 할리퀸 마름모 + 방울
        P = lambda x, y: (cx + x * R, cy + y * R)
        dia = lambda x, y, w, h: [P(x, y - h), P(x + w, y), P(x, y + h), P(x - w, y), P(x, y - h)]
        L = [dia(0, 0.02, 0.24, 0.42), dia(-0.42, 0.08, 0.17, 0.3), dia(0.42, 0.08, 0.17, 0.3)]
        L += [circle(*P(0, -0.52), 0.08 * R, 16), circle(*P(-0.42, -0.34), 0.07 * R, 14), circle(*P(0.42, -0.34), 0.07 * R, 14)]
        return L

    def j6(cx, cy, R):  # 광대 지팡이
        P = lambda x, y: (cx + x * R, cy + y * R)
        L = [[P(0, -0.05), P(0, 0.72)], [P(-0.12, 0.72), P(0.12, 0.72)]]
        L += joker(cx, cy - 0.32 * R, R * 0.36)
        return L

    def j7(cx, cy, R):  # J 모노그램 + 모자
        L = jester_hat(cx, cy - 0.22 * R, R * 0.48)
        L.append(('text', cx, cy + 0.26 * R, 'J', R * 0.78))
        return L

    def j8(cx, cy, R):  # 체셔 미소
        P = lambda x, y: (cx + x * R, cy + y * R)
        grin = bez(P(-0.66, 0.0), P(0, 0.62), P(0.66, 0.0)) + bez(P(0.66, 0.0), P(0, 0.3), P(-0.66, 0.0))[1:]
        teeth = [[P(x, 0.12 + 0.01), P(x, 0.27 - abs(x) * 0.25)] for x in (-0.34, -0.17, 0, 0.17, 0.34)]
        eyes = [bez(P(-0.42, -0.32), P(-0.28, -0.46), P(-0.14, -0.32), 8), bez(P(0.14, -0.32), P(0.28, -0.46), P(0.42, -0.32), 8)]
        return [grin] + teeth + eyes

    def j9(cx, cy, R):  # 방울
        P = lambda x, y: (cx + x * R, cy + y * R)
        bell = bez(P(-0.42, 0.34), P(-0.38, -0.36), P(0, -0.36)) + bez(P(0, -0.36), P(0.38, -0.36), P(0.42, 0.34))[1:]
        rim = bez(P(-0.52, 0.34), P(0, 0.48), P(0.52, 0.34))
        L = [bell, rim, [P(-0.52, 0.34), P(0.52, 0.34)], circle(*P(0, 0.52), 0.08 * R, 16)]
        L += [bez(P(0, -0.36), P(0.05, -0.62), P(0.34, -0.62), 10), circle(*P(0.42, -0.6), 0.07 * R, 14)]  # 위에 말린 모자 끝
        return L

    def j10(cx, cy, R):  # 모자 쓴 별
        L = suit('wild')(cx, cy + 0.22 * R, R * 0.52)
        L += jester_hat(cx, cy - 0.24 * R, R * 0.4)
        return L

    designs = [j1, j2, j3, j4, j5, j6, j7, j8, j9, j10]
    return [(frame(d), 'pink') for d in designs]


# ---------------------------------------------------------------- 스킬 트리 화면 (1280 x 720, 언리얼 트리 창과 같은 배치)
# tree.json 은 tools/tree/gen_tree.py 가 만든다 (노드 위치가 화면 좌표 그대로)
SUIT_COL = ['spade', 'heart', 'diamond', 'club']
THEME_COL = SUIT_COL + ['cyan', 'white', 'pink', 'white']


def tree_color(n):
    return 'gold' if n['k'] == 3 else THEME_COL[n['theme']]


# ---------------------------------------------------------------- 트리 노드 아이콘 (D-119): 노드가 하는 일마다 기호 하나
# EPDStat: 1 SuitDmg 2 SuitRate 3 SuitRange 4 SuitSplash 5 SuitSlow 6 HandDmg 7 HandRate 8 HandSplash 9 AllDmg 10 ChainAdd
#          11 FuseAdd 12 FusedDmg 13 GoldPct 14 HandMax 15 Draw 16 Lives 19 JokerDmg 20 JokerAdd 21 AllRate 22 CritChance 23 CritMult
# 좌표는 반지름 r 기준 (-1..1). 모두 닫힌 윤곽선으로 그려 작아져도 덩어리로 읽히게 한다
def _xf(cx, cy, r, ang=0.0, ox=0.0, oy=0.0, sc=1.0):
    a = math.radians(ang); c, s = math.cos(a), math.sin(a)
    return lambda x, y: (cx + (ox + (x * c - y * s) * sc) * r, cy + (oy + (x * s + y * c) * sc) * r)


def _poly(P, pts, close=True):
    L = [P(x, y) for x, y in pts]
    return L + [L[0]] if close else L


def _rbox(P, x0, y0, x1, y1, rad, n=4):
    """P 좌표계 안의 둥근 사각형 (회전 따라감)"""
    pts = []
    for (qx, qy, a0) in ((x1 - rad, y0 + rad, -90), (x1 - rad, y1 - rad, 0), (x0 + rad, y1 - rad, 90), (x0 + rad, y0 + rad, 180)):
        for k in range(n + 1):
            a = math.radians(a0 + k * 90 / n)
            pts.append((qx + math.cos(a) * rad, qy + math.sin(a) * rad))
    return _poly(P, pts)


def _arc(P, x, y, rad, a0, a1, n=16, sx=1.0):
    return [P(x + math.cos(math.radians(a0 + (a1 - a0) * k / n)) * rad * sx, y + math.sin(math.radians(a0 + (a1 - a0) * k / n)) * rad) for k in range(n + 1)]


def _sword(P):
    """칼: 끝이 뾰족한 칼날 윤곽 + 홈 + 코등이 + 손잡이 + 끝 구슬 (세로, 위가 칼끝)"""
    return [_poly(P, [(0, -1.0), (0.17, -0.78), (0.17, 0.3), (-0.17, 0.3), (-0.17, -0.78)]),
            [P(0, -0.72), P(0, 0.22)],
            _rbox(P, -0.48, 0.3, 0.48, 0.44, 0.07),
            _poly(P, [(-0.08, 0.44), (0.08, 0.44), (0.08, 0.78), (-0.08, 0.78)]),
            [P(math.cos(math.radians(a)) * 0.12, 0.9 + math.sin(math.radians(a)) * 0.12) for a in range(0, 361, 30)]]


def _arrow(P, tail=True):
    """빠른 화살: 날렵한 화살촉 + 살 + 깃 (오른쪽으로)"""
    L = [_poly(P, [(1.0, 0), (0.45, -0.38), (0.55, -0.1), (-0.55, -0.1), (-0.55, 0.1), (0.55, 0.1), (0.45, 0.38)]),
         _poly(P, [(-0.55, -0.1), (-0.85, -0.38), (-1.0, -0.38), (-0.75, -0.1)], False),
         _poly(P, [(-0.55, 0.1), (-0.85, 0.38), (-1.0, 0.38), (-0.75, 0.1)], False)]
    return L


def _bolt(P):
    return [_poly(P, [(0.22, -1.0), (-0.5, 0.12), (-0.06, 0.12), (-0.3, 1.0), (0.5, -0.18), (0.06, -0.18), (0.32, -1.0)])]


def _heart(P, k=1.0):
    pts = []
    for i in range(0, 361, 8):
        t = math.radians(i)
        x = 16 * math.sin(t) ** 3
        y = -(13 * math.cos(t) - 5 * math.cos(2 * t) - 2 * math.cos(3 * t) - math.cos(4 * t))
        pts.append((x / 17 * k, (y + 2) / 17 * k))
    return _poly(P, pts, False)


def _card(P, pip=True):
    L = [_rbox(P, -0.4, -0.62, 0.4, 0.62, 0.1)]
    if pip:
        L.append(_poly(P, [(0, -0.22), (0.16, 0), (0, 0.22), (-0.16, 0)]))
    return L


def _burst(P, n=8, ro=1.0, ri=0.55, rot=0.0):
    pts = []
    for k in range(n * 2):
        a = math.radians(rot + k * 180 / n - 90)
        rr_ = ro if k % 2 == 0 else ri
        pts.append((math.cos(a) * rr_, math.sin(a) * rr_))
    return _poly(P, pts)


def _inside(pt, poly):
    x, y = pt; c = False
    for i in range(len(poly) - 1):
        (x1, y1), (x2, y2) = poly[i], poly[i + 1]
        if (y1 > y) != (y2 > y) and x < x1 + (y - y1) * (x2 - x1) / (y2 - y1): c = not c
    return c


def _hide(lines, fronts, step=0.6):
    """앞 물체(닫힌 윤곽 fronts)에 가려지는 선 조각을 지운다 (겹친 카드·동전·사람)"""
    out = []
    for ln in lines:
        cur = []
        for i in range(len(ln) - 1):
            (x1, y1), (x2, y2) = ln[i], ln[i + 1]
            n = max(1, int(math.hypot(x2 - x1, y2 - y1) / step))
            for k in range(n):
                a = (x1 + (x2 - x1) * k / n, y1 + (y2 - y1) * k / n)
                b = (x1 + (x2 - x1) * (k + 1) / n, y1 + (y2 - y1) * (k + 1) / n)
                mid = ((a[0] + b[0]) / 2, (a[1] + b[1]) / 2)
                if any(_inside(mid, f) for f in fronts):
                    if len(cur) > 1: out.append(cur)
                    cur = []
                else:
                    if not cur: cur = [a]
                    cur.append(b)
        if len(cur) > 1: out.append(cur)
    return out


def _stack(groups):
    """groups: 뒤→앞 순서의 (선들, 윤곽). 뒤 것은 앞 윤곽에 가려진다"""
    L = []
    for i, (lines, _) in enumerate(groups):
        L += _hide(lines, [g[1] for g in groups[i + 1:]]) if i + 1 < len(groups) else lines
    return L


def _cards(specs, pips):
    """specs: 뒤→앞 (cx, cy, r, ang, ox, oy, sc)"""
    G = []
    for sp, pip in zip(specs, pips):
        L = _card(_xf(*sp), pip)
        G.append((L, L[0]))
    return _stack(G)


def _ring(P, x, y, rad, sx=1.0, n=28):
    return _arc(P, x, y, rad, 0, 360, n, sx)


def _person(P, x, y, s):
    return [_ring(P, x, y - 0.32 * s, 0.24 * s, 1.0, 18), _arc(P, x, y + 0.45 * s, 0.42 * s, 180, 360, 14, 1.0) + [P(x - 0.42 * s, y + 0.45 * s)]]


def _person_sil(P, x, y, s):
    """사람 윤곽 (머리 둘레 + 어깨를 조금 넓게): 뒤 사람을 가릴 때"""
    head = _ring(P, x, y - 0.32 * s, 0.32 * s, 1.0, 18)
    body = _arc(P, x, y + 0.45 * s, 0.5 * s, 180, 360, 14, 1.0) + [P(x + 0.5 * s, y + 0.6 * s), P(x - 0.5 * s, y + 0.6 * s), P(x - 0.5 * s, y + 0.45 * s)]
    return [head, body]


def stat_icon(st, cx, cy, r):
    P = _xf(cx, cy, r)
    if st in (1, 6):                      # 공격: 칼
        return _sword(_xf(cx, cy, r, 45))
    if st == 9:                           # 모든 공격: 엇갈린 칼 둘
        return _sword(_xf(cx, cy, r, 45, sc=0.95)) + _sword(_xf(cx, cy, r, -45, sc=0.95))
    if st == 12:                          # 합성 타워 공격: 겹쳐 쌓인 판 셋 위에 칼
        G = []
        for k in (2, 1, 0):   # 아래 판부터 (위 판이 가린다)
            y = 0.25 + k * 0.25
            d = _poly(P, [(-0.9, y), (0, y - 0.32), (0.9, y), (0, y + 0.32)])
            G.append(([d], d))
        return _stack(G) + _sword(_xf(cx, cy, r, 0, 0, -0.35, 0.62))
    if st == 19:                          # 조커 공격: 칼 + 칼끝에 광대 방울
        return _sword(_xf(cx, cy, r, 45, -0.05, 0.05, 0.9)) + [_ring(P, 0.68, -0.68, 0.2, 1.0, 16), [P(0.55, -0.68), P(0.81, -0.68)]]
    if st in (2, 7):                      # 연사: 빠른 화살 + 바람 줄
        return _arrow(_xf(cx, cy, r, -30, 0, 0, 1.0))
    if st == 21:                          # 모든 연사: 겹친 화살 둘
        return _arrow(_xf(cx, cy, r, -30, 0.2, -0.32, 0.8)) + _arrow(_xf(cx, cy, r, -30, -0.2, 0.32, 0.8))
    if st == 3:                           # 사거리: 조준경 (두 겹 원 + 눈금 + 가운데 점)
        return [_ring(P, 0, 0, 0.72), _ring(P, 0, 0, 0.12, 1.0, 12),
                [P(-1.0, 0), P(-0.42, 0)], [P(0.42, 0), P(1.0, 0)], [P(0, -1.0), P(0, -0.42)], [P(0, 0.42), P(0, 1.0)]]
    if st in (4, 8):                      # 범위: 터지는 별 + 가운데 원
        return [_burst(P, 8, 1.0, 0.58, 22.5), _ring(P, 0, 0, 0.28, 1.0, 16)]
    if st == 5:                           # 둔화: 눈꽃 (육각 가운데 + 가지 끝 갈래)
        L = [_poly(P, [(math.cos(math.radians(30 + 60 * k)) * 0.22, math.sin(math.radians(30 + 60 * k)) * 0.22) for k in range(6)])]
        for k in range(6):
            a = math.radians(-90 + 60 * k); ux, uy = math.cos(a), math.sin(a)
            L.append([P(ux * 0.22, uy * 0.22), P(ux, uy)])
            for sg in (1, -1):
                b = a + sg * math.radians(40)
                L.append([P(ux * 0.6, uy * 0.6), P(ux * 0.6 + math.cos(b) * 0.3, uy * 0.6 + math.sin(b) * 0.3)])
        return L
    if st == 10:                          # 연쇄: 맞물린 사슬 고리 둘 (비스듬히)
        Q = _xf(cx, cy, r, -45)
        return [_rbox(Q, -1.0, -0.32, 0.15, 0.32, 0.3), _rbox(Q, -0.15, -0.32, 1.0, 0.32, 0.3)]
    if st == 11:                          # 합성 배율: 두 길이 굽어 모여 위로 하나
        L = [_rbox(P, -1.0, 0.35, -0.55, 0.8, 0.1), _rbox(P, 0.55, 0.35, 1.0, 0.8, 0.1), _rbox(P, -0.42, -0.95, 0.42, -0.11, 0.14),
             _burst(_xf(cx, cy, r, 0, 0, -0.53, 0.3), 5, 1.0, 0.45, 0)]
        L += [bez(P(-0.78, 0.35), P(-0.75, -0.05), P(-0.48, -0.2), 8), _poly(P, [(-0.62, -0.32), (-0.46, -0.2), (-0.6, -0.06)], False),
              bez(P(0.78, 0.35), P(0.75, -0.05), P(0.48, -0.2), 8), _poly(P, [(0.62, -0.32), (0.46, -0.2), (0.6, -0.06)], False)]
        return L
    if st == 13:                          # 골드: 쌓인 동전 셋 (타원 + 옆면)
        G = []
        for k in range(3):    # 아래 동전부터
            y = 0.5 - k * 0.38
            top = _ring(P, 0, y - 0.12, 0.26, 3.0, 24)
            side = [P(-0.78, y - 0.12), P(-0.78, y + 0.06)] + _arc(P, 0, y + 0.06, 0.26, 180, 0, 12, 3.0) + [P(0.78, y - 0.12)]
            sil = _arc(P, 0, y - 0.12, 0.26, 180, 360, 12, 3.0) + [P(0.78, y + 0.06)] + _arc(P, 0, y + 0.06, 0.26, 0, 180, 12, 3.0)[1:] + [P(-0.78, y - 0.12)]
            G.append(([top, side], sil))
        return _stack(G)
    if st == 14:                          # 손패 칸: 부채꼴 카드 셋
        return _cards([(cx, cy, r, k * 24, k * 0.42, 0.12 + abs(k) * 0.08, 0.95) for k in (-1, 1, 0)], [False, False, True])
    if st == 15:                          # 드로우: 카드 + 뽑는 굽은 화살
        L = _card(_xf(cx, cy, r, -8, -0.3, 0.15, 0.95))
        L.append(bez(P(0.25, 0.7), P(0.95, 0.55), P(0.75, -0.6), 12))
        L.append(_poly(P, [(0.5, -0.45), (0.78, -0.85), (0.98, -0.4)], False))
        return L
    if st == 16:                          # 목숨: 하트 + 안쪽 빛
        return [_heart(P, 1.0), _arc(P, -0.38, -0.25, 0.22, 200, 280, 8)]
    if st == 20:                          # 조커 하나 더: 광대 모자 + 더하기
        return jester_hat(cx - r * 0.12, cy + r * 0.55, r * 0.95, band=True) + [[P(0.5, -0.78), P(1.0, -0.78)], [P(0.75, -1.03), P(0.75, -0.53)]]
    if st == 22:                          # 치명 확률: 번개
        return _bolt(P)
    if st == 23:                          # 치명 배율: 터지는 별 안에 번개
        return [_burst(P, 10, 1.0, 0.78)] + _bolt(_xf(cx, cy, r, sc=0.6))
    return [_ring(P, 0, 0, 0.4)]


def keystone_icon(k, cx, cy, r):
    P = _xf(cx, cy, r)
    if k == 0:                            # 외골수: 깎은 보석 하나 (한 무늬만)
        return [_poly(P, [(-0.9, -0.3), (-0.5, -0.8), (0.5, -0.8), (0.9, -0.3), (0, 0.95)]), [P(-0.9, -0.3), P(0.9, -0.3)],
                _poly(P, [(-0.5, -0.8), (-0.25, -0.3), (0, 0.95)], False), _poly(P, [(0.5, -0.8), (0.25, -0.3), (0, 0.95)], False),
                [P(-0.25, -0.3), P(0, -0.8), P(0.25, -0.3)]]
    if k == 1:                            # 잡식가: 네 무늬 다
        L = []
        for (kind, ox, oy) in (('spade', -0.48, -0.48), ('heart', 0.48, -0.48), ('diamond', -0.48, 0.48), ('club', 0.48, 0.48)):
            L += suit(kind)(cx + ox * r, cy + oy * r, r * 0.4)
        return L
    if k == 2:                            # 큰 판: 다섯 장 부채
        return _cards([(cx, cy, r, j * 17, j * 0.3, 0.1 + abs(j) * 0.06, 0.82) for j in (-2, 2, -1, 1, 0)], [True] * 5)
    if k == 3:                            # 작은 판: 두 장 (원 페어)
        return _cards([(cx, cy, r, -14, -0.3, 0, 0.95), (cx, cy, r, 14, 0.3, 0, 0.95)], [True, True])
    if k == 4:                            # 대장장이: 모루 + 망치
        return [_poly(P, [(-0.95, 0.05), (0.75, 0.05), (0.75, 0.25), (0.35, 0.4), (0.35, 0.65), (0.6, 0.9), (-0.6, 0.9), (-0.35, 0.65), (-0.35, 0.4), (-0.7, 0.25)]),
                _rbox(_xf(cx, cy, r, -35, 0.1, -0.38), -0.5, -0.2, 0.15, 0.2, 0.06), [_xf(cx, cy, r, -35, 0.1, -0.38)(0.15, 0), _xf(cx, cy, r, -35, 0.1, -0.38)(0.9, 0)]]
    if k == 5:                            # 광대의 길: 광대 모자
        return jester_hat(cx, cy + r * 0.5, r * 0.95, band=False)
    if k == 6:                            # 고독한 저격수: 조준경 안에 한 사람
        return [_ring(P, 0, 0, 0.92), [P(-1.0, 0), P(-0.62, 0)], [P(0.62, 0), P(1.0, 0)], [P(0, -1.0), P(0, -0.62)], [P(0, 0.62), P(0, 1.0)]] + _person(P, 0, 0.05, 0.85)
    if k == 7:                            # 군중: 세 사람
        fronts = _person_sil(P, -0.55, 0.25, 0.75) + _person_sil(P, 0.55, 0.25, 0.75)   # 양옆이 앞, 가운데가 뒤
        return _hide(_person(P, 0, -0.1, 0.9), fronts) + _person(P, -0.55, 0.25, 0.75) + _person(P, 0.55, 0.25, 0.75)
    return []


def render_icon_sheet(out, S=2):
    """아이콘을 크게 / 실제 크기로 나란히 (확인용)"""
    stats = [(1, '공격'), (9, '모든 공격'), (12, '합성 공격'), (19, '조커 공격'), (2, '연사'), (21, '모든 연사'), (3, '사거리'), (4, '범위'),
             (5, '둔화'), (10, '연쇄'), (11, '합성 배율'), (13, '골드'), (14, '손패 칸'), (15, '드로우'), (16, '목숨'), (20, '조커 +1'), (22, '치명 확률'), (23, '치명 배율')]
    keys = ['외골수', '잡식가', '큰 판', '작은 판', '대장장이', '광대의 길', '고독한 저격수', '군중']
    W, H = 1280, 720
    M = _Mock(S, W, H)
    for i, (st, n) in enumerate(stats):
        x, y = 70 + (i % 9) * 132, 90 + (i // 9) * 200
        M.lines += [(ln, (127, 214, 255), 1.4) for ln in stat_icon(st, x, y, 36)]
        M.lines += [(circle(x, y + 92, 11, 28), (127, 214, 255), 1.2)] + [(ln, (127, 214, 255), 0.9) for ln in stat_icon(st, x, y + 92, 7)]
        M.tx(x, y + 50, n, (207, 216, 230), 11, 'mm')
    for i, n in enumerate(keys):
        x, y = 80 + i * 150, 560
        M.lines += [(ln, (255, 217, 106), 1.4) for ln in keystone_icon(i, x, y, 36)]
        oct_ = [(x + 90 * 0 + math.cos(math.radians(22.5 + 45 * j)) * 22, y + 100 + math.sin(math.radians(22.5 + 45 * j)) * 22) for j in range(9)]
        M.lines += [(oct_, (255, 217, 106), 1.4)] + [(ln, (255, 217, 106), 0.9) for ln in keystone_icon(i, x, y + 100, 13)]
        M.tx(x, y + 50, n, (207, 216, 230), 11, 'mm')
    base = Image.new('RGB', (W * S, H * S), BG)
    M.draw(base).resize((W, H), Image.LANCZOS).save(out)


def tree_parts(n, cx, cy):
    """(틀, 안쪽 기호). 기호는 틀보다 가는 선으로 그린다"""
    k = n['k']
    if k == 0:
        return [circle(cx, cy, 14, 32)], suit('wild')(cx, cy, 8)
    if k == 1:   # 작은 노드: 동그라미 안에 하는 일 기호
        return [circle(cx, cy, 11, 32)], stat_icon(n['fx'][0][0], cx, cy, 6.8)
    if k == 2:   # 굵은 노드: 둥근 사각 안에 주된 효과 기호 (색이 무늬·주제)
        return [rrect(cx, cy, 15, 4)], stat_icon(n['fx'][0][0], cx, cy, 10)
    if k == 3:   # 키스톤: 팔각 틀 안에 키스톤마다 다른 기호
        oct_ = [(cx + math.cos(math.radians(22.5 + 45 * j)) * 21, cy + math.sin(math.radians(22.5 + 45 * j)) * 21) for j in range(9)]
        return [oct_], keystone_icon(n['fx'][0][1], cx, cy, 13)
    L = joker_cards()[n['fx'][0][1]][0](cx, cy, 16)   # 조커 노드: 그 조커의 카드 그림
    return L[:1], L[1:]


def tree_shape(n, cx, cy):
    f, i = tree_parts(n, cx, cy)
    return f + i


def render_tree(out, path, owned=None, sealed=(), S=2):
    import json
    nodes = json.load(open(path, encoding='utf-8'))
    if owned is None:  # 예시: ♠ 큰길 끝 키스톤 + 조커 하나 + ♥ 주머니 하나
        owned = {0}
        def walk_to(target):
            prev = {0: None}
            q = [0]
            while q:
                c = q.pop(0)
                for m in nodes[c]['l']:
                    if m not in prev:
                        prev[m] = c
                        q.append(m)
            while target is not None:
                owned.add(target)
                target = prev[target]
        keys = [n['id'] for n in nodes if n['k'] == 3]
        jokers = [n['id'] for n in nodes if n['k'] == 4]
        walk_to(keys[0])
        walk_to(jokers[0])
        hn = [n['id'] for n in nodes if n['k'] == 2 and n['theme'] == 1]
        walk_to(hn[-1])
        sealed = (keys[1],)
    COL.setdefault('gold', (255, 210, 110))
    COL.setdefault('red', (255, 90, 110))
    img = Image.new('RGB', (1280 * S, 720 * S), BG)
    jobs = []
    for n in nodes:
        for m in n['l']:
            if m < n['id']:
                continue
            on = n['id'] in owned and m in owned
            NR = (14, 11, 16, 21, 17)
            ax, ay, bx_, by_ = n['x'], n['y'], nodes[m]['x'], nodes[m]['y']
            dd = max(1e-3, math.hypot(bx_ - ax, by_ - ay)); ux, uy = (bx_ - ax) / dd, (by_ - ay) / dd
            ra, rb = NR[n['k']] + 1.5, NR[nodes[m]['k']] + 1.5
            a, b = (ax + ux * ra, ay + uy * ra), (bx_ - ux * rb, by_ - uy * rb)
            col = tree_color(nodes[m] if nodes[m]['k'] != 3 else n) if on else 'cyan'
            jobs.append(([[a, b]], col, 1.0 if on else 0.2, 1.1))
    for n in nodes:
        cx, cy = n['x'], n['y']
        i = n['id']
        if i in sealed:
            jobs.append((tree_shape(n, cx, cy), 'red', 0.5, 1.3))
            jobs.append(([[(cx - 11, cy - 11), (cx + 11, cy + 11)], [(cx + 11, cy - 11), (cx - 11, cy + 11)]], 'red', 0.8, 1.3))
        else:
            fr, ic = tree_parts(n, cx, cy)
            jobs.append((fr, tree_color(n), 1.0 if i in owned else 0.4, 1.5 if n['k'] >= 2 else 1.2))
            jobs.append((ic, tree_color(n), 1.0 if i in owned else 0.5, 0.9, 0.3))
    for job in jobs:
        lines, col, alpha, w = job[:4]
        gk = job[4] if len(job) > 4 else 1.0
        c = tuple(int(v * alpha) for v in COL[col])
        glow = Image.new('RGB', img.size, (0, 0, 0))
        gd = ImageDraw.Draw(glow)
        sl = [[(x * S, y * S) for x, y in ln] for ln in lines if not (ln and ln[0] == 'text')]
        tl = [ln for ln in lines if ln and ln[0] == 'text']   # 조커 카드 그림의 글자 (잭의 J)
        for ln in sl:
            gd.line(ln, fill=c, width=int(w * (4 if gk >= 1 else 2) * S))
        for ln in tl:
            gd.text((ln[1] * S, ln[2] * S), ln[3], fill=c, font=FONT(ln[4] * S), anchor='mm')
        img = ImageChops.add(img, glow.filter(ImageFilter.GaussianBlur(5 * S)).point(lambda v: int(v * 0.8 * gk)))
        d = ImageDraw.Draw(img)
        for ln in sl:
            d.line(ln, fill=c, width=max(1, int(w * S)))
        for ln in tl:
            d.text((ln[1] * S, ln[2] * S), ln[3], fill=c, font=FONT(ln[4] * S), anchor='mm')
    d = ImageDraw.Draw(img)
    d.text((40 * S, 40 * S), 'SKILL TREE  %d nodes' % len(nodes), fill=COL['gold'], font=FONT(14 * S), anchor='lm')
    img.resize((1280, 720), Image.LANCZOS).save(out)

# ---------------------------------------------------------------- 발사체 (무늬마다 다름): 왼쪽 → 오른쪽으로 날아가는 모습
def shot_laser(cx, cy, R):          # ♠ 저격: 즉시 레이저 (한 줄 + 총구 고리)
    return [[(cx - R, cy), (cx + R, cy)], circle(cx - R, cy, R * 0.12, 16), circle(cx + R, cy, R * 0.18, 16)]


def shot_missile(cx, cy, R):        # ♥ 폭발: 휘어 날아가는 미사일 (몸통 + 날개 + 연기 꼬리)
    L = [[(cx - R * .1, cy - R * .08), (cx + R * .35, cy - R * .08), (cx + R * .5, cy), (cx + R * .35, cy + R * .08), (cx - R * .1, cy + R * .08), (cx - R * .1, cy - R * .08)],
         [(cx - R * .1, cy - R * .08), (cx - R * .25, cy - R * .22), (cx - R * .05, cy - R * .08)],
         [(cx - R * .1, cy + R * .08), (cx - R * .25, cy + R * .22), (cx - R * .05, cy + R * .08)]]
    L += [circle(cx - R * (.35 + .17 * k), cy + math.sin(k * 1.3) * R * .06, R * (.05 + .02 * k), 12) for k in range(4)]
    return L


def shot_bullet(cx, cy, R):         # ♦ 연사: 짧은 탄환 셋 (빠른 줄무늬)
    return [[(cx - R * .9 + k * R * .6, cy + (k - 1) * R * .05), (cx - R * .6 + k * R * .6, cy + (k - 1) * R * .05)] for k in range(3)]


def shot_orb(cx, cy, R):            # ♣ 둔화: 서리 구체 (동그라미 + 눈꽃 + 옅은 꼬리)
    L = [circle(cx + R * .3, cy, R * .22, 24)]
    for k in range(3):
        a = math.radians(k * 60)
        L.append([(cx + R * .3 - math.cos(a) * R * .14, cy - math.sin(a) * R * .14), (cx + R * .3 + math.cos(a) * R * .14, cy + math.sin(a) * R * .14)])
    L += [circle(cx - R * (.1 + .25 * k), cy, R * (.08 - .02 * k), 12) for k in range(3)]
    return L


def shot_star(cx, cy, R):           # ★ 와일드: 도는 별 조각
    return suit('wild')(cx + R * .3, cy, R * .28) + [[(cx - R * .6, cy), (cx - R * .05, cy)]]


SHOTS = [(shot_laser, 'spade'), (shot_missile, 'heart'), (shot_bullet, 'diamond'), (shot_orb, 'club'), (shot_star, 'white')]


# ---------------------------------------------------------------- 레벨 진화 (합성 레벨 1~4): 같은 무늬가 레벨마다 굵고 화려해진다
def level_shot(kind, lv):
    def f(cx, cy, R):
        L = []
        if kind == 'spade':      # 레이저 → 쌍줄 → 레일건(나선 고리) → 레일건 + 충격 고리
            n = 1 if lv == 1 else 2
            for k in range(n):
                o = (k - (n - 1) / 2) * R * .12
                L.append([(cx - R, cy + o), (cx + R, cy + o)])
            if lv >= 3:
                for k in range(6):
                    x = cx - R * .8 + k * R * .32
                    L.append(circle(x, cy, R * .14, 16, sx=.35))
            if lv >= 4:
                L += [circle(cx + R, cy, R * .3, 24), circle(cx + R, cy, R * .45, 24)]
            L.append(circle(cx + R, cy, R * (.12 + .04 * lv), 16))
        elif kind == 'heart':    # 미사일 → 큰 미사일 → 세 발 떼 → 어뢰
            if lv == 3:
                for k, dy in enumerate([-.32, 0, .32]):
                    L += scaled(shot_missile, .6, .15 if k == 1 else -.1, dy)(cx, cy, R)
            else:
                L += scaled(shot_missile, .9 + .25 * (lv == 2) + .5 * (lv == 4))(cx, cy, R)
                if lv == 4:
                    L.append(circle(cx + R * .55, cy, R * .12, 12))
        elif kind == 'diamond':  # 탄환 → 쌍탄 → 세 줄 → 레이저 탄 (긴 줄 + 마름모 머리)
            rows = {1: [0], 2: [-.1, .1], 3: [-.18, 0, .18], 4: [0]}[lv]
            for dy in rows:
                for k in range(3 if lv < 4 else 1):
                    x0 = cx - R * .9 + k * R * .6 if lv < 4 else cx - R * .9
                    x1 = x0 + (R * .3 if lv < 4 else R * 1.5)
                    L.append([(x0, cy + dy * R), (x1, cy + dy * R)])
            if lv == 4:
                h = R * .16
                L.append([(cx + R * .6, cy), (cx + R * .6 + h, cy - h), (cx + R * .6 + 2 * h, cy), (cx + R * .6 + h, cy + h), (cx + R * .6, cy)])
        elif kind == 'club':     # 서리 구체 → 큰 구체 → 눈꽃 결정 → 얼음 혜성
            L += scaled(shot_orb, 1 + .2 * (lv - 1))(cx, cy, R)
            if lv >= 3:
                for k in range(6):
                    a = k * math.pi / 3
                    L.append([(cx + R * .3 + math.cos(a) * R * .3, cy + math.sin(a) * R * .3), (cx + R * .3 + math.cos(a) * R * .42, cy + math.sin(a) * R * .42)])
            if lv >= 4:
                L += [[(cx - R, cy - R * .15), (cx, cy - R * .05)], [(cx - R, cy + R * .15), (cx, cy + R * .05)]]
        else:                    # 별 → 별 둘 → 별 셋 → 큰 별 + 고리
            n = min(lv, 3)
            for k in range(n):
                L += suit('wild')(cx + R * (.3 - .32 * k), cy + (k % 2) * R * .12, R * (.28 - .05 * k))
            if lv >= 4:
                L.append(circle(cx + R * .3, cy, R * .42, 24))
        return L
    return f


# ---------------------------------------------------------------- 족보 발사체 (스트레이트 이상, 족보 색)
def hand_arrows(cx, cy, R):         # 스트레이트: 화살촉 셋이 줄지어
    return [[(cx + R * (k * .4 - .55), cy - R * .18), (cx + R * (k * .4 - .35), cy), (cx + R * (k * .4 - .55), cy + R * .18)] for k in range(3)] + [[(cx - R * .9, cy), (cx + R * .45, cy)]]


def hand_arc(cx, cy, R):            # 플러시: 꺾이는 번개 (즉시) + 옆으로 튀는 잔번개
    P = [(cx - R, cy), (cx - R * .5, cy - R * .2), (cx - R * .1, cy + R * .15), (cx + R * .35, cy - R * .15), (cx + R * .8, cy)]
    return [P, [(cx + R * .35, cy - R * .15), (cx + R * .55, cy - R * .45)], [(cx - R * .1, cy + R * .15), (cx + R * .1, cy + R * .45)], circle(cx + R * .8, cy, R * .1, 12)]


def hand_cluster(cx, cy, R):        # 풀하우스: 분열탄 (껍데기 + 안의 작은 알 다섯)
    L = [circle(cx + R * .2, cy, R * .3, 24), [(cx - R * .8, cy), (cx - R * .1, cy)]]
    for k in range(5):
        a = k * 2 * math.pi / 5
        L.append(circle(cx + R * .2 + math.cos(a) * R * .15, cy + math.sin(a) * R * .15, R * .05, 10))
    return L


def hand_shell(cx, cy, R):          # 포카드: 무거운 포탄 (큰 원 + 굵은 꼬리 둘)
    return [circle(cx + R * .25, cy, R * .38, 28), circle(cx + R * .25, cy, R * .2, 20),
            [(cx - R, cy - R * .2), (cx - R * .15, cy - R * .25)], [(cx - R, cy + R * .2), (cx - R * .15, cy + R * .25)]]


def hand_comet(cx, cy, R):          # 스트레이트 플러시: 혜성 (빛나는 머리 + 긴 꼬리 셋)
    L = [circle(cx + R * .5, cy, R * .18, 20), circle(cx + R * .5, cy, R * .32, 24, sx=1.2)]
    L += [[(cx + R * .35, cy + d * R * .12), (cx - R, cy + d * R * .3)] for d in (-1, 0, 1)]
    return L


def hand_nova(cx, cy, R):           # 파이브 카드: 퍼져 나가는 파동 (반원 셋)
    return [arc(cx - R * .4 + k * R * .35, cy, R * (.3 + k * .12), -70, 70, 16) for k in range(3)] + [circle(cx - R * .55, cy, R * .08, 10)]


def hand_crown(cx, cy, R):          # 로열: 금빛 굵은 광선 + 왕관
    c = (cx + R * .55, cy - R * .02)
    crown = [(c[0] - R * .3, c[1] + R * .15), (c[0] - R * .3, c[1] - R * .15), (c[0] - R * .15, c[1]), (c[0], c[1] - R * .22), (c[0] + R * .15, c[1]), (c[0] + R * .3, c[1] - R * .15), (c[0] + R * .3, c[1] + R * .15), (c[0] - R * .3, c[1] + R * .15)]
    return [[(cx - R, cy - R * .06), (cx + R * .2, cy - R * .06)], [(cx - R, cy + R * .06), (cx + R * .2, cy + R * .06)], crown]


# ---------------------------------------------------------------- 조커 발사체 (분홍, 조커마다 다름)
def jk_juggle(cx, cy, R):           # 광대: 포물선으로 던지는 공 (점선 궤적 + 공)
    L = [circle(cx + R * .6, cy + R * .1, R * .16, 20)]
    for k in range(6):
        t = k / 6
        x = cx - R + t * R * 1.5
        y = cy + R * .2 - math.sin(t * math.pi * .9) * R * .6
        L.append(circle(x, y, R * .03, 8))
    return L


def jk_hat(cx, cy, R):              # 모자 장수: 도는 모자 (부메랑처럼)
    hx, hy, s = cx + R * .25, cy, R * .3   # 실크해트: 챙 + 높은 몸통 + 띠, 기울어져 돈다
    rot = lambda x, y: (hx + (x * .94 - y * .34) * s, hy + (x * .34 + y * .94) * s)
    brim = [rot(-1.1, .55), rot(1.1, .55)]
    body = [rot(-.65, .55), rot(-.6, -.9), rot(.6, -.9), rot(.65, .55)]
    band = [rot(-.63, .2), rot(.63, .2)]
    return [brim, body, band, arc(hx, hy, R * .5, 150, 260, 12), arc(hx, hy, R * .5, -30, 80, 12)]


def jk_crescent(cx, cy, R):         # 웃는 가면: 도는 웃음 초승달
    return [arc(cx + R * .2, cy - R * .1, R * .4, 20, 160, 18), arc(cx + R * .2, cy - R * .25, R * .4, 35, 145, 18), circle(cx + R * .05, cy - R * .25, R * .05, 8), circle(cx + R * .35, cy - R * .25, R * .05, 8)]


def jk_twin(cx, cy, R):             # 희비극: 서로 감기며 나는 두 구슬
    L = [circle(cx + R * .5, cy - R * .15, R * .12, 14), circle(cx + R * .5, cy + R * .15, R * .12, 14)]
    L.append([(cx - R + k * R * .1, cy + math.sin(k * .7) * R * .15) for k in range(15)])
    L.append([(cx - R + k * R * .1, cy - math.sin(k * .7) * R * .15) for k in range(15)])
    return L


def jk_shards(cx, cy, R):           # 할리퀸: 부채꼴로 날아가는 마름모 셋
    L = []
    for k, a in enumerate([-.5, 0, .5]):
        x, y, h = cx + R * (.15 + .35 * math.cos(a)), cy + R * .8 * math.sin(a), R * .13
        L.append([(x - h * 1.6, y), (x, y - h), (x + h * 1.6, y), (x, y + h), (x - h * 1.6, y)])
        L.append([(cx - R * .7, cy), (x - h * 1.6, y)])
    return L


def jk_note(cx, cy, R):             # 지휘봉: 물결치며 나는 음표
    nx, ny = cx + R * .4, cy + R * .1
    L = [circle(nx, ny, R * .12, 16, sx=1.3), [(nx + R * .15, ny), (nx + R * .15, ny - R * .45), (nx + R * .35, ny - R * .3)]]
    L.append([(cx - R + k * R * .1, cy + math.sin(k * .9) * R * .12) for k in range(12)])
    return L


def jk_blade(cx, cy, R):            # 잭: 크게 휘두른 칼날 (초승달 베기, 느림)
    return [arc(cx, cy, R * .6, -70, 70, 20), arc(cx - R * .12, cy, R * .5, -60, 60, 18), [(cx + R * .21, cy - R * .56), (cx + R * .6, cy)], [(cx + R * .21, cy + R * .56), (cx + R * .6, cy)]]


def jk_grin(cx, cy, R):             # 체셔: 즉시 꽂히는 보랏빛 광선 + 목표에 웃는 입
    m = (cx + R * .7, cy)
    return [[(cx - R, cy), m], arc(m[0], m[1] - R * .1, R * .25, 20, 160, 14), circle(m[0] - R * .1, m[1] - R * .2, R * .04, 8), circle(m[0] + R * .1, m[1] - R * .2, R * .04, 8)]


def jk_wave(cx, cy, R):             # 방울: 퍼지는 소리 고리 + 작은 종
    L = [arc(cx - R * .2 + k * R * .25, cy, R * (.25 + k * .1), -55, 55, 14) for k in range(4)]
    b = (cx - R * .6, cy)
    L += [[(b[0] - R * .15, b[1] + R * .12), (b[0] - R * .1, b[1] - R * .12), (b[0] + R * .1, b[1] - R * .12), (b[0] + R * .15, b[1] + R * .12), (b[0] - R * .15, b[1] + R * .12)], circle(b[0], b[1] + R * .18, R * .04, 8)]
    return L


def jk_star(cx, cy, R):             # 스타: 큰 별 혜성
    return suit('wild')(cx + R * .4, cy, R * .3) + [[(cx + R * .15, cy + d * R * .1), (cx - R, cy + d * R * .25)] for d in (-1, 0, 1)]


SHOTS_LEVEL = [(level_shot(k, lv), k if k != 'wild' else 'white') for k in ['spade', 'heart', 'diamond', 'club', 'wild'] for lv in range(1, 5)]
SHOTS_LEVEL = [SHOTS_LEVEL[k * 4 + lv] for lv in range(4) for k in range(5)]  # 줄 = 레벨, 칸 = 무늬
SHOTS_HAND = [(hand_arrows, 'cyan'), (hand_arc, 'pink'), (hand_cluster, 'orange'), (hand_shell, 'red'), (hand_comet, 'pink'), (hand_nova, 'gold'), (hand_crown, 'white')]
SHOTS_JOKER = [(jk_juggle, 'pink'), (jk_hat, 'pink'), (jk_crescent, 'pink'), (jk_twin, 'pink'), (jk_shards, 'pink'), (jk_note, 'pink'), (jk_blade, 'pink'), (jk_grin, 'pink'), (jk_wave, 'pink'), (jk_star, 'pink')]
COL.update({'orange': (255, 168, 106), 'red': (255, 106, 106), 'gold': (255, 210, 110)})


SHAPES = {
    'jokers': joker_cards(),
    'towers': [(tower('spade', '9'), 'spade'), (tower('heart', 'A'), 'heart'), (tower('diamond', '10'), 'diamond'), (tower('club', 'K'), 'club'), (tower('wild', ''), 'white')],
    'joker': [(joker, 'pink')],
    # 무늬 색 줄이기 비교 (줄마다): 지금 4색 / 2색 (♥♦ 빨강, ♠♣ 흰빛, 와일드 금색)
    'suitcolors': [(tower('spade', '9'), 'spade'), (tower('heart', 'A'), 'heart'), (tower('diamond', '10'), 'diamond'), (tower('club', 'K'), 'club'), (tower('wild', ''), 'white'),
                   (tower('spade', '9'), 'ice'), (tower('heart', 'A'), 'red'), (tower('diamond', '10'), 'red'), (tower('club', 'K'), 'ice'), (tower('wild', ''), 'gold')],
    'shots': SHOTS,
    'shots_level': SHOTS_LEVEL,   # 줄 = 레벨 1~4, 칸 = ♠ ♥ ♦ ♣ ★
    'shots_hand': SHOTS_HAND,     # 스트레이트, 플러시, 풀하우스, 포카드, 스트레이트 플러시, 파이브 카드, 로열
    'shots_joker': SHOTS_JOKER,   # 조커 1~10
    'suits': [(suit('spade'), 'spade'), (suit('heart'), 'heart'), (suit('diamond'), 'diamond'), (suit('club'), 'club'), (suit('wild'), 'white')],
}


# ---------------------------------------------------------------- 그리기 (3겹 빛번짐 흉내: 흐린 번짐 + 심)
def render(name, out, S=4):
    items = SHAPES[name]
    if len(items) == 1:  # 그림 하나: 큰 그림 / 손패 크기 / 타워 크기
        W, H = 900, 320
        jobs = [(items[0][0], items[0][1], x, H // 2, R) for x, R in [(200, 110), (560, 55), (800, 24)]]
    else:                # 여러 그림: 손패 크기로 5칸씩 줄 맞춰 (아래에 번호)
        cols = min(5, len(items)); rows = (len(items) + cols - 1) // cols
        W, H = 180 * cols, 200 * rows
        jobs = [(fn, col, 90 + (k % cols) * 180, 90 + (k // cols) * 200, 60) for k, (fn, col) in enumerate(items)]
    img = Image.new('RGB', (W * S, H * S), BG)
    for fn, col, cx, cy, R in jobs:
        c = COL[col]
        core = max(1.5, 1.6 * R / 60) * S   # 심 굵기: 가늘고 또렷하게 (기준 화면 px × S)
        core = int(round(core))
        glow = Image.new('RGB', img.size, (0, 0, 0))
        gd = ImageDraw.Draw(glow)
        lines = fn(cx * S, cy * S, R * S)
        for ln in lines:
            if ln and ln[0] == 'text':
                gd.text((ln[1], ln[2]), ln[3], fill=c, font=FONT(ln[4]), anchor='mm')
            else:
                gd.line(ln, fill=c, width=core * 4)
        img = ImageChops.add(img, glow.filter(ImageFilter.GaussianBlur(7 * S)).point(lambda v: int(v * 0.9)))  # 넓고 부드러운 번짐 (기존 그림은 어둡게 하지 않음)
        d = ImageDraw.Draw(img)
        for ln in lines:
            if ln and ln[0] == 'text':
                d.text((ln[1], ln[2]), ln[3], fill=c, font=FONT(ln[4]), anchor='mm')
            else:
                d.line(ln, fill=c, width=core)
    if len(items) > 5:
        d = ImageDraw.Draw(img)
        for k in range(len(items)):
            d.text(((90 + (k % 5) * 180) * S, (90 + (k // 5) * 200 + 82) * S), str(k + 1), fill=(138, 148, 166), font=FONT(13 * S), anchor='mm')
    img.resize((W, H), Image.LANCZOS).save(out)


# ---------------------------------------------------------------- 족보 장판 (족보마다 색, 옅은 채움 + 빗금 + 테두리)
HAND_FLOOR = [  # (이름, 색, 장수) 높은 족보부터. 로열은 무지개로 돈다 (그림에서는 금백색)
    ('로열', (255, 244, 214), 5), ('파이브', (255, 217, 106), 5), ('스트레이트 플러시', (255, 154, 213), 5), ('포카드', (255, 106, 106), 4),
    ('풀하우스', (255, 168, 106), 5), ('플러시', (180, 140, 255), 3), ('스트레이트', (106, 140, 255), 3), ('트리플', (156, 255, 176), 3),
    ('투 페어', (127, 214, 255), 4), ('원 페어', (207, 216, 230), 2)]


def render_floors(out, S=3):
    cell, gap = 40, 6
    W, H = 5 * 230, 2 * 150
    img = Image.new('RGB', (W * S, H * S), BG)
    for k, (name, col, n) in enumerate(HAND_FLOOR):
        ox, oy = 20 + (k % 5) * 230, 30 + (k // 5) * 150
        layer = Image.new('RGB', img.size, (0, 0, 0))
        d = ImageDraw.Draw(layer)
        x0, y0, x1, y1 = ox * S, oy * S, (ox + n * cell) * S, (oy + cell) * S
        dim = lambda a: tuple(int(c * a) for c in col)
        d.rectangle([x0, y0, x1, y1], fill=dim(0.13))                         # 옅은 채움 13%
        for t in range(-cell, n * cell, 8):                                     # 빗금 (45도, 8px 간격)
            ax, bx = max(0, t), min(n * cell, t + cell)
            if ax < bx:
                d.line([(x0 + ax * S, y1 - (ax - t) * S), (x0 + bx * S, y1 - (bx - t) * S)], fill=dim(0.4), width=S)
        glow = Image.new('RGB', img.size, (0, 0, 0))
        ImageDraw.Draw(glow).rectangle([x0, y0, x1, y1], outline=col, width=4 * S)
        img = ImageChops.add(img, layer)
        img = ImageChops.add(img, glow.filter(ImageFilter.GaussianBlur(5 * S)).point(lambda v: int(v * 0.8)))
        dd = ImageDraw.Draw(img)
        dd.rectangle([x0, y0, x1, y1], outline=col, width=max(1, int(1.5 * S)))
        for i in range(n):                                                     # 타워 틀 (회색, 장판 위)
            cx = (ox + i * cell + cell / 2) * S; cy = (oy + cell / 2) * S
            dd.line(rrect(cx, cy, cell * 0.38 * S, cell * 0.38 * 0.22 * S), fill=(150, 160, 180), width=S)
        dd.text(((ox + n * cell / 2) * S, (oy + cell + 22) * S), name, fill=col, font=KFONT(12 * S), anchor='mm')
    img.resize((W, H), Image.LANCZOS).save(out)


def _bloom(img, threshold, strength, radii):
    """밝은 부분만 골라 여러 크기로 흐려 더한다 (언리얼 블룸 흉내)"""
    lum = img.convert('L').point(lambda v: 255 if v > threshold else int(v * v / (threshold * threshold) * 255 * 0.15))
    bright = Image.composite(img, Image.new('RGB', img.size, (0, 0, 0)), lum)
    acc = img.copy()
    for r, w in radii:
        b = bright.filter(ImageFilter.GaussianBlur(r))
        acc = ImageChops.add(acc, b.point(lambda v, k=w * strength: min(255, int(v * k))))
    return acc


def render_bloom(src, out, box=None):
    """게임 화면 캡처로 블룸 미리보기: 현재 / 네온 강화 / 네온 강화 + 블룸"""
    base = Image.open(src).convert('RGB')
    if box:
        base = base.crop(box)
    base = base.resize((base.width * 2, base.height * 2), Image.LANCZOS)
    # 네온 강화: 선 심을 밝게 + 좁은 번짐 한 겹 더 (Slate 번짐 겹을 세게 한 것과 비슷)
    strong = base.point(lambda v: min(255, int(255 * (v / 255) ** 0.8 * 1.1)))
    strong = ImageChops.add(strong, strong.filter(ImageFilter.GaussianBlur(4)).point(lambda v: int(v * 0.45)))
    bloomed = _bloom(strong, 170, 0.6, [(5, 0.45), (14, 0.35), (30, 0.25)])
    W, H = base.size
    pad, top = 16, 40
    img = Image.new('RGB', (W * 3 + pad * 4, H + top + pad), BG)
    d = ImageDraw.Draw(img)
    for i, (im, name) in enumerate([(base, '현재'), (strong, '네온 강화'), (bloomed, '네온 강화 + 블룸')]):
        x = pad + i * (W + pad)
        img.paste(im, (x, top))
        d.text((x + W / 2, top / 2), name, fill=(207, 216, 230), font=KFONT(16), anchor='mm')
    img.save(out)


# ---------------------------------------------------------------- 나이아가라 이펙트 모양 미리보기 (D-111): 효과마다 다른 모양, 절반쯤 지난 순간
def render_fxshapes(out, S=3):
    import random
    rnd = random.Random(7)
    cell, cols = 230, 4
    names = [('Hit', '맞음 (그대로)', 'white'), ('Death', '죽음: 섬광 + 충격파 고리 + 파편', 'pink'),
             ('ImpSpark', '방향 불꽃: 가늘고 긴 줄무늬 원뿔', 'spade'), ('ImpBlast', '폭발: 큰 섬광 + 굵은 불똥', 'heart'),
             ('ImpFrost', '서리: 천천히 퍼지는 점 고리', 'club'), ('ImpZap', '전기: 지그재그 번개 가닥', 'pink2'),
             ('ImpGlitter', '반짝이: 네 갈래 반짝임이 내려앉음', 'gold'), ('ImpStarburst', '별빛: 곧게 뻗는 긴 빛살', 'pink'),
             ('Muzzle', '총구: 짧은 줄무늬 셋', 'spade'), ('Crit', '치명타: 십자 빛살 + 작은 고리', 'white'),
             ('Splash', '범위: 고르게 퍼지는 점 고리', 'diamond'), ('Place', '놓기: 작은 고리 + 위로 오르는 불티', 'club'),
             ('Synergy', '족보: 고리 + 솟는 줄무늬', 'pink2'), ('Fuse', '합성: 섬광 + 고리 + 휘는 꼬리', 'gold'),
             ('JokerFuse', '조커: 큰 섬광 + 이중 고리 + 빛살', 'pink'), ('Leak', '새어 나감: 붉은 고리 + 오르는 연기', 'red')]
    COLX = dict(COL, pink2=(180, 140, 255))
    W, H = cell * cols, (cell + 30) * ((len(names) + cols - 1) // cols)
    img = Image.new('RGB', (W * S, H * S), BG)
    for k, (key, label, col) in enumerate(names):
        cx, cy = (k % cols) * cell + cell / 2, (k // cols) * (cell + 30) + cell / 2
        c = COLX[col]
        layer = Image.new('RGB', img.size, (0, 0, 0))
        d = ImageDraw.Draw(layer)
        P = lambda x, y: ((cx + x) * S, (cy + y) * S)
        def dot(x, y, r=1.6): d.ellipse([P(x - r, y - r), P(x + r, y + r)], fill=c)
        def line(pts, w=1.4): d.line([P(*q) for q in pts], fill=c, width=max(1, int(w * S)))
        def ring(r, n=0, w=1.4):
            if n: [dot(math.cos(a) * r, math.sin(a) * r, 1.8) for a in [i / n * 2 * math.pi for i in range(n)]]
            else: line([(math.cos(a) * r, math.sin(a) * r) for a in [i / 48 * 2 * math.pi for i in range(49)]], w)
        def flash(r):
            for rr, a in [(r, 0.18), (r * 0.6, 0.35), (r * 0.3, 0.8)]:
                d.ellipse([P(-rr, -rr), P(rr, rr)], fill=tuple(int(v * a) for v in c))
        def radial(n, r0, r1, jitter=0.3, w=1.4, a0=0.0, spread=2 * math.pi):
            for i in range(n):
                a = a0 + (i / n - 0.5) * spread + rnd.uniform(-jitter, jitter) * spread / n if spread < 6 else a0 + i / n * 2 * math.pi + rnd.uniform(-jitter, jitter)
                line([(math.cos(a) * r0, math.sin(a) * r0), (math.cos(a) * r1, math.sin(a) * r1)], w)
        def sparkle(x, y, s):
            line([(x - s, y), (x + s, y)], 1.1); line([(x, y - s), (x, y + s)], 1.1)
        if key == 'Hit':
            for _ in range(12): a = rnd.uniform(0, 6.28); r = rnd.uniform(8, 34); dot(math.cos(a) * r, math.sin(a) * r, 1.3)
            line(bez((0, 0), (14, -18), (34, -26)), 1.2)
        elif key == 'Death':
            flash(26); ring(62, 28)
            for _ in range(18): a = rnd.uniform(0, 6.28); r = rnd.uniform(20, 80); dot(math.cos(a) * r, math.sin(a) * r, 2.2)
        elif key == 'ImpSpark':
            radial(7, 10, 70, 0.4, 1.2, 0.0, 0.9)
        elif key == 'ImpBlast':
            flash(34)
            for _ in range(16): a = rnd.uniform(0, 6.28); r = rnd.uniform(30, 75); dot(math.cos(a) * r, math.sin(a) * r, 3.0)
            for a in (0.6, 2.6, 4.4): line(bez((0, 0), (math.cos(a) * 40, math.sin(a) * 40 - 20), (math.cos(a) * 70, math.sin(a) * 70)), 1.3)
        elif key == 'ImpFrost':
            ring(40, 22)
            for _ in range(8): dot(rnd.uniform(-30, 30), rnd.uniform(-55, -10), 1.4)
        elif key == 'ImpZap':
            for i in range(5):
                a = i / 5 * 6.28 + rnd.uniform(-0.3, 0.3); pts = [(0, 0)]; r = 0
                while r < 70:
                    r += rnd.uniform(9, 15); aa = a + rnd.uniform(-0.45, 0.45); pts.append((math.cos(aa) * r, math.sin(aa) * r))
                line(pts, 1.3)
        elif key == 'ImpGlitter':
            for _ in range(10): sparkle(rnd.uniform(-55, 55), rnd.uniform(-20, 70), rnd.uniform(3, 7))
        elif key == 'ImpStarburst':
            radial(8, 16, 95, 0.0, 1.6)
        elif key == 'Muzzle':
            radial(3, 6, 34, 0.2, 1.2, 0.0, 0.5)
        elif key == 'Crit':
            for a in (0.79, 2.36, 3.93, 5.5): line([(math.cos(a) * 6, math.sin(a) * 6), (math.cos(a) * 60, math.sin(a) * 60)], 1.6)
            ring(18)
        elif key == 'Splash':
            ring(80, 36)
        elif key == 'Place':
            ring(26, 14)
            for _ in range(10): dot(rnd.uniform(-22, 22), rnd.uniform(-75, -20), 1.5)
        elif key == 'Synergy':
            ring(54, 26)
            for x in (-36, -18, 0, 18, 36): line([(x, -10 - abs(x) * 0.3), (x, -70 - abs(x) * 0.3)], 1.3)
        elif key == 'Fuse':
            flash(30); ring(70, 30)
            for i in range(8):
                a = i / 8 * 6.28; line(bez((0, 0), (math.cos(a + 0.5) * 50, math.sin(a + 0.5) * 50), (math.cos(a + 0.9) * 95, math.sin(a + 0.9) * 95)), 1.3)
        elif key == 'JokerFuse':
            flash(44); ring(70, 30); ring(100, 40); radial(12, 30, 105, 0.0, 1.3)
        elif key == 'Leak':
            ring(50, 24)
            for i in range(6): r = 6 + i * 2; y = -20 - i * 14; x = rnd.uniform(-14, 14); d.ellipse([P(x - r, y - r), P(x + r, y + r)], outline=c, width=max(1, S))
        img = ImageChops.add(img, layer.filter(ImageFilter.GaussianBlur(6 * S)).point(lambda v: int(v * 0.9)))
        img = ImageChops.add(img, layer)
        dd = ImageDraw.Draw(img)
        dd.text((cx * S, (cy + cell / 2 + 6) * S), key, fill=c, font=FONT(12 * S), anchor='mm')
        dd.text((cx * S, (cy + cell / 2 + 22) * S), label, fill=(138, 148, 166), font=KFONT(10 * S), anchor='mm')
    img.resize((W, H), Image.LANCZOS).save(out)


# ---------------------------------------------------------------- 보스 13종 미리보기 (D-112): 기존 3 + 새 10. 모두 가운데 작은 원(보스 표시) + 고유 모양
def render_bosses(out, S=4):
    HEX = lambda h: tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))
    bosses = [('방패 거인', '9fc3e8', '방어력', 'shield'), ('분열 군주', 'ff9ad5', '죽으면 탱크 넷', 'split'), ('재생 왕', 'b6ff6a', '체력 재생', 'regen'),
              ('돌진 기사', 'ffa86a', '4초마다 0.8초 돌진 ×3', 'charger'), ('소환사', 'c49aff', '3초마다 졸개 둘', 'summoner'),
              ('유령', 'cfe4ff', '4초마다 1.2초 무적', 'phantom'), ('군단장', 'ffd96a', '주변 적 빠르고 단단하게', 'warlord'),
              ('분노 거인', 'ff6a6a', '체력이 줄수록 빨라짐', 'berserker'), ('공간 도약자', '7fd6ff', '5초마다 앞으로 순간이동', 'blinker'),
              ('흡수자', 'e07aff', '주변 적이 죽으면 회복', 'devourer'), ('수호자', 'a8f0ff', '6초마다 보호막', 'warden'),
              ('저주술사', '9a7aff', '주변 타워 공격력 ↓', 'hexer'), ('파괴자', 'ffcf5a', '6초마다 가까운 타워 기절', 'breaker')]
    cell, cols = 200, 5
    rows = (len(bosses) + cols - 1) // cols
    W, H = cell * cols, (cell + 34) * rows
    img = Image.new('RGB', (W * S, H * S), BG)
    for k, (name, col, ability, key) in enumerate(bosses):
        cx, cy = (k % cols) * cell + cell / 2, (k // cols) * (cell + 34) + cell / 2 - 6
        c = HEX(col)
        R = 44.0  # 판 위 반지름 22 의 2배로 그림
        lines = []
        P = lambda x, y: ((cx + x) * S, (cy + y) * S)
        poly = lambda n, r, rot=0.0: [P(math.cos(rot + i / n * 2 * math.pi) * r, math.sin(rot + i / n * 2 * math.pi) * r) for i in range(n + 1)]
        circ = lambda x, y, r, n=40: [P(x + math.cos(i / n * 2 * math.pi) * r, y + math.sin(i / n * 2 * math.pi) * r) for i in range(n + 1)]
        core = circ(0, 0, 24)
        if key == 'shield': lines += [poly(4, R, 0.3), core]
        elif key == 'split': lines += [poly(8, R, 0.2), core]
        elif key == 'regen': lines += [poly(6, R, 0.1), core]
        elif key == 'charger':
            lines += [[P(R + 6, 0), P(-R * 0.6, -R * 0.8), P(-R * 0.2, 0), P(-R * 0.6, R * 0.8), P(R + 6, 0)], circ(4, 0, 16)]
            lines += [[P(-R - 4, -14), P(-R - 26, -14)], [P(-R - 8, 0), P(-R - 34, 0)], [P(-R - 4, 14), P(-R - 26, 14)]]
        elif key == 'summoner':
            lines += [circ(0, 0, 30), circ(0, 0, 14)] + [circ(math.cos(a) * 44, math.sin(a) * 44, 8) for a in (0.4, 2.5, 4.6)]
        elif key == 'phantom':
            body = [P(-30, 30)] + [P(math.cos(math.pi + i / 20 * math.pi) * 30, -6 + math.sin(math.pi + i / 20 * math.pi) * 36) for i in range(21)]
            body += [P(30, 30), P(20, 22), P(10, 30), P(0, 22), P(-10, 30), P(-20, 22), P(-30, 30)]
            lines += [body, circ(-11, -8, 4, 16), circ(11, -8, 4, 16)]
        elif key == 'warlord':
            star = [P(math.cos(-math.pi / 2 + i / 10 * 2 * math.pi) * (R + 4 if i % 2 == 0 else R * 0.45), math.sin(-math.pi / 2 + i / 10 * 2 * math.pi) * (R + 4 if i % 2 == 0 else R * 0.45)) for i in range(11)]
            lines += [star, circ(0, 0, 13)]
        elif key == 'berserker':
            lines += [[P(math.cos(i / 24 * 2 * math.pi) * (R if i % 2 == 0 else R * 0.68), math.sin(i / 24 * 2 * math.pi) * (R if i % 2 == 0 else R * 0.68)) for i in range(25)], core]
        elif key == 'blinker':
            dia = lambda ox, s: [P(ox, -s), P(ox + s * 0.7, 0), P(ox, s), P(ox - s * 0.7, 0), P(ox, -s)]
            lines += [dia(10, R), dia(-18, R * 0.75), dia(-40, R * 0.5), circ(10, 0, 12)]
        elif key == 'devourer':
            lines += [[P(math.cos(i / 60 * 5 * math.pi) * (4 + i * 0.68), math.sin(i / 60 * 5 * math.pi) * (4 + i * 0.68)) for i in range(61)], circ(0, 0, R + 2)]
        elif key == 'warden':
            lines += [poly(6, 30, math.pi / 6), circ(0, 0, 12)]
            lines += [circ(0, 0, R + 4, 60)[i:i + 4] for i in range(0, 60, 6)]  # 점선 보호막
        elif key == 'hexer':
            eye = [P(-R, 0)] + [P(-R + i / 20 * 2 * R, -math.sin(i / 20 * math.pi) * 26) for i in range(21)] + [P(R - i / 20 * 2 * R, math.sin(i / 20 * math.pi) * 26) for i in range(1, 21)]
            lines += [eye, circ(0, 0, 13), circ(0, 0, 5, 16)]
        elif key == 'breaker':
            lines += [poly(4, R, math.pi / 4), [P(-20, -20), P(20, 20)], [P(20, -20), P(-20, 20)]]
            lines += [[P(math.cos(a) * (R - 2), math.sin(a) * (R - 2)), P(math.cos(a) * (R + 14), math.sin(a) * (R + 14))] for a in (math.pi / 4, 3 * math.pi / 4, 5 * math.pi / 4, 7 * math.pi / 4)]
        glow = Image.new('RGB', img.size, (0, 0, 0)); gd = ImageDraw.Draw(glow)
        for ln in lines: gd.line(ln, fill=c, width=int(2.4 * S * 3))
        img = ImageChops.add(img, glow.filter(ImageFilter.GaussianBlur(6 * S)).point(lambda v: int(v * 0.8)))
        d = ImageDraw.Draw(img)
        for ln in lines: d.line(ln, fill=c, width=int(2.4 * S))
        d.text((cx * S, (cy + 72) * S), f'{(k + 1) * 5}웨이브 · {name}', fill=c, font=KFONT(12 * S), anchor='mm')
        d.text((cx * S, (cy + 90) * S), ability, fill=(138, 148, 166), font=KFONT(10 * S), anchor='mm')
    img.resize((W, H), Image.LANCZOS).save(out)


# ---------------------------------------------------------------- UI 개선 시안 (D-119): 1280×720 한 화면
def render_ui_mock(out, S=2):
    W, H = 1280, 720
    img = Image.new('RGB', (W * S, H * S), BG)
    glow = Image.new('RGB', img.size, (0, 0, 0))
    gd = ImageDraw.Draw(glow)
    HEX = lambda h: tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))
    CY, GOLD, RED, GRN, PNK, DIM, FG, GRY = HEX('7fd6ff'), HEX('ffd96a'), HEX('ff6a6a'), HEX('9cffb0'), HEX('ff9ad5'), HEX('5f6878'), HEX('cfd8e6'), HEX('8a94a6')
    lines, texts, fills = [], [], []

    def rr(x, y, w, h, r, c, wd=1.6):
        P = []
        for (cx, cy, a0) in ((x + w - r, y + r, -90), (x + w - r, y + h - r, 0), (x + r, y + h - r, 90), (x + r, y + r, 180)):
            for k in range(7):
                a = math.radians(a0 + k * 15)
                P.append((cx + math.cos(a) * r, cy + math.sin(a) * r))
        P.append(P[0])
        lines.append((P, c, wd))

    def ln(pts, c, wd=1.6): lines.append((pts, c, wd))
    def tx(x, y, s, c, px=12, anchor='lm'): texts.append((x, y, s, c, px, anchor))
    def box(x, y, w, h, c, a): fills.append((x, y, w, h, c, a))

    # 판 (가운데)
    FX, FY, CS = 262, 74, 48
    for i in range(17): ln([(FX + i * CS, FY), (FX + i * CS, FY + 10 * CS)], (24, 30, 44), 1)
    for j in range(11): ln([(FX, FY + j * CS), (FX + 16 * CS, FY + j * CS)], (24, 30, 44), 1)
    path = [(0, 2), (5, 2), (5, 6), (10, 6), (10, 1), (15, 1), (15, 8)]
    ln([(FX + x * CS + CS / 2, FY + y * CS + CS / 2) for x, y in path], (40, 70, 90), 10)
    rr(FX, FY, 16 * CS, 10 * CS, 8, CY, 1.4)
    for (x, y, col, s) in ((7, 3, GOLD, '7'), (8, 3, GOLD, '8'), (9, 3, GOLD, '9'), (7, 4, HEX('7fb8ff'), '2'), (11, 4, PNK, 'J')):
        rr(FX + x * CS + 4, FY + y * CS + 4, CS - 8, CS - 8, 5, col, 1.6)
        tx(FX + x * CS + CS / 2, FY + y * CS + CS / 2, s, col, 18, 'mm')
    # 위쪽 HUD 띠
    rr(16, 12, W - 32, 50, 10, CY, 1.2)
    tx(34, 30, 'WAVE 12', CY, 22)
    tx(34, 50, '다음 보스  20웨이브 · 돌진 기사', HEX('ffa86a'), 10)
    tx(330, 28, '남은 적 23 / 64', FG, 11)
    rr(330, 40, 420, 10, 5, CY, 1.2); box(330, 40, 420 * 0.64, 10, CY, 0.35)
    tx(820, 36, '목숨 18', RED, 17); tx(930, 36, '골드 1,240', GOLD, 17)
    tx(1060, 26, '판 위력', GRY, 10); tx(1060, 46, '3.4k', GRN, 20); tx(1124, 46, '+12%', GRN, 11)
    rr(1184, 22, 30, 30, 8, CY, 1.2); tx(1199, 37, 'x2', CY, 11, 'mm')
    rr(1222, 22, 30, 30, 8, GRY, 1.2); tx(1237, 37, '=', GRY, 14, 'mm')
    # 왼쪽 족보 판 (간결)
    rr(16, 74, 232, 480, 10, CY, 1.2); tx(30, 94, '족보', CY, 15); tx(234, 94, '판 위', DIM, 9, 'rm')
    rows = [('로열', HEX('fff4d6'), '전부 x2.5', 0), ('파이브 카드', GOLD, '공격 x5', 0), ('스트레이트 플러시', PNK, '혜성', 0), ('포카드', RED, '공격 x3.5', 0),
            ('풀하우스', HEX('ffa86a'), '범위 · 연사', 0), ('플러시', HEX('b48cff'), '범위 +', 1), ('스트레이트', HEX('6a8cff'), '연사 +', 2), ('트리플', GRN, '공격 x2', 0),
            ('투 페어', CY, '사거리 +', 0), ('원 페어', FG, '공격 x1.5', 1)]
    for k, (n, c, e, cnt) in enumerate(rows):
        y = 122 + k * 42
        if cnt: box(24, y - 15, 216, 30, c, 0.08)
        ln([(34, y - 7), (34, y + 7)], c, 3)
        tx(46, y - 4, n, c if cnt else GRY, 12); tx(46, y + 10, e, DIM, 9)
        if cnt: rr(206, y - 9, 26, 18, 9, c, 1.2); tx(219, y, 'x%d' % cnt, c, 10, 'mm')
    # 오른쪽 패널 (폭 220)
    RX, PW = 1044, 220
    rr(RX, 74, PW, 186, 10, GRN, 1.2); tx(RX + 12, 92, '판 위력', GRN, 13); tx(RX + PW - 10, 92, '초당 피해', DIM, 9, 'rm')
    tx(RX + 12, 122, '3,412', GRN, 26); tx(RX + 124, 116, '족보 덤 x1.8', FG, 10); tx(RX + 124, 132, '연쇄 x1.25', FG, 10)
    for k, (n, v, c, s) in enumerate((('지휘봉 Lv12', 0.95, PNK, '1.4k'), ('플러시 7 8 9', 0.62, HEX('b48cff'), '880'), ('다이아 9 Lv3', 0.4, GOLD, '560'))):
        y = 158 + k * 32
        tx(RX + 12, y, n, c, 10); tx(RX + PW - 10, y, s, c, 10, 'rm')
        rr(RX + 12, y + 9, PW - 24, 8, 4, c, 1.0); box(RX + 12, y + 9, (PW - 24) * v, 8, c, 0.35)
    rr(RX, 272, PW, 48, 12, CY, 2.0); box(RX, 272, PW, 48, CY, 0.08); tx(RX + PW / 2, 291, '웨이브 13 시작', CY, 15, 'mm'); tx(RX + PW / 2, 309, 'Space', DIM, 9, 'mm')
    for k, (n, key, c) in enumerate((('트리', 'K', HEX('b48cff')), ('강화', 'U', GOLD), ('도감', 'J', PNK), ('메뉴', 'Esc', GRY))):
        x, y = RX + (k % 2) * 113, 332 + (k // 2) * 44
        rr(x, y, 107, 36, 10, c, 1.4); tx(x + 14, y + 18, n, c, 13); tx(x + 96, y + 18, key, DIM, 9, 'rm')
    rr(RX, 428, PW, 92, 10, GOLD, 1.0); tx(RX + 12, 446, '부적', GOLD, 12); tx(RX + PW - 10, 446, '다음 15웨이브', DIM, 9, 'rm')
    for k in range(5):
        cx = RX + 26 + k * 42
        if k < 2: rr(cx - 16, 466, 32, 32, 16, GOLD, 1.6); tx(cx, 482, ['H', '7'][k], GOLD, 12, 'mm')
        else: rr(cx - 16, 466, 32, 32, 16, DIM, 1.0)
    tx(RX + 12, 510, '하트 애호가 · 행운의 7', GRY, 9)
    # 아래: 손패 (크게) + 덱
    for k, (r, c, s) in enumerate((('K', HEX('7fb8ff'), 'S'), ('Q', RED, 'H'), ('7', GOLD, 'D'), ('3', HEX('8affc4'), 'C'), ('A', HEX('7fb8ff'), 'S'))):
        x = 470 + k * 84
        y = 572 if k != 1 else 560
        rr(x, y, 70, 98, 10, c, 2.0 if k == 1 else 1.4); tx(x + 35, y + 38, r, c, 24, 'mm')
        sym = {'S': suit('spade'), 'H': suit('heart'), 'D': suit('diamond'), 'C': suit('club')}[s]
        for pts in sym(x + 35, y + 72, 9): ln(pts, c, 1.4)
    tx(330, 600, '손패 5/7', FG, 12); tx(330, 620, '덱 34장', GRY, 10)
    tx(W - 20, 702, '?  도움말 (H)', DIM, 10, 'rm')
    # 타워 정보 (위력 포함) - 지휘봉에 마우스
    bx, by = FX + 12 * CS + 8, FY + 3 * CS + 10
    box(bx, by, 196, 92, (6, 6, 11), 1.0); rr(bx, by, 196, 92, 8, PNK, 1.4)
    tx(bx + 12, by + 16, '지휘봉 · Lv12', PNK, 12); tx(bx + 12, by + 36, '위력 1.4k/초 (판의 41%)', GRN, 11)
    tx(bx + 12, by + 54, '공격 128 · 연사 2.4 · 사거리 3.2', FG, 10); tx(bx + 12, by + 72, '주변 타워 연사 +40%', DIM, 10)
    # 그리기
    base = ImageDraw.Draw(img)
    for (x, y, w, h, c, a) in fills: base.rectangle([x * S, y * S, (x + w) * S, (y + h) * S], fill=tuple(int(v * a) for v in c))
    for (pts, c, wd) in lines: gd.line([(x * S, y * S) for x, y in pts], fill=c, width=max(1, int(wd * S * 3)))
    img = ImageChops.add(img, glow.filter(ImageFilter.GaussianBlur(5 * S)).point(lambda v: int(v * 0.6)))
    d = ImageDraw.Draw(img)
    for (pts, c, wd) in lines: d.line([(x * S, y * S) for x, y in pts], fill=c, width=max(1, int(wd * S)))
    for (x, y, s, c, px, anchor) in texts:
        if s: d.text((x * S, y * S), s, fill=c, font=KFONT(px * S), anchor=anchor)
    img.resize((W, H), Image.LANCZOS).save(out)


# ---------------------------------------------------------------- 도감·트리 화면 시안 (D-119)
class _Mock:
    """시안용 그리기 도우미: 선(빛번짐), 글자, 옅은 채움을 모았다가 한 번에 그린다"""
    def __init__(self, S=2, W=1280, H=720):
        self.S, self.W, self.H = S, W, H
        self.lines, self.texts, self.fills = [], [], []

    @staticmethod
    def hx(h): return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))

    def rr(self, x, y, w, h, r, c, wd=1.6):
        P = []
        for (cx, cy, a0) in ((x + w - r, y + r, -90), (x + w - r, y + h - r, 0), (x + r, y + h - r, 90), (x + r, y + r, 180)):
            for k in range(7):
                a = math.radians(a0 + k * 15)
                P.append((cx + math.cos(a) * r, cy + math.sin(a) * r))
        P.append(P[0])
        self.lines.append((P, c, wd))

    def ln(self, pts, c, wd=1.6): self.lines.append((pts, c, wd))
    def tx(self, x, y, s, c, px=12, anchor='lm'): self.texts.append((x, y, s, c, px, anchor))
    def box(self, x, y, w, h, c, a): self.fills.append((x, y, w, h, c, a))

    def draw(self, img=None):
        S = self.S
        if img is None: img = Image.new('RGB', (self.W * S, self.H * S), BG)
        base = ImageDraw.Draw(img)
        for (x, y, w, h, c, a) in self.fills: base.rectangle([x * S, y * S, (x + w) * S, (y + h) * S], fill=tuple(int(v * a) for v in c))
        glow = Image.new('RGB', img.size, (0, 0, 0))
        gd = ImageDraw.Draw(glow)
        for (pts, c, wd) in self.lines: gd.line([(x * S, y * S) for x, y in pts], fill=c, width=max(1, int(wd * S * 3)))
        img = ImageChops.add(img, glow.filter(ImageFilter.GaussianBlur(5 * S)).point(lambda v: int(v * 0.6)))
        d = ImageDraw.Draw(img)
        for (pts, c, wd) in self.lines: d.line([(x * S, y * S) for x, y in pts], fill=c, width=max(1, int(wd * S)))
        for (x, y, s, c, px, anchor) in self.texts:
            if s: d.text((x * S, y * S), s, fill=c, font=KFONT(px * S), anchor=anchor)
        return img


_HAND_COL = {'로열': 'fff4d6', '파이브': 'ffd96a', '스티플': 'ff9ad5', '포카드': 'ff6a6a', '풀하우스': 'ffa86a', '플러시': 'b48cff',
             '스트레이트': '6a8cff', '트리플': '9cffb0', '투 페어': '7fd6ff', '원 페어': 'cfd8e6'}
_JOKERS = [  # 이름, 등급, 능력, 3개 레시피, 2개 레시피 (D-118)
    ('광대', 3.0, '무늬 4종 공격을 번갈아', ['파이브', '스트레이트', '플러시'], ['파이브', '스트레이트']),
    ('모자 장수', 3.0, '주변 타워 공격 +25%', ['스티플', '스트레이트', '플러시'], ['스티플', '플러시']),
    ('웃는 가면', 2.0, '옆 타워 공격을 따라 함', ['투 페어', '원 페어', '스트레이트'], ['투 페어', '원 페어']),
    ('희비극', 2.0, '두 적을 동시에', ['원 페어', '스트레이트', '플러시'], ['원 페어', '스트레이트']),
    ('할리퀸', 2.5, '같은 무늬 타워 +30%', ['플러시', '플러시', '플러시'], ['플러시', '플러시']),
    ('지휘봉', 2.0, '주변 타워 연사 +40%', ['스트레이트', '스트레이트', '스트레이트'], ['스트레이트', '스트레이트']),
    ('잭', 2.0, '공격 ×3, 연사 ×0.5', ['트리플', '스트레이트', '플러시'], ['트리플', '스트레이트']),
    ('체셔', 2.5, '사거리 무한', ['포카드', '스트레이트', '플러시'], ['포카드', '스트레이트']),
    ('방울', 2.5, '8초마다 적을 멈춤', ['풀하우스', '스트레이트', '플러시'], ['풀하우스', '플러시']),
    ('스타', 3.0, '1초마다 모든 적에게', ['로열', '스티플', '플러시'], ['로열', '스티플'])]


_HAND_SHORT = {'스트레이트': '스트', '원 페어': '페어', '투 페어': '투페어', '풀하우스': '풀하'}
def _chips(M, x, y, hands, px=9, h=16, gap=4, short=False):
    """족보 이름을 색 알약으로 나란히. 끝 x 를 돌려준다 (short: 작은 칸용 줄임말)"""
    for full in hands:
        c = M.hx(_HAND_COL[full])
        n = _HAND_SHORT.get(full, full) if short else full
        w = 12 + len(n.replace(' ', '')) * px * 1.36 + n.count(' ') * px * 0.5
        M.rr(x, y, w, h, h / 2, c, 1.1)
        M.box(x, y, w, h, c, 0.1)
        M.tx(x + w / 2, y + h / 2, n, c, px, 'mm')
        x += w + gap
    return x


def render_codex_mock(out, S=2):
    M = _Mock(S)
    PNK, GOLD, DIM, FG, GRY, CY = M.hx('ff9ad5'), M.hx('ffd96a'), M.hx('5f6878'), M.hx('cfd8e6'), M.hx('8a94a6'), M.hx('7fd6ff')
    found = {0, 2, 3, 4, 5, 6, 8}
    sel = 5
    M.rr(16, 12, 1248, 50, 10, PNK, 1.2)
    M.tx(34, 37, '조커 도감', PNK, 20); M.tx(170, 37, '7 / 10 발견', FG, 13)
    M.tx(420, 37, '맞닿은 족보 3개(또는 2개)가 레시피와 맞으면 조커. 3개 쪽이 카드가 많아 세다', GRY, 11)
    M.rr(1180, 22, 70, 30, 10, GRY, 1.2); M.tx(1215, 37, '닫기 J', GRY, 11, 'mm')
    designs = joker_cards()
    S2 = S
    icon_jobs = []
    for k, (name, tier, abil, r3, r2) in enumerate(_JOKERS):
        col, row = k % 5, k // 5
        x, y = 16 + col * 172, 76 + row * 316
        on = k in found
        c = PNK if on else DIM
        if k == sel: M.box(x, y, 164, 304, PNK, 0.07)
        M.rr(x, y, 164, 304, 10, c, 2.0 if k == sel else 1.2)
        icon_jobs.append((designs[k][0], x + 82, y + 70, 40, on))
        M.tx(x + 82, y + 132, name if on else '???', c if on else GRY, 15, 'mm')
        tc = M.hx('ffd96a') if tier >= 3 else M.hx('ffa86a') if tier >= 2.5 else CY
        M.rr(x + 52, y + 146, 60, 18, 9, tc, 1.1); M.tx(x + 82, y + 155, '위력 ×%g' % tier, tc, 9, 'mm')
        M.tx(x + 82, y + 180, abil if on else '만들면 열림', FG if on else DIM, 10, 'mm')
        M.tx(x + 10, y + 206, '3개', GRY, 9); _chips(M, x + 10, y + 216, r3, 9, 17, 4, True)
        M.tx(x + 10, y + 250, '2개', GRY, 9); _chips(M, x + 10, y + 260, r2, 9, 17, 4, True)
    # 오른쪽 자세히
    X0 = 884
    name, tier, abil, r3, r2 = _JOKERS[sel]
    M.rr(X0, 76, 380, 620, 12, PNK, 1.6)
    icon_jobs.append((designs[sel][0], X0 + 190, 170, 70, True))
    M.tx(X0 + 190, 270, name, PNK, 24, 'mm')
    M.tx(X0 + 190, 300, '스트레이트 대장 · 위력 ×%g' % tier, GOLD, 12, 'mm')
    M.tx(X0 + 24, 340, '능력', GRY, 10); M.tx(X0 + 24, 360, abil, FG, 13)
    M.tx(X0 + 24, 388, '지휘: 스트레이트 타워 위력 +30% (겹침)', FG, 11)
    M.tx(X0 + 24, 430, '3개 레시피', GRY, 10); _chips(M, X0 + 24, 442, r3, 10, 22, 6)
    M.tx(X0 + 24, 486, '2개 레시피', GRY, 10); _chips(M, X0 + 24, 498, r2, 10, 22, 6)
    M.tx(X0 + 24, 548, '스트레이트끼리는 숫자가 이어지지 않게', DIM, 10)
    M.rr(X0 + 24, 572, 332, 100, 10, M.hx('9cffb0'), 1.1)
    M.tx(X0 + 40, 594, '위력 계산', M.hx('9cffb0'), 11)
    M.tx(X0 + 40, 616, '재료 위력 합 × 1.2 × 등급 ×2', FG, 11)
    M.tx(X0 + 40, 636, '× 카드 수 덤 (한 장마다 +10%)', FG, 11)
    M.tx(X0 + 40, 656, '3개 레시피(6장)는 2개(4장)보다 약 15% 셈', DIM, 10)
    img = M.draw()
    # 조커 그림 (켜진 것만 분홍, 꺼진 건 어둡게)
    for fn, cx, cy, R, on in icon_jobs:
        col = (255, 154, 213) if on else (70, 76, 92)
        glow = Image.new('RGB', img.size, (0, 0, 0)); gd = ImageDraw.Draw(glow)
        L = fn(cx * S2, cy * S2, R * S2)
        for l in L:
            if l and l[0] == 'text': continue
            gd.line(l, fill=col, width=int(3 * S2 * 2))
        img = ImageChops.add(img, glow.filter(ImageFilter.GaussianBlur(5 * S2)).point(lambda v: int(v * 0.7)))
        d = ImageDraw.Draw(img)
        for l in L:
            if l and l[0] == 'text': continue
            d.line(l, fill=col, width=max(1, int(1.6 * S2)))
    img.resize((1280, 720), Image.LANCZOS).save(out)


def render_tree_mock(out, path, S=2):
    import json, tempfile, os as _os
    tmp = _os.path.join(tempfile.gettempdir(), 'pd_tree_base.png')
    render_tree(tmp, path, S=S)
    base = Image.open(tmp).convert('RGB').resize((1280 * S, 720 * S))
    # 기존 왼쪽 위 글자 지우기
    ImageDraw.Draw(base).rectangle([0, 0, 400 * S, 70 * S], fill=BG)
    nodes = json.load(open(path, encoding='utf-8'))
    M = _Mock(S)
    GOLD, DIM, FG, GRY, CY, GRN, RED, PRP = M.hx('ffd96a'), M.hx('5f6878'), M.hx('cfd8e6'), M.hx('8a94a6'), M.hx('7fd6ff'), M.hx('9cffb0'), M.hx('ff6a6a'), M.hx('b48cff')
    M.box(0, 0, 1280, 64, BG, 1.0)
    M.rr(16, 12, 1248, 46, 10, PRP, 1.2)
    M.tx(34, 35, '스킬 트리', PRP, 20)
    M.tx(170, 35, '포인트 2', GRN, 15); M.tx(270, 35, '다음 포인트 1,240 골드', GOLD, 12)
    M.tx(470, 35, '키스톤 1 / 3', M.hx('ffd96a'), 12); M.tx(590, 35, '찍은 노드 18', FG, 12)
    M.rr(1180, 20, 70, 30, 10, GRY, 1.2); M.tx(1215, 35, '닫기 K', GRY, 11, 'mm')
    # 범례
    M.rr(16, 640, 560, 64, 10, DIM, 1.0)
    for k, (n, c, a) in enumerate((('찍음', CY, 1.0), ('찍을 수 있음', GRN, 1.0), ('아직 못 찍음', DIM, 1.0), ('봉인 (짝 키스톤)', RED, 1.0))):
        x = 34 + k * 130
        M.rr(x, 664, 16, 16, 8, c, 1.6); M.tx(x + 24, 672, n, c, 10)
    M.tx(34, 694, '키스톤은 둘씩 짝: 하나를 찍으면 짝은 이 판 동안 봉인', DIM, 9)
    # 마우스가 올라간 노드 (찍을 수 있는 키스톤 하나를 고름)
    keys = [n for n in nodes if n['k'] == 3]
    hov = keys[2] if len(keys) > 2 else nodes[1]
    hx_, hy_ = hov['x'], hov['y']
    M.rr(hx_ - 24, hy_ - 24, 48, 48, 24, GRN, 2.2)
    bx = hx_ + 34 if hx_ < 900 else hx_ - 34 - 300
    by = min(max(hy_ - 60, 80), 520)
    M.box(bx, by, 300, 132, BG, 1.0); M.rr(bx, by, 300, 132, 10, GRN, 1.6)
    KN = ['외골수', '잡식가', '큰 판', '작은 판', '대장장이', '광대의 길', '고독한 저격수', '군중']
    KD = ['플러시 타워 위력 ×2', '플러시 아닌 족보 위력 ×1.3', '5장 족보 위력 ×1.6', '원 페어·투 페어 위력 ×1.8', '합성 배율 1.5 → 2.0', '조커 배율 1.2 → 1.6', '족보에 안 든 타워 위력 ×1.6', '연쇄 덤 +25% → +40%']
    hk = hov['fx'][0][1]
    M.tx(bx + 14, by + 20, KN[hk], GOLD, 14)
    M.tx(bx + 14, by + 44, '키스톤 · 찍을 수 있음', GRN, 10)
    M.tx(bx + 14, by + 68, KD[hk], FG, 12)
    M.tx(bx + 14, by + 90, '짝: %s (함께 못 찍음)' % KN[hk ^ 1], RED, 10)
    M.rr(bx + 14, by + 102, 120, 22, 8, GRN, 1.2); M.tx(bx + 74, by + 113, '찍기 · 포인트 1', GRN, 10, 'mm')
    img = M.draw(base)
    img.resize((1280, 720), Image.LANCZOS).save(out)

if __name__ == '__main__':
    if len(sys.argv) >= 3 and sys.argv[1] == 'iconsheet':  # python neon.py iconsheet 출력.png (트리 노드 아이콘 확인)
        render_icon_sheet(sys.argv[2]); print('saved', sys.argv[2]); sys.exit(0)
    if len(sys.argv) >= 3 and sys.argv[1] == 'codexmock':  # python neon.py codexmock 출력.png
        render_codex_mock(sys.argv[2]); print('saved', sys.argv[2]); sys.exit(0)
    if len(sys.argv) >= 4 and sys.argv[1] == 'treemock':  # python neon.py treemock 출력.png tools/tree/tree.json
        render_tree_mock(sys.argv[2], sys.argv[3]); print('saved', sys.argv[2]); sys.exit(0)
    if len(sys.argv) >= 3 and sys.argv[1] == 'uimock':  # python neon.py uimock 출력.png
        render_ui_mock(sys.argv[2]); print('saved', sys.argv[2]); sys.exit(0)
    if len(sys.argv) >= 3 and sys.argv[1] == 'bosses':  # python neon.py bosses 출력.png
        render_bosses(sys.argv[2]); print('saved', sys.argv[2]); sys.exit(0)
    if len(sys.argv) >= 3 and sys.argv[1] == 'fxshapes':  # python neon.py fxshapes 출력.png
        render_fxshapes(sys.argv[2]); print('saved', sys.argv[2]); sys.exit(0)
    if len(sys.argv) >= 4 and sys.argv[1] == 'bloom':  # python neon.py bloom 캡처.png 출력.png [x0 y0 x1 y1]
        render_bloom(sys.argv[2], sys.argv[3], tuple(int(v) for v in sys.argv[4:8]) if len(sys.argv) >= 8 else None)
        print('saved', sys.argv[3])
        sys.exit(0)
    if len(sys.argv) >= 4 and sys.argv[1] == 'tree':
        render_tree(sys.argv[2], sys.argv[3])
        print('saved', sys.argv[2])
        sys.exit(0)
    if len(sys.argv) >= 3 and sys.argv[1] == 'floors':
        render_floors(sys.argv[2])
        print('saved', sys.argv[2])
        sys.exit(0)
    if len(sys.argv) < 3 or sys.argv[1] not in SHAPES:
        print(__doc__)
        print('그림:', ', '.join(SHAPES))
        sys.exit(1)
    render(sys.argv[1], sys.argv[2])
    print('saved', sys.argv[2])
