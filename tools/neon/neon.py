"""네온 선 화풍 미리보기 (docs/15-art-style.md)

언리얼 빌드 없이 같은 화풍으로 그림을 그려 PNG로 확인한다.
그림은 '선 목록'(점 목록의 목록)을 돌려주는 함수로 만든다. 좌표는 중심 (cx, cy), 크기 R 기준.

    python3 tools/neon/neon.py joker 출력.png      # 조커 광대 머리
    python3 tools/neon/neon.py suits 출력.png      # 카드 무늬 4종 + 별
    python3 tools/neon/neon.py towers 출력.png     # 타워 (틀 + 숫자 + 무늬 기호)

새 그림은 아래 SHAPES 에 함수를 더하면 된다. 큰 크기 / 손패 크기 / 타워 크기 세 가지로 나란히 그린다.
"""
import math
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

BG = (6, 6, 11)
FONT = lambda px: ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf', max(6, int(px * 1.33)))
COL = {'spade': (127, 184, 255), 'heart': (255, 106, 138), 'diamond': (255, 217, 106), 'club': (138, 255, 196),
       'pink': (255, 154, 213), 'white': (255, 255, 255), 'cyan': (127, 214, 255)}


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


SHAPES = {
    'towers': [(tower('spade', '9'), 'spade'), (tower('heart', 'A'), 'heart'), (tower('diamond', '10'), 'diamond'), (tower('club', 'K'), 'club'), (tower('wild', ''), 'white')],
    'joker': [(joker, 'pink')],
    'suits': [(suit('spade'), 'spade'), (suit('heart'), 'heart'), (suit('diamond'), 'diamond'), (suit('club'), 'club'), (suit('wild'), 'white')],
}


# ---------------------------------------------------------------- 그리기 (3겹 빛번짐 흉내: 흐린 번짐 + 심)
def render(name, out, S=4):
    items = SHAPES[name]
    if len(items) == 1:  # 그림 하나: 큰 그림 / 손패 크기 / 타워 크기
        W, H = 900, 320
        jobs = [(items[0][0], items[0][1], x, H // 2, R) for x, R in [(200, 110), (560, 55), (800, 24)]]
    else:                # 여러 그림: 손패 크기로 나란히
        W, H = 180 * len(items), 220
        jobs = [(fn, col, 90 + k * 180, H // 2, 60) for k, (fn, col) in enumerate(items)]
    img = Image.new('RGB', (W * S, H * S), BG)
    for fn, col, cx, cy, R in jobs:
        c = COL[col]
        core = max(1, round(1.4 * R / 60)) * S   # 심 굵기: 가늘고 또렷하게 (기준 화면 px × S)
        glow = Image.new('RGB', img.size, (0, 0, 0))
        gd = ImageDraw.Draw(glow)
        lines = fn(cx * S, cy * S, R * S)
        for ln in lines:
            if ln and ln[0] == 'text':
                gd.text((ln[1], ln[2]), ln[3], fill=c, font=FONT(ln[4]), anchor='mm')
            else:
                gd.line(ln, fill=c, width=core * 4)
        img = ImageChops.add(img, glow.filter(ImageFilter.GaussianBlur(7 * S)), scale=1.1)  # 넓고 부드러운 번짐
        d = ImageDraw.Draw(img)
        for ln in lines:
            if ln and ln[0] == 'text':
                d.text((ln[1], ln[2]), ln[3], fill=c, font=FONT(ln[4]), anchor='mm')
            else:
                d.line(ln, fill=c, width=core)
    img.resize((W, H), Image.LANCZOS).save(out)


if __name__ == '__main__':
    if len(sys.argv) < 3 or sys.argv[1] not in SHAPES:
        print(__doc__)
        print('그림:', ', '.join(SHAPES))
        sys.exit(1)
    render(sys.argv[1], sys.argv[2])
    print('saved', sys.argv[2])
