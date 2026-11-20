"""따라 그린 시작 화면 간판(tools/ui_art/title_logo_strokes.json)을 게임이 읽는 C++ 데이터로 (D-440)

    python tools/ui_art/gen_title_inl.py   → Source/ProjectB/PDTitleLogo.inl

좌표는 원본 그림(1139 × 546) 화소. 색: 0 하늘(BAR) 1 분홍(Alice) 2 노랑(마티니). 깜빡임 묶음:
하늘은 획 가운데 x 로 B · A · R (0 ~ 2, 시작 화면의 NB), 분홍은 A·l · i · c · e (0 ~ 4, PB — 획이 이어져 있어 가운데 x 로 나눔), 노랑은 잔(CB)
"""
import json
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


def main():
    J = json.load(open(SRC))
    lines = ['// 자동 생성 (tools/ui_art/gen_title_inl.py, D-440): 시작 화면 BAR Alice 간판 — 원본 그림에서 따라 그린 네온 획. 손으로 고치지 말 것',
             '#pragma once', '', 'namespace PDTitleLogo', '{',
             '\tconstexpr float SrcW = %.1ff, SrcH = %.1ff;' % tuple(J['size']),
             '\tconstexpr float Tube[3] = {%s}; // 관 굵기 (원본 화소, 하늘 · 분홍 · 노랑)' % ', '.join('%.2ff' % J['tube'][c] for c in COLS),
             '\tstruct FStroke { int32 Color, Group, N; bool bFreeA, bFreeB; const float* P; };']
    arrays, entries = [], []
    k = 0
    for ci, col in enumerate(COLS):
        for st in J['strokes'][col]:
            P = clean(st['pts'])
            # fillet 은 쓰지 않는다 (D-445: 잘게 꺾인 점이 번짐 띠를 더 삐져나가게 했다 — 게임이 모서리에서 번짐을 끊어 찍는다)
            cx = sum(p[0] for p in P) / len(P)
            arrays.append('\tconst float S%d[] = {%s};' % (k, ', '.join('%.1ff, %.1ff' % (x, y) for x, y in P)))
            entries.append('\t\t{%d, %d, %d, %s, %s, S%d},' % (ci, group(col, cx), len(P), 'true' if st['free'][0] else 'false', 'true' if st['free'][1] else 'false', k))
            k += 1
    lines += arrays
    lines += ['\tconst FStroke Strokes[] = {'] + entries + ['\t};', '\tconstexpr int32 NumStrokes = %d;' % k, '}', '']
    open(OUT, 'w', encoding='utf-8').write('\n'.join(lines))
    print('ok', k, 'strokes', OUT)


if __name__ == '__main__':
    main()
