"""네온 선 화풍 미리보기 (docs/15-art-style.md)

언리얼 빌드 없이 같은 화풍으로 그림을 그려 PNG로 확인한다.
그림은 '선 목록'(점 목록의 목록)을 돌려주는 함수로 만든다. 좌표는 중심 (cx, cy), 크기 R 기준.

    python3 tools/neon/neon.py joker 출력.png      # 조커 광대 머리
    python3 tools/neon/neon.py suits 출력.png      # 카드 무늬 4종 + 별
    python3 tools/neon/neon.py towers 출력.png     # 타워 (틀 + 숫자 + 무늬 기호)
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


def tree_shape(n, cx, cy):
    k = n['k']
    if k == 0:
        return [circle(cx, cy, 13, 32)] + suit('wild')(cx, cy, 8)
    if k == 1:
        return [circle(cx, cy, 6, 24)]
    if k == 2:
        L = [rrect(cx, cy, 13, 4)]
        if n['theme'] < 4:
            L += suit(SUIT_COL[n['theme']])(cx, cy, 7)
        else:
            L.append(circle(cx, cy, 3.5, 16))
        return L
    if k == 3:  # 키스톤: 팔각 이중 틀
        oct_ = lambda r: [(cx + math.cos(math.radians(22.5 + 45 * j)) * r, cy + math.sin(math.radians(22.5 + 45 * j)) * r) for j in range(9)]
        return [oct_(18), oct_(11)]
    return [rrect(cx, cy, 15, 4)] + jester_hat(cx, cy + 4, 11, band=False)


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
            a, b = (n['x'], n['y']), (nodes[m]['x'], nodes[m]['y'])
            col = tree_color(nodes[m] if nodes[m]['k'] != 3 else n) if on else 'cyan'
            jobs.append(([[a, b]], col, 1.0 if on else 0.2, 1.1))
    for n in nodes:
        cx, cy = n['x'], n['y']
        i = n['id']
        if i in sealed:
            jobs.append((tree_shape(n, cx, cy), 'red', 0.5, 1.3))
            jobs.append(([[(cx - 11, cy - 11), (cx + 11, cy + 11)], [(cx + 11, cy - 11), (cx - 11, cy + 11)]], 'red', 0.8, 1.3))
        else:
            jobs.append((tree_shape(n, cx, cy), tree_color(n), 1.0 if i in owned else 0.4, 1.5 if n['k'] >= 2 else 1.2))
    for lines, col, alpha, w in jobs:
        c = tuple(int(v * alpha) for v in COL[col])
        glow = Image.new('RGB', img.size, (0, 0, 0))
        gd = ImageDraw.Draw(glow)
        sl = [[(x * S, y * S) for x, y in ln] for ln in lines]
        for ln in sl:
            gd.line(ln, fill=c, width=int(w * 4 * S))
        img = ImageChops.add(img, glow.filter(ImageFilter.GaussianBlur(5 * S)).point(lambda v: int(v * 0.8)))
        d = ImageDraw.Draw(img)
        for ln in sl:
            d.line(ln, fill=c, width=max(1, int(w * S)))
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


if __name__ == '__main__':
    if len(sys.argv) >= 4 and sys.argv[1] == 'tree':
        render_tree(sys.argv[2], sys.argv[3])
        print('saved', sys.argv[2])
        sys.exit(0)
    if len(sys.argv) < 3 or sys.argv[1] not in SHAPES:
        print(__doc__)
        print('그림:', ', '.join(SHAPES))
        sys.exit(1)
    render(sys.argv[1], sys.argv[2])
    print('saved', sys.argv[2])
