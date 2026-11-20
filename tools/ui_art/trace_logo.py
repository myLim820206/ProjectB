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


def trace(sk, min_len=18):  # 18px 보다 짧은 조각은 잘못 잡힌 잔가지 (A 꼭대기 · 꼬치 아래, D-432)
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


def resample(P, step=2.0):
    out = [P[0]]; acc = 0.0
    for a, b in zip(P, P[1:]):
        L = math.dist(a, b)
        t = step - acc
        while t <= L:
            out.append((a[0] + (b[0] - a[0]) * t / L, a[1] + (b[1] - a[1]) * t / L)); t += step
        acc = (acc + L) % step if L > 0 else acc
        acc = L - (t - step) if L > 0 else acc
    if math.dist(out[-1], P[-1]) > 0.5: out.append(P[-1])
    return out


def smooth_keep_corners(P, step=2.0, reach=7, sigma=5.0, corner_deg=55.0):
    """화소 계단 때문에 우는 곡선을 편다 (D-431). 2px 간격으로 다시 찍고, 앞뒤 reach 점(14px) 방향이 corner_deg 넘게 꺾인 곳을
    모서리로 남긴 뒤, 모서리 사이 조각만 가우스(sigma 점 = 10px)로 편다. 조각 끝(모서리 · 선 끝)은 움직이지 않는다"""
    Q = resample(P, step)
    n = len(Q)
    if n < 2 * reach + 3: return P
    ang = [0.0] * n
    for k in range(reach, n - reach):
        v1 = (Q[k][0] - Q[k - reach][0], Q[k][1] - Q[k - reach][1])
        v2 = (Q[k + reach][0] - Q[k][0], Q[k + reach][1] - Q[k][1])
        c = (v1[0] * v2[0] + v1[1] * v2[1]) / (math.hypot(*v1) * math.hypot(*v2) + 1e-6)
        ang[k] = math.degrees(math.acos(max(-1.0, min(1.0, c))))
    corners = [0]
    k = reach
    while k < n - reach:
        if ang[k] > corner_deg:
            m = max(range(k, min(n - reach, k + 2 * reach)), key=lambda q: ang[q])  # 그 둘레에서 가장 꺾인 점
            corners.append(m); k = m + reach
        else: k += 1
    corners.append(n - 1)
    out = []
    W = int(sigma * 3)
    wts = [math.exp(-(d * d) / (2 * sigma * sigma)) for d in range(-W, W + 1)]
    for a, b in zip(corners, corners[1:]):
        seg = Q[a:b + 1]
        sm = []
        for idx in range(len(seg)):
            if idx == 0 or idx == len(seg) - 1: sm.append(seg[idx]); continue
            sx = sy = sw = 0.0
            r = min(W, idx, len(seg) - 1 - idx)  # 끝 가까이는 짧게 (끝이 끌려가지 않게)
            for d in range(-r, r + 1):
                w = wts[d + W]; sx += seg[idx + d][0] * w; sy += seg[idx + d][1] * w; sw += w
            sm.append((sx / sw, sy / sw))
        out.extend(sm if not out else sm[1:])
    return out


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


def unit(v):
    n = math.hypot(*v)
    return (v[0] / n, v[1] / n) if n > 1e-6 else (0.0, 0.0)


def merge_collinear(strokes, gap, k=6, cos_min=0.85):
    """곧게 이어지는 끝끼리 잇기 (D-434): 두 끝이 gap 안에 있고, 두 선의 방향과 끝을 잇는 방향이 모두 거의 같으면 한 줄로.
    교차점에서 깎기가 선을 끊어, 사선이 가로획을 지나 이어지는 곳(A 왼쪽 다리)이 18px 떨어져 따로 끝나던 것"""
    S = [list(x) for x in strokes]
    def d(a, b): return unit((b[0] - a[0], b[1] - a[1]))
    def dot(u, v): return u[0] * v[0] + u[1] * v[1]
    changed = True
    while changed:
        changed = False
        best = None
        for i in range(len(S)):
            for j in range(len(S)):
                if i == j or len(S[i]) <= k or len(S[j]) <= k: continue
                for ri in (False, True):
                    for rj in (False, True):
                        A = S[i][::-1] if ri else S[i]
                        B = S[j][::-1] if rj else S[j]
                        g = math.dist(A[-1], B[0])
                        if g > gap or g < 1e-6: continue
                        vA, vB, vAB = d(A[-1 - k], A[-1]), d(B[0], B[k]), d(A[-1], B[0])
                        c = dot(vA, vB) if min(dot(vA, vAB), dot(vAB, vB)) >= 0.6 else -1.0  # 두 선은 거의 같은 방향, 잇는 방향은 조금 비껴도 됨 (교차점에서 끊긴 자리는 비껴 있다)
                        if c >= cos_min and (best is None or g < best[0]): best = (g, i, j, ri, rj)
        if best:
            _, i, j, ri, rj = best
            A = S[i][::-1] if ri else S[i]
            B = S[j][::-1] if rj else S[j]
            S[i] = A + B
            S.pop(j)
            changed = True
    return S


def straighten_junctions(strokes, tol, m=6, win=14):
    """교차점에서 곧게 잇기 (D-434): 선 A 의 끝이 선 B 의 가운데에 닿으면, B 를 닿는 둘레(±win 점)에서 A 의 방향과 가장 곧게
    이어지는 점에서 둘로 나누고, A 와 곧게 이어지는 조각을 A 에 붙인다. A 의 사선이 B 의 가로획을 지나 이어지는 글자(A 왼쪽 다리)가
    교차점에서 꺾여 따로 떨어지던 것. 점들은 촘촘해야 한다 (2px)"""
    S = [list(x) for x in strokes]
    changed = True
    guard = 0
    while changed and guard < 50:
        changed = False; guard += 1
        for ai in range(len(S)):
            A = S[ai]
            if len(A) < m + 2: continue
            for end in (0, 1):
                E = A[0] if end == 0 else A[-1]
                inner = A[min(m, len(A) - 1)] if end == 0 else A[max(0, len(A) - 1 - m)]
                vA = unit((E[0] - inner[0], E[1] - inner[1]))  # 끝으로 들어가는 방향
                best = None
                for bi in range(len(S)):
                    if bi == ai: continue
                    B = S[bi]
                    if len(B) < 2 * m + 2: continue
                    dists = [math.dist(E, q) for q in B]
                    k0 = min(range(len(B)), key=lambda q: dists[q])
                    if dists[k0] > tol or k0 < m or k0 > len(B) - 1 - m: continue  # B 의 끝 근처는 잇기(merge) 몫
                    for k in range(max(m, k0 - win), min(len(B) - 1 - m, k0 + win) + 1):
                        if dists[k] > tol * 2.2: continue
                        v_in = unit((B[k][0] - B[k - m][0], B[k][1] - B[k - m][1]))   # B 앞 조각이 k 로 들어오는 방향
                        v_out = unit((B[k + m][0] - B[k][0], B[k + m][1] - B[k][1]))  # B 뒤 조각이 k 에서 나가는 방향
                        # A 가 k 에 들어와 곧게 이어지는 조각: 뒤 조각(같은 방향 v_out) 또는 앞 조각을 거꾸로(-v_in)
                        for piece, c in (('after', vA[0] * v_out[0] + vA[1] * v_out[1]), ('before', -(vA[0] * v_in[0] + vA[1] * v_in[1]))):
                            score = c - dists[k] / (tol * 6)
                            if c > 0.8 and (best is None or score > best[0]): best = (score, bi, k, piece)
                if best is None: continue
                _, bi, k, piece = best
                B = S[bi]
                before, after = B[:k + 1], B[k:]
                Ar = A if end == 1 else A[::-1]  # 끝이 맨 뒤로 오게
                if piece == 'after': joined, rest = Ar + after, before
                else: joined, rest = Ar + before[::-1], after
                S[ai] = joined; S[bi] = rest
                changed = True
                break
            if changed: break
    return S


def nearest_on(P, q):
    """꺾은선 P 위에서 q 와 가장 가까운 점과 거리"""
    best, bd = None, 1e9
    for a, b in zip(P, P[1:]):
        ax, ay = a; bx, by = b
        vx, vy = bx - ax, by - ay
        L2 = vx * vx + vy * vy
        t = 0.0 if L2 == 0 else max(0.0, min(1.0, ((q[0] - ax) * vx + (q[1] - ay) * vy) / L2))
        p = (ax + vx * t, ay + vy * t)
        d = math.dist(p, q)
        if d < bd: best, bd = p, d
    return best, bd


def snap_ends(strokes, tol):
    """다른 선(또는 제 선의 먼 부분)에 닿는 끝은 그 선 위 가장 가까운 점까지 이어 붙이고 '이음 끝' 으로 표시한다.
    이음 끝에는 둥근 마개를 그리지 않는다 (이음매에 점이 생기던 것, D-430). 돌려주는 값: [(선, [처음이 열린 끝인가, 끝이 열린 끝인가])]"""
    out = []
    for i, P in enumerate(strokes):
        P = list(P)
        free = [True, True]
        for k, idx in ((0, 0), (1, -1)):
            q = P[idx]
            best, bd = None, 1e9
            for j, Q in enumerate(strokes):
                if j == i:
                    # 제 선: 끝에서 멀리 떨어진 부분만 (고리 · 되돌아와 닿는 획)
                    L = 0.0; part = []
                    seq = P if idx == 0 else P[::-1]
                    for a, b in zip(seq, seq[1:]):
                        L += math.dist(a, b)
                        if L > tol * 4: part.append(b)
                    if len(part) < 2: continue
                    p, d = nearest_on(part, q)
                else:
                    p, d = nearest_on(Q, q)
                if d < bd: best, bd = p, d
            if best is not None and bd <= tol:
                free[k] = False
                if bd > 0.5:
                    if idx == 0: P.insert(0, best)
                    else: P.append(best)
        out.append((P, free))
    return out


EDGE = {'cyan': (20, 210, 235), 'pink': (255, 45, 205), 'yellow': (255, 215, 20)}   # 관 바깥 테 (원본에서 잰 진한 색)
CORE = {'cyan': (175, 255, 255), 'pink': (255, 175, 242), 'yellow': (255, 250, 165)}  # 관 가운데 (파스텔)


def neon(size, strokes_by_col, wds, ss=3):
    """strokes_by_col: 색 → [(점들, [처음 열린 끝, 끝 열린 끝])]. ss 배로 크게 그려 줄인다 (가장자리 매끈, D-432).
    겹: 넓은 빛(진한 테 색) → 테(진한 색, 관 폭) → 가운데(파스텔, 관 폭 0.6). 색 관을 다 그린 뒤 가운데를 한꺼번에"""
    W, H = size
    S = (W * ss, H * ss)
    glow = Image.new('RGB', S, (0, 0, 0)); core = Image.new('RGB', S, (0, 0, 0))
    g, c = ImageDraw.Draw(glow), ImageDraw.Draw(core)
    def sc(P): return [(x * ss, y * ss) for x, y in P]
    def cap(P, free, col, w):
        for f, p in ((free[0], P[0]), (free[1], P[-1])):
            if f: r = w / 2; c.ellipse([p[0] - r, p[1] - r, p[0] + r, p[1] + r], fill=col)
    for col, strokes in strokes_by_col.items():
        wd = wds[col] * ss
        for P, free in strokes:
            Q = sc(P)
            g.line(Q, fill=EDGE[col], width=int(wd * 1.6), joint='curve')
            c.line(Q, fill=EDGE[col], width=int(wd), joint='curve')
            cap(Q, free, EDGE[col], wd)
    for col, strokes in strokes_by_col.items():
        wd = wds[col] * ss
        for P, free in strokes:
            Q = sc(P)
            c.line(Q, fill=CORE[col], width=max(1, int(wd * 0.6)), joint='curve')
            cap(Q, free, CORE[col], wd * 0.6)
    img = ImageChops.add(glow.filter(ImageFilter.GaussianBlur(12 * ss)).point(lambda v: int(v * 0.75)), glow.filter(ImageFilter.GaussianBlur(4 * ss)).point(lambda v: int(v * 0.45)))
    img = ImageChops.lighter(img, core)
    return img.resize(size, Image.LANCZOS)


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
        # 필기체(분홍)는 정말 뾰족한 곳(A 꼭대기)만 모서리, 더 넓게 편다. BAR · 잔은 각진 모서리를 남긴다 (D-433: 급한 곡선을 모서리로 오인해 꺾임이 생겼다)
        kw = dict(corner_deg=120.0, sigma=7.0) if col == 'pink' else dict(corner_deg=55.0, sigma=5.0)
        strokes = [smooth_keep_corners(smooth_keep_corners(s, **kw), **kw) for s in strokes]  # 두 번 펴기
        strokes = merge_collinear(strokes, max(10.0, wds[col] * 2.0))
        strokes = straighten_junctions(strokes, max(8.0, wds[col] * 1.6))
        strokes = [smooth_keep_corners(s, **kw) for s in strokes]  # 이은 자리도 매끈하게
        strokes = [rdp(s, 0.2) for s in strokes]  # 점은 촘촘히 (짧은 직선으로 각지지 않게)
        strokes = [s for s in strokes if min(x for x, y in s) > 20]  # 원본 왼쪽 가장자리의 회색 점(화면 조각) 빼기
        snapped = snap_ends(strokes, max(8.0, wds[col] * 1.2))
        res[col] = [{'pts': [(float(x), float(y)) for x, y in P], 'free': free} for P, free in snapped]
        print(col, 'strokes', len(strokes), 'width', round(wds[col], 1))
    json.dump({'size': img.size, 'width': wds, 'tube': {k: v * 1.55 for k, v in wds.items()}, 'strokes': res}, open(OUTJ, 'w'), indent=0)
    tube = {k: v * 1.55 for k, v in wds.items()}  # 관 굵기 = 가운데 흰빛 폭 × 1.55 (원본 관 폭에 맞춤, 색마다, D-432)
    ours = neon(img.size, {k: [([tuple(p) for p in st['pts']], st['free']) for st in v] for k, v in res.items()}, tube)
    W, H = img.size
    sheet = Image.new('RGB', (W, H * 2 + 10), (0, 0, 0))
    sheet.paste(img, (0, 0)); sheet.paste(ours, (0, H + 10))
    sheet.save(OUTP)
    print('ok', OUTP)


if __name__ == '__main__':
    main()
