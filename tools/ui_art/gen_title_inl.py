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
GLASS = dict(cx=940.0, rim_y=196.0, half=100.0, apex_y=306.0, foot_y=414.0, foot_rx=50.0, foot_ry=9.0, corner=7.0)
TILT = 0.0  # 잔 기울기 (도, + = 시계 방향). 받침 가운데를 축으로


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
    """(색, 점 목록) 목록. 색: 'yellow' 잔 · 반짝, 'liquid' 잔 속 하늘, 'straw' 분홍 빨대"""
    g = GLASS
    cx, ry, h, ay = g['cx'], g['rim_y'], g['half'], g['apex_y']
    wall = lambda y: h * (ay - y) / (ay - ry)  # 높이 y 에서 잔 벽까지 반폭
    out = []
    # V 잔: 윗변 · 두 벽을 한 관으로. 빨대가 지나는 자리만 윗변을 끊는다 (틈 왼끝 → 왼쪽 모서리 → 아래 꼭짓점 → 오른쪽 모서리 → 틈 오른끝)
    sx0, sx1 = cx + 30.0, cx + 54.0
    out.append(('yellow', _round([(sx0, ry), (cx - h, ry), (cx, ay), (cx + h, ry), (sx1, ry)], g['corner'], False)))
    # 다리 + 받침: 꼭짓점에서 조금 띄워 내려와 납작한 타원 받침 윗가장자리에 닿는다
    fy, frx, fry = g['foot_y'], g['foot_rx'], g['foot_ry']
    out.append(('yellow', [(cx, ay + 10.0), (cx, fy - fry)]))
    out.append(('yellow', [(cx + frx * math.cos(math.radians(a)), fy + fry * math.sin(math.radians(a))) for a in range(-90, 271, 10)]))
    # 잔 속 술 선: 윗변과 나란히, 양쪽 벽에서 띄운다
    ly = ry + 30.0
    lw = wall(ly) - 16.0
    out.append(('liquid', [(cx - lw, ly), (cx + lw, ly)]))
    # 각얼음: 윗변과 나란한 정사각형 (모서리 살짝 둥글게), 술 선 아래
    s, iy = 15.0, ly + 32.0
    out.append(('liquid', _round([(cx - s, iy - s), (cx + s, iy - s), (cx + s, iy + s), (cx - s, iy + s)], 4.0, True)))
    # 빨대: 오른쪽 위에서 윗변 틈 가운데를 지나 술 선 바로 위까지 (선끼리 닿지 않게)
    gx = (sx0 + sx1) / 2
    tx, ty = gx + 32.0, ry - 56.0
    t = (ly - 14.0 - ty) / (ry - ty)
    out.append(('straw', [(tx, ty), (tx + (gx - tx) * t, ty + (ry - ty) * t)]))
    # 반짝 선 셋: 잔 오른쪽 위 한 점에서 퍼지는 같은 길이 선
    sc = (cx + h - 4.0, ry - 14.0)
    for a in (-72.0, -40.0, -10.0):
        c, sn = math.cos(math.radians(a)), math.sin(math.radians(a))
        out.append(('yellow', [(sc[0] + c * 56.0, sc[1] + sn * 56.0), (sc[0] + c * 90.0, sc[1] + sn * 90.0)]))
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
