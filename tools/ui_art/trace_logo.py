"""시작 화면 간판(docs/images/ui-1120/title_logo_ref.webp)을 코드 선으로 따라 그리기 (D-429)

    python tools/ui_art/trace_logo.py   → tools/ui_art/title_logo_strokes.json + docs/images/ui-1120/title_logo_trace.png (원본 | 코드 네온)

1. 색별로 네온 관 몸통만 고른다 (하늘 = BAR, 분홍 = Alice, 노랑 = 마티니): 밝고 그 색상에 가까운 화소
2. Zhang-Suen 깎기로 관의 가운데 선(1화소)을 얻는다
3. 끝점 · 갈림점 사이를 따라가 꺾은선으로 만들고, 짧은 가지는 버리고, Ramer-Douglas-Peucker 로 점을 줄인다
4. 같은 굵기 · 색으로 네온을 다시 그려 원본과 나란히 비교
"""
import json
import math
import os

import numpy as np
from PIL import Image, ImageChops, ImageDraw, ImageFilter

ROOT = os.path.join(os.path.dirname(__file__), '..', '..')
SRC = os.path.join(ROOT, 'docs', 'images', 'ui-1120', 'title_logo_ref.webp')
OUTJ = os.path.join(os.path.dirname(__file__), 'title_logo_strokes.json')
OUTP = os.path.join(ROOT, 'docs', 'images', 'ui-1120', 'title_logo_trace.png')

COLORS = {'cyan': (60, 235, 240), 'pink': (255, 90, 225), 'yellow': (255, 235, 40)}


def shift(m, dy, dx):
    out = np.zeros_like(m)
    H, W = m.shape
    out[max(0, dy):H + min(0, dy), max(0, dx):W + min(0, dx)] = m[max(0, -dy):H + min(0, -dy), max(0, -dx):W + min(0, -dx)]
    return out


def dilate(m, r):
    out = m.copy()
    for dy in range(-r, r + 1):
        for dx in range(-r, r + 1):
            if dy * dy + dx * dx <= r * r: out |= shift(m, dy, dx)
    return out


def erode(m, r): return ~dilate(~m, r)


def masks(img):
    """관 몸통(밝은 화소)을 색별로. 가운데 흰 심은 색이 약해서, 흐리게 한 그림의 색으로 어느 관인지 정한다. 작은 구멍은 메운다"""
    a = np.asarray(img).astype(np.float32) / 255.0
    body = a.min(axis=2) > 0.36  # 관 가운데는 흰빛(모든 채널이 밝다), 빛번짐은 한 색이라 가장 어두운 채널이 낮다
    bl = np.asarray(img.filter(ImageFilter.GaussianBlur(6))).astype(np.float32) / 255.0
    ch = bl / (np.linalg.norm(bl, axis=2, keepdims=True) + 1e-6)
    keys = list(COLORS)
    vec = np.array([np.array(COLORS[k], float) / np.linalg.norm(COLORS[k]) for k in keys])
    cls = np.argmax(np.tensordot(ch, vec.T, axes=1), axis=2)
    m = {}
    for i, k in enumerate(keys):
        mk = body & (cls == i)
        mk = erode(dilate(mk, 2), 2)  # 닫기: 작은 틈 메우기
        m[k] = mk
    return m


def thin(m):
    """Zhang-Suen thinning (numpy)"""
    img = np.pad(m.astype(np.uint8), 1)
    changed = True
    while changed:
        changed = False
        for step in (0, 1):
            P = img
            p2, p3, p4 = P[:-2, 1:-1], P[:-2, 2:], P[1:-1, 2:]
            p5, p6, p7 = P[2:, 2:], P[2:, 1:-1], P[2:, :-2]
            p8, p9 = P[1:-1, :-2], P[:-2, :-2]
            c = P[1:-1, 1:-1]
            nb = [p2, p3, p4, p5, p6, p7, p8, p9]
            B = sum(x.astype(np.int32) for x in nb)
            A = sum(((nb[k] == 0) & (nb[(k + 1) % 8] == 1)).astype(np.int32) for k in range(8))
            if step == 0: c1, c2 = p2 * p4 * p6, p4 * p6 * p8
            else: c1, c2 = p2 * p4 * p8, p2 * p6 * p8
            rm = (c == 1) & (B >= 2) & (B <= 6) & (A == 1) & (c1 == 0) & (c2 == 0)
            if rm.any():
                img[1:-1, 1:-1][rm] = 0
                changed = True
    return img[1:-1, 1:-1].astype(bool)


NB8 = [(-1, -1), (-1, 0), (-1, 1), (0, -1), (0, 1), (1, -1), (1, 0), (1, 1)]


def trace(sk, min_len=10):
    """가운데 선 화소를 이어 꺾은선으로: 끝점부터 아직 안 간 이웃을 따라간다 (계단 화소 때문에 생기는 가짜 갈림점에 끊기지 않게,
    갈림점 판정 없이 '간 화소' 로만). 같은 방향으로 이어지는 이웃을 먼저 고른다"""
    pts = set(zip(*np.nonzero(sk)))
    def nbrs(p): return [(p[0] + dy, p[1] + dx) for dy, dx in NB8 if (p[0] + dy, p[1] + dx) in pts]
    visited = set()
    out = []
    def walk(start):
        path = [start]; visited.add(start)
        cur, d = start, None
        while True:
            cand = [q for q in nbrs(cur) if q not in visited]
            if not cand: break
            def score(q):
                v = (q[0] - cur[0], q[1] - cur[1])
                s = 0.0 if abs(v[0]) + abs(v[1]) == 1 else 0.3  # 상하좌우 먼저
                if d is not None: s -= (v[0] * d[0] + v[1] * d[1]) / (math.hypot(*v) * math.hypot(*d) + 1e-6)
                return s
            q = min(cand, key=score)
            for c in cand:  # 옆 계단 화소는 같이 간 것으로
                if c != q and abs(c[0] - q[0]) <= 1 and abs(c[1] - q[1]) <= 1: visited.add(c)
            v = (q[0] - cur[0], q[1] - cur[1])
            d = v if d is None else (d[0] * 0.6 + v[0] * 0.4, d[1] * 0.6 + v[1] * 0.4)
            visited.add(q); path.append(q); cur = q
        return path
    ends = sorted([p for p in pts if len(nbrs(p)) == 1])
    for p in ends:
        if p not in visited: out.append(walk(p))
    for p in sorted(pts):
        if p not in visited: out.append(walk(p))
    res = []
    for P in out:
        L = sum(math.dist(P[k], P[k + 1]) for k in range(len(P) - 1))
        if L >= min_len: res.append([(x, y) for y, x in P])
    return res


def rdp(P, eps):
    if len(P) < 3: return P
    a, b = np.array(P[0], float), np.array(P[-1], float)
    ab = b - a; n = np.linalg.norm(ab)
    def dist(p):
        p = np.array(p, float)
        return np.linalg.norm(p - a) if n == 0 else abs(ab[0] * (a[1] - p[1]) - ab[1] * (a[0] - p[0])) / n
    ds = [dist(p) for p in P[1:-1]]
    i = int(np.argmax(ds)) + 1
    if ds[i - 1] > eps: return rdp(P[:i + 1], eps)[:-1] + rdp(P[i:], eps)
    return [P[0], P[-1]]


def merge(strokes, gap=10.0):
    """끝이 맞닿은 선을 이어 붙인다 (갈림점에서 끊긴 것)"""
    S = [list(s) for s in strokes]
    changed = True
    while changed:
        changed = False
        for i in range(len(S)):
            for j in range(len(S)):
                if i == j or not S[i] or not S[j]: continue
                a, b = S[i], S[j]
                for ra in (False, True):
                    for rb in (False, True):
                        A = a[::-1] if ra else a
                        B = b[::-1] if rb else b
                        if math.dist(A[-1], B[0]) <= gap:
                            # 꺾임이 크면 잇지 않는다 (글자 모서리 보존)
                            if len(A) > 2 and len(B) > 2:
                                v1 = np.subtract(A[-1], A[-3]); v2 = np.subtract(B[2], B[0])
                                c = np.dot(v1, v2) / (np.linalg.norm(v1) * np.linalg.norm(v2) + 1e-6)
                                if c < 0.3: continue
                            S[i] = A + B[1:]; S[j] = []
                            changed = True
                            break
                    if changed: break
                if changed: break
            if changed: break
        S = [s for s in S if s]
    return S


def neon(size, strokes_by_col, wd):
    W, H = size
    glow = Image.new('RGB', size, (0, 0, 0)); core = Image.new('RGB', size, (0, 0, 0))
    g, c = ImageDraw.Draw(glow), ImageDraw.Draw(core)
    for col, strokes in strokes_by_col.items():
        C = COLORS[col]
        light = tuple(int(v + (255 - v) * 0.55) for v in C)
        for P in strokes:
            g.line(P, fill=tuple(int(v * 0.55) for v in C), width=int(wd * 1.8), joint='curve')
            c.line(P, fill=C, width=int(wd), joint='curve')
            c.line(P, fill=light, width=max(1, int(wd * 0.62)), joint='curve')
            for p in (P[0], P[-1]):
                r = wd / 2; c.ellipse([p[0] - r, p[1] - r, p[0] + r, p[1] + r], fill=C)
    img = ImageChops.add(glow.filter(ImageFilter.GaussianBlur(14)).point(lambda v: int(v * 0.8)), glow.filter(ImageFilter.GaussianBlur(4)).point(lambda v: int(v * 0.35)))
    return ImageChops.lighter(img, core)


def main():
    img = Image.open(SRC).convert('RGB')
    M = masks(img)
    res, wds = {}, {}
    for col, m in M.items():
        # 관 굵기 = 몸통 화소 수 / 가운데 선 길이
        sk = thin(m)
        wds[col] = m.sum() / max(1, sk.sum())
        strokes = trace(sk)
        strokes = merge(strokes)
        strokes = [rdp(s, 1.1) for s in strokes]
        strokes = [s for s in strokes if min(x for x, y in s) > 20]  # 원본 왼쪽 가장자리의 회색 점(화면 조각) 빼기
        res[col] = [[(float(x), float(y)) for x, y in s] for s in strokes]
        print(col, 'strokes', len(strokes), 'width', round(wds[col], 1))
    json.dump({'size': img.size, 'width': wds, 'strokes': res}, open(OUTJ, 'w'), indent=0)
    wd = float(np.median(list(wds.values()))) * 1.35  # 가운데 흰빛 폭보다 관이 굵다
    ours = neon(img.size, {k: [[tuple(p) for p in s] for s in v] for k, v in res.items()}, wd)
    W, H = img.size
    sheet = Image.new('RGB', (W, H * 2 + 10), (0, 0, 0))
    sheet.paste(img, (0, 0)); sheet.paste(ours, (0, H + 10))
    sheet.save(OUTP)
    print('ok', OUTP)


if __name__ == '__main__':
    main()
