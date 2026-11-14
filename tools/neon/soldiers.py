"""카드병정 적 시안 (네온 화풍, docs/15-art-style.md). 적을 트럼프 카드 몸통의 병정으로.

    python tools/neon/soldiers.py sheet 출력.png   # 종류 8 × 걷기 4프레임
    python tools/neon/soldiers.py gif 출력.gif     # 길을 따라 걷는 움직이는 시안

몸통 = 둥근 카드(가운데 무늬, 왼쪽 위 숫자), 머리 = 동그라미 + 모자, 팔다리 = 선, 무기는 종류마다.
걷기: 다리가 번갈아 앞뒤로(±26도), 팔은 반대로, 몸이 한 걸음마다 살짝 튄다.
"""
import math
import sys

import neon
from PIL import Image

HX = neon._Mock.hx
KINDS = [
    # 이름, 무늬, 색, 몸통 너비·높이 배율, 걸음 빠르기, 무기
    ('보통', 'spade', 'e8f0ff', 1.0, 1.0, 1.0, 'spear'),
    ('빠름', 'diamond', 'ffa86a', 0.85, 0.95, 1.8, 'none'),
    ('탱크', 'club', 'c49aff', 1.35, 1.1, 0.7, 'axe'),
    ('방패병', 'heart', '9fc3e8', 1.0, 1.0, 0.9, 'shield'),
    ('분열체', 'diamond', 'ff9ad5', 1.0, 1.0, 1.1, 'split'),
    ('재생자', 'club', 'b6ff6a', 1.0, 1.0, 0.9, 'regen'),
    ('조각', 'heart', 'ff9ad5', 0.6, 0.6, 1.6, 'none'),
    ('보스: 왕', 'spade', 'ffd96a', 1.35, 1.35, 0.6, 'king'),
]


def rot(p, c, a):
    s, co = math.sin(a), math.cos(a)
    x, y = p[0] - c[0], p[1] - c[1]
    return (c[0] + x * co - y * s, c[1] + x * s + y * co)


def soldier(kind, cx, cy, R, t):
    """선 목록 [(점들, 색, 굵기)]. cy = 발바닥. t = 걷기 위상 0..1"""
    name, st, col, wk, hk, spd, wpn = kind
    c = HX(col)
    L = []
    ph = t * 2 * math.pi
    sw = math.sin(ph)
    bob = abs(math.sin(ph)) * 0.05 * R
    lean = math.radians(8) if name == '빠름' else 0.0
    bw, bh = 0.9 * R * wk, 1.2 * R * hk
    hip = (cx, cy - 0.55 * R * hk - bob)               # 다리 시작
    legL = 0.55 * R * hk
    # 다리 (무릎 없이 곧게, 번갈아)
    for s in (1, -1):
        a = math.radians(26) * sw * s
        foot = (hip[0] + math.sin(a) * legL, hip[1] + math.cos(a) * legL)
        L.append(([(hip[0] - s * 0.12 * bw, hip[1]), foot, (foot[0] + 0.12 * R, foot[1])], c, 1.6))
    # 몸통 카드
    top = hip[1] - bh
    body_c = (cx, top + bh / 2)
    P = []
    r = 0.12 * R
    for (ox, oy, a0) in ((bw / 2 - r, -bh / 2 + r, -90), (bw / 2 - r, bh / 2 - r, 0), (-bw / 2 + r, bh / 2 - r, 90), (-bw / 2 + r, -bh / 2 + r, 180)):
        for k in range(7):
            a = math.radians(a0 + k * 15)
            P.append((body_c[0] + ox + math.cos(a) * r, body_c[1] + oy + math.sin(a) * r))
    P.append(P[0])
    P = [rot(p, hip, lean) for p in P]
    L.append((P, c, 2.4 if name == '탱크' else 1.7))
    if wpn == 'split':  # 가운데 찢어진 금
        zz = [(body_c[0] + (0.08 if k % 2 else -0.08) * bw, top + bh * k / 6) for k in range(7)]
        L.append(([rot(p, hip, lean) for p in zz], c, 1.2))
    sym = neon.suit(st)(body_c[0], body_c[1] + 0.02 * R, 0.26 * R * min(wk, hk))
    for s in sym:
        L.append(([rot(p, hip, lean) for p in s], c, 1.3))
    # 왼쪽 위 숫자 (작은 획): 보스는 K
    # 머리 + 모자
    head = rot((cx, top - 0.26 * R), hip, lean)
    L.append((neon.circle(head[0], head[1], 0.2 * R, 20), c, 1.5))
    if wpn == 'king':  # 왕관
        crown = [(head[0] - 0.22 * R, head[1] - 0.16 * R), (head[0] - 0.22 * R, head[1] - 0.42 * R), (head[0] - 0.11 * R, head[1] - 0.28 * R),
                 (head[0], head[1] - 0.46 * R), (head[0] + 0.11 * R, head[1] - 0.28 * R), (head[0] + 0.22 * R, head[1] - 0.42 * R),
                 (head[0] + 0.22 * R, head[1] - 0.16 * R), (head[0] - 0.22 * R, head[1] - 0.16 * R)]
        L.append((crown, HX('ffd96a'), 1.6))
    else:  # 납작 모자
        L.append(([(head[0] - 0.24 * R, head[1] - 0.14 * R), (head[0] + 0.24 * R, head[1] - 0.14 * R)], c, 1.4))
        L.append(([(head[0] - 0.14 * R, head[1] - 0.14 * R), (head[0] - 0.12 * R, head[1] - 0.3 * R), (head[0] + 0.12 * R, head[1] - 0.3 * R), (head[0] + 0.14 * R, head[1] - 0.14 * R)], c, 1.4))
    # 팔 (어깨에서, 다리와 반대로 흔든다)
    sh_y = top + 0.22 * bh
    for s in (1, -1):
        sh = rot((cx + s * bw / 2, sh_y), hip, lean)
        a = -math.radians(30) * sw * s
        hand = (sh[0] + s * 0.18 * R + math.sin(a) * 0.1 * R, sh[1] + 0.42 * R * math.cos(a * 0.5))
        L.append(([sh, hand], c, 1.5))
        if s == 1:
            rh = hand
        else:
            lh = hand
    # 무기
    if wpn == 'spear':
        tip = (rh[0] + 0.05 * R, rh[1] - 1.25 * R)
        L.append(([(rh[0], rh[1] + 0.35 * R), tip], c, 1.4))
        L.append(([tip, (tip[0] - 0.08 * R, tip[1] + 0.2 * R), (tip[0], tip[1] - 0.12 * R), (tip[0] + 0.08 * R, tip[1] + 0.2 * R), tip], c, 1.4))
    elif wpn == 'axe':
        top_a = (rh[0], rh[1] - 1.0 * R)
        L.append(([(rh[0], rh[1] + 0.25 * R), top_a], c, 1.8))
        L.append(([top_a, (top_a[0] + 0.32 * R, top_a[1] - 0.12 * R), (top_a[0] + 0.36 * R, top_a[1] + 0.22 * R), (top_a[0], top_a[1] + 0.2 * R)], c, 1.8))
    elif wpn == 'shield':
        sc = (cx + 0.55 * bw, body_c[1] + 0.05 * R)
        sp = [(sc[0], sc[1] - 0.42 * R), (sc[0] + 0.32 * R, sc[1] - 0.28 * R), (sc[0] + 0.28 * R, sc[1] + 0.2 * R), (sc[0], sc[1] + 0.45 * R),
              (sc[0] - 0.28 * R, sc[1] + 0.2 * R), (sc[0] - 0.32 * R, sc[1] - 0.28 * R), (sc[0], sc[1] - 0.42 * R)]
        L.append((sp, c, 2.0))
        for s in neon.suit('heart')(sc[0], sc[1], 0.15 * R):
            L.append((s, c, 1.2))
    elif wpn == 'regen':  # 떠오르는 + 둘
        for k in range(2):
            u = (t + k * 0.5) % 1.0
            px, py = cx + (0.45 if k else -0.45) * bw, top - u * 0.6 * R
            a = 0.12 * R
            cc = tuple(int(v * (1 - u)) for v in c)
            L.append(([(px - a, py), (px + a, py)], cc, 1.4))
            L.append(([(px, py - a), (px, py + a)], cc, 1.4))
    elif wpn == 'king':  # 홀 + 망토
        tip = (rh[0], rh[1] - 1.1 * R)
        L.append(([(rh[0], rh[1] + 0.2 * R), tip], c, 1.6))
        L.append((neon.circle(tip[0], tip[1] - 0.1 * R, 0.1 * R, 14), c, 1.6))
        cape = [rot((cx - bw / 2 - 0.05 * R, top + 0.1 * R), hip, lean), (cx - bw / 2 - 0.35 * R - 0.08 * R * sw, hip[1] + 0.15 * R), (cx - bw / 2 - 0.05 * R, hip[1] + 0.05 * R)]
        L.append((cape, c, 1.3))
    elif name == '빠름':  # 뒤로 흩날리는 속도선
        for k in range(3):
            y = top + bh * (0.25 + 0.25 * k)
            x0 = cx - bw / 2 - 0.15 * R
            L.append(([(x0, y), (x0 - (0.3 + 0.15 * ((t * 3 + k) % 1)) * R, y)], tuple(int(v * 0.6) for v in c), 1.2))
    return L


def sheet(out):
    W, H, S = 1280, 920, 2
    M = neon._Mock(S, W, H)
    R = 46
    for i, k in enumerate(KINDS):
        row, colx = i // 2, (i % 2) * 640
        y = 70 + row * 220
        M.tx(colx + 30, y - 40, k[0], HX(k[2]), 16, 'lm')
        for f in range(4):
            x = colx + 120 + f * 130
            for pts, c, wd in soldier(k, x, y + 130, R * (1.0 if k[0] != '조각' else 1.0), f / 4):
                M.ln(pts, c, wd)
        M.tx(colx + 30, y + 150, '걷기 4프레임', (95, 104, 120), 10, 'lm')
    img = Image.new('RGB', (W * S, H * S), neon.BG)
    M.draw(img).resize((W, H), Image.LANCZOS).save(out)


def gif(out):
    W, H, S = 960, 300, 1
    frames = []
    N = 24
    for f in range(N):
        M = neon._Mock(S, W, H)
        # 길
        M.ln([(0, 230), (W, 230)], (40, 70, 90), 1.0)
        for x in range(0, W, 24):
            M.ln([(x, 200), (x + 12, 200)], (60, 110, 140), 1.0)
        for i, k in enumerate(KINDS):
            spd = k[5]
            t = (f / N * spd * 2) % 1.0
            x = (i * 120 + 60 + f / N * 120 * spd) % (W + 80) - 40
            R = 34 if k[0] != '보스: 왕' else 40
            for pts, c, wd in soldier(k, x, 220, R, t):
                M.ln(pts, c, wd)
        img = Image.new('RGB', (W * S, H * S), neon.BG)
        frames.append(M.draw(img).convert('P', palette=Image.ADAPTIVE))
    frames[0].save(out, save_all=True, append_images=frames[1:], duration=70, loop=0)


if __name__ == '__main__':
    {'sheet': sheet, 'gif': gif}[sys.argv[1]](sys.argv[2])
    print('saved', sys.argv[2])
