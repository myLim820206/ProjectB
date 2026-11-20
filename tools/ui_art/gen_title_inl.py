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
            P = st['pts']
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
