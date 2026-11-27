"""따라 그린 시작 화면 간판(tools/ui_art/title_logo_strokes.json)을 게임이 읽는 C++ 데이터로 (D-440)

    python tools/ui_art/gen_title_inl.py   → Source/ProjectB/PDTitleLogo.inl

좌표는 원본 그림(1139 × 546) 화소. 색: 0 하늘(BAR) 1 분홍(Alice) 2 노랑(마티니). 깜빡임 묶음:
하늘은 획 가운데 x 로 B · A · R (0 ~ 2, 시작 화면의 NB), 분홍은 A·l · i · c · e (0 ~ 4, PB — 획이 이어져 있어 가운데 x 로 나눔), 노랑은 잔(CB).
잔(노랑 · 잔 속 하늘 · 분홍 빨대)은 따라 그린 선이 아니라 martini() 가 손으로 설계한 선 (D-481)
"""
import json
import math
import os

ROOT = os.path.join(os.path.dirname(__file__), '..', '..')
SRC = os.path.join(os.path.dirname(__file__), 'title_logo_strokes.json')
OUT = os.path.join(ROOT, 'Source', 'ProjectB', 'PDTitleLogo.inl')
COLS = ['cyan', 'pink', 'yellow']


def clean(P, min_d=1.0, max_turn=140.0):
    """되돌아가는 점 · 겹친 점 지우기 (D-443): 선 끝을 다른 선에 맞붙일 때 끝이 살짝 뒤로 꺾이는 점이 생겨,
    게임 선 찍기가 그 자리에서 뾰족하게 튀어나왔다 (잔 모서리 · 받침). 1px 안의 점과 140° 넘게 되꺾이는 점을 없앤다"""
    import math
    Q = [P[0]]
    for p in P[1:]:
        if math.dist(p, Q[-1]) >= min_d: Q.append(p)
    if len(Q) < 2: Q.append(P[-1])
    changed = True
    while changed and len(Q) > 2:
        changed = False
        for i in range(1, len(Q) - 1):
            a, b, c = Q[i - 1], Q[i], Q[i + 1]
            u = (b[0] - a[0], b[1] - a[1]); v = (c[0] - b[0], c[1] - b[1])
            lu, lv = math.hypot(*u), math.hypot(*v)
            if lu < 1e-6 or lv < 1e-6: Q.pop(i); changed = True; break
            cs = (u[0] * v[0] + u[1] * v[1]) / (lu * lv)
            if math.degrees(math.acos(max(-1.0, min(1.0, cs)))) > max_turn: Q.pop(i); changed = True; break
    return Q


def fillet(P, turn_deg=50.0, d=6.0, n=5):
    """뾰족한 꼭짓점을 작은 2차 곡선으로 둥글린다 (D-441): 게임의 빛번짐 띠가 날카로운 모서리(잔 받침 20°)에서 바깥으로 삐져나가던 것.
    꺾임이 turn_deg 넘는 꼭짓점만, 양쪽 마디를 따라 d 화소씩. 닫힌 선(처음 = 끝)은 맞물린 꼭짓점도"""
    import math
    closed = len(P) > 3 and math.dist(P[0], P[-1]) < 1.0
    Q = P[:-1] if closed else P[:]
    m = len(Q)
    out = []
    for i in range(m):
        V = Q[i]
        if not closed and (i == 0 or i == m - 1):
            out.append(V); continue
        A, B = Q[(i - 1) % m], Q[(i + 1) % m]
        u1 = (V[0] - A[0], V[1] - A[1]); u2 = (B[0] - V[0], B[1] - V[1])
        l1, l2 = math.hypot(*u1), math.hypot(*u2)
        if l1 < 1e-6 or l2 < 1e-6: out.append(V); continue
        c = (u1[0] * u2[0] + u1[1] * u2[1]) / (l1 * l2)
        if math.degrees(math.acos(max(-1.0, min(1.0, c)))) < turn_deg: out.append(V); continue
        dd = min(d, l1 / 2.5, l2 / 2.5)
        p0 = (V[0] - u1[0] / l1 * dd, V[1] - u1[1] / l1 * dd)
        p2 = (V[0] + u2[0] / l2 * dd, V[1] + u2[1] / l2 * dd)
        for k in range(n + 1):
            t = k / n
            out.append(((1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * V[0] + t * t * p2[0], (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * V[1] + t * t * p2[1]))
    if closed: out.append(out[0])
    return out


def group(col, cx):
    if col == 'cyan': return 0 if cx < 165 else 1 if cx < 235 else 2
    if col == 'pink': return 0 if cx < 470 else 2 if cx < 600 else 3 if cx < 700 else 4
    return 0


# ---- 마티니 잔 (D-481): 따라 그린 잔 대신 손으로 설계한 선. 좌표는 원본 그림(1139 × 546) 화소
# 곧게 선 대칭 잔: V 잔(닫힌 관 하나) · 다리 + 납작한 타원 받침(관 하나), 잔 속 하늘색 술 선 · 각얼음, 분홍 빨대, 노란 반짝 선 셋.
# 잔 전체를 TILT 만큼 한 덩어리로 기울일 수 있다 (선 모양은 그대로 — 예전처럼 윗변만 찌그러지지 않게)
GLASS = dict(cx=952.0, rim_y=176.0, half=124.0, apex_y=336.0, foot_y=426.0, corner=7.0)
TILT = -8.0  # 잔 기울기 (도, + = 시계 방향). 받침 가운데를 축으로. −8° = 오른쪽이 올라감 (인게임 시안처럼, 사용자 2026-11-27)
# 게임에서 보이는 관 반폭 (원본 화소): 관 굵기 × 0.8 × 1.5(시작 화면 LineK) / 2. 서로 다른 관은 반폭 합 + GAP 보다 가까이 두지 않는다 (네온 선이 붙으면 어색하다)
HALF = {'yellow': 14.86 * 0.6, 'liquid': 14.86 * 0.6, 'straw': 20.11 * 0.6}
GAP = 10.0


def _resample(P, step=1.0):
    out = [P[0]]
    for a, b in zip(P, P[1:]):
        n = max(1, int(math.dist(a, b) / step))
        out += [(a[0] + (b[0] - a[0]) * k / n, a[1] + (b[1] - a[1]) * k / n) for k in range(1, n + 1)]
    return out


def _seg_dist(p, a, b):
    vx, vy = b[0] - a[0], b[1] - a[1]
    t = max(0.0, min(1.0, ((p[0] - a[0]) * vx + (p[1] - a[1]) * vy) / (vx * vx + vy * vy)))
    return math.dist(p, (a[0] + vx * t, a[1] + vy * t))


def _poly_dist(p, Q):
    return min(_seg_dist(p, a, b) for a, b in zip(Q, Q[1:]))


def _cut(P, Q, d):
    """선 P 에서 선 Q 와 d 보다 가까운 부분을 잘라 낸 조각들 (빨대가 지나는 자리의 잔 테 · 술 선 틈)"""
    parts, cur = [], []
    for p in _resample(P):
        if _poly_dist(p, Q) < d:
            if len(cur) > 1: parts.append(cur)
            cur = []
        else:
            cur.append(p)
    if len(cur) > 1: parts.append(cur)
    return [_simplify(c) for c in parts if math.dist(c[0], c[-1]) >= 14.0]  # 너무 짧은 토막은 버린다


def _simplify(P, eps=0.3):
    """곧은 구간의 촘촘한 점을 줄인다 (Ramer-Douglas-Peucker)"""
    if len(P) < 3: return P
    i, dm = max(((k, _seg_dist(P[k], P[0], P[-1])) for k in range(1, len(P) - 1)), key=lambda t: t[1])
    if dm <= eps: return [P[0], P[-1]]
    return _simplify(P[:i + 1], eps)[:-1] + _simplify(P[i:], eps)


def check(out):
    """다른 관끼리 가장 가까운 거리 / 필요한 거리 목록 (1 보다 작으면 겹쳐 보인다). 같은 색 노랑끼리(잔 · 다리 · 받침 · 반짝)는 이어지는 관이라 뺀다"""
    res = []
    for i in range(len(out)):
        for j in range(i + 1, len(out)):
            (ca, A), (cb, B) = out[i], out[j]
            if ca == cb: continue
            dmin = min(_poly_dist(p, B) for p in _resample(A, 2.0))
            res.append((dmin / (HALF[ca] + HALF[cb] + GAP), ca, cb, round(dmin, 1)))
    return sorted(res)


def _round(P, r, closed, n=4):
    """꼭짓점을 반지름 r 정도의 작은 곡선으로 (유리관을 구부린 모서리). 마디당 꺾임이 작아 번짐이 끊기지 않는다.
    closed 면 P 를 닫힌 다각형으로 보고 모든 꼭짓점을, 아니면 두 끝을 뺀 꼭짓점만 둥글린다"""
    m = len(P)
    idx = range(m) if closed else range(1, m - 1)
    out = [] if closed else [P[0]]
    for i in idx:
        A, V, B = P[i - 1], P[i], P[(i + 1) % m]
        la, lb = math.dist(A, V), math.dist(V, B)
        d = min(r, la / 3, lb / 3)
        p0 = (V[0] + (A[0] - V[0]) / la * d, V[1] + (A[1] - V[1]) / la * d)
        p2 = (V[0] + (B[0] - V[0]) / lb * d, V[1] + (B[1] - V[1]) / lb * d)
        for k in range(n + 1):
            t = k / n
            out.append(((1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * V[0] + t * t * p2[0], (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * V[1] + t * t * p2[1]))
    out.append(out[0] if closed else P[-1])
    return out


def martini(tilt=None):
    """(색, 점 목록) 목록. 색: 'yellow' 잔 · 반짝, 'liquid' 잔 속 하늘, 'straw' 분홍 빨대.
    빨대가 지나는 자리는 잔 윗변 · 술 선을 끊어 틈을 낸다 (사용자: 빨대가 노란 선을 중간에 잘라 겹치지 않게). 잔 속 하늘 그림은 노란 벽과 띄운다"""
    g = GLASS
    cx, ry, h, ay = g['cx'], g['rim_y'], g['half'], g['apex_y']
    wall = lambda y: h * (ay - y) / (ay - ry)  # 높이 y 에서 잔 벽(가운데 선)까지 반폭
    cosw = (ay - ry) / math.hypot(h, ay - ry)   # 벽과 수평 거리 → 수직 거리
    need = lambda a, b: HALF[a] + HALF[b] + GAP
    # 빨대: 오른쪽 위 잔 밖에서 비스듬히 들어와 술 선을 지나 얼음 오른쪽 위에서 끝난다
    sx, sk, se = cx + 32.0, 0.5, 30.0  # 윗변을 지나는 x, 기울기(dx/dy), 잔 속 길이
    straw = [(sx + sk * 64.0, ry - 64.0), (sx - sk * se, ry + se)]
    # V 잔: 윗변 · 두 벽을 한 관으로 (모서리는 작게 구부림), 빨대 자리만 끊는다
    bowl = _round([(cx - h, ry), (cx, ay), (cx + h, ry)], g['corner'], True)
    # 닫힌 관을 윗변 가운데에서 열어 한 줄로 (끊는 자리가 윗변 위에 오게)
    k = min(range(len(bowl) - 1), key=lambda q: abs(bowl[q][0] - cx) + abs(bowl[q][1] - ry) * 10)
    ring = bowl[k:-1] + bowl[:k + 1]
    out = [('yellow', P) for P in _cut(ring, straw, need('yellow', 'straw'))]
    # 술 선: 윗변과 나란히, 벽과 노랑-하늘 간격만큼 띄움. 빨대 자리는 끊는다
    ly = ry + need('yellow', 'liquid') + 4.0
    lw = wall(ly) - need('yellow', 'liquid') / cosw
    out += [('liquid', P) for P in _cut([(cx - lw, ly), (cx + lw, ly)], straw, need('liquid', 'straw'))]
    # 각얼음: 윗변과 나란한 정사각형 (모서리 둥글게). 술 선 아래 · 벽 · 빨대와 띄우고, 빨대 반대쪽(왼쪽)으로 조금
    s = 11.0
    iy = ly + need('liquid', 'liquid') + s
    ix = cx - 6.0
    out.append(('liquid', _round([(ix - s, iy - s), (ix + s, iy - s), (ix + s, iy + s), (ix - s, iy + s)], 4.0, True)))
    out.append(('straw', straw))
    # 다리 + 받침: 꼭짓점에서 조금 띄워 내려와 받침(납작한 사다리꼴, 인게임 시안처럼) 윗변 가운데에 닿는다
    fy = g['foot_y']
    out.append(('yellow', [(cx, ay + 6.0), (cx, fy - 16.0)]))
    out.append(('yellow', _round([(cx - 26.0, fy - 16.0), (cx + 26.0, fy - 16.0), (cx + 56.0, fy), (cx - 56.0, fy)], 5.0, True)))
    # 반짝 선 셋: 잔 오른쪽 위 한 점에서 퍼지는 같은 길이 선
    sc = (cx + h + 6.0, ry - 22.0)
    for a in (-80.0, -46.0, -14.0):
        c, sn = math.cos(math.radians(a)), math.sin(math.radians(a))
        out.append(('yellow', [(sc[0] + c * 48.0, sc[1] + sn * 48.0), (sc[0] + c * 82.0, sc[1] + sn * 82.0)]))
    tilt = TILT if tilt is None else tilt
    if tilt:
        r = math.radians(tilt); cr, sr = math.cos(r), math.sin(r)
        px, py = cx, fy
        out = [(c, [(px + (x - px) * cr - (y - py) * sr, py + (x - px) * sr + (y - py) * cr) for x, y in P]) for c, P in out]
    return out


def strokes(tilt=None):
    """(색 번호, 묶음, 점 목록, 열린 끝 A, 열린 끝 B) 목록. 색: 0 하늘 1 분홍 2 노랑 3 잔 속 하늘"""
    J = json.load(open(SRC))
    res = []
    for ci, col in enumerate(COLS):
        if col == 'yellow': continue  # 잔은 손으로 설계한 선 (D-481)
        for st in J['strokes'][col]:
            P = clean(st['pts'])
            if col == 'pink' and min(p[0] for p in P) > 900: continue  # 따라 그린 빨대 (아래 martini 가 대신)
            if len(P) > 2 and sum(math.dist(P[q], P[q + 1]) for q in range(len(P) - 1)) < 40.0:
                P = [P[0], P[-1]]  # 짧은 획(i 점)은 다듬기를 건너뛴 화소 계단이라 끊긴 듯 보였다 → 두 끝을 잇는 곧은 선 (D-447)
            # fillet 은 쓰지 않는다 (D-445: 잘게 꺾인 점이 번짐 띠를 더 삐져나가게 했다 — 게임이 모서리에서 번짐을 끊어 찍는다)
            cx = sum(p[0] for p in P) / len(P)
            res.append((ci, group(col, cx), P, st['free'][0], st['free'][1]))
    for c, P in martini(tilt):
        closed = math.dist(P[0], P[-1]) < 0.5
        ci, grp = (2, 0) if c == 'yellow' else (3, 0) if c == 'liquid' else (1, 4)  # 빨대는 분홍, 깜빡임 묶음은 예전처럼 e
        res.append((ci, grp, P, not closed, not closed))
    return res


def main():
    J = json.load(open(SRC))
    lines = ['// 자동 생성 (tools/ui_art/gen_title_inl.py, D-440): 시작 화면 BAR Alice 간판 — 원본 그림에서 따라 그린 네온 획. 손으로 고치지 말 것',
             '#pragma once', '', 'namespace PDTitleLogo', '{',
             '\tconstexpr float SrcW = %.1ff, SrcH = %.1ff;' % tuple(J['size']),
             '\tconstexpr float Tube[4] = {%s, %.2ff}; // 관 굵기 (원본 화소, 하늘 · 분홍 · 노랑 · 잔 속 하늘 = 노랑 굵기)' % (', '.join('%.2ff' % J['tube'][c] for c in COLS), J['tube']['yellow']),
             '\tstruct FStroke { int32 Color, Group, N; bool bFreeA, bFreeB; const float* P; };']
    arrays, entries = [], []
    for k, (ci, grp, P, fa, fb) in enumerate(strokes()):
        arrays.append('\tconst float S%d[] = {%s};' % (k, ', '.join('%.1ff, %.1ff' % (x, y) for x, y in P)))
        entries.append('\t\t{%d, %d, %d, %s, %s, S%d},' % (ci, grp, len(P), 'true' if fa else 'false', 'true' if fb else 'false', k))
    k = len(entries)
    lines += arrays
    lines += ['\tconst FStroke Strokes[] = {'] + entries + ['\t};', '\tconstexpr int32 NumStrokes = %d;' % k, '}', '']
    open(OUT, 'w', encoding='utf-8').write('\n'.join(lines))
    print('ok', k, 'strokes', OUT)


if __name__ == '__main__':
    main()
