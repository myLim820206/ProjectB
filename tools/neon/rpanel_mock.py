"""오른쪽 패널 아래 셋(웨이브 시작 · 스킬 트리 · 칵테일 칸) 크기 맞추기 시안. 화면 x 1044 ~ 1264 (폭 220), y 334 ~ 582 를 1.3배로

    python tools/neon/rpanel_mock.py 출력폴더   → rpanel_mock.png

지금: 시작 52 (모서리 14) · 사이 10 · 트리 36 (알약) · 사이 12 · 칵테일 138 (모서리 12). 글자 16 / 13 / 12
A 같은 버튼: 시작 · 트리 둘 다 높이 44 · 모서리 12 · 사이 10 똑같이, 글자 15 / 15. 칵테일이 154 로 늘어남
B 같은 높이 52: 트리도 52 (포인트 꼬리표가 넉넉), 사이 10, 글자 16 / 15. 칵테일 126
C 셋을 한 틀 규칙으로: 모서리 12 두 겹 틀 통일, 시작 52 · 트리 40 · 사이 10 똑같이, 칵테일 머리글을 트리 글자와 같은 13 굵게
"""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, PINK, CYAN, GOLD, WHITE, FONT_KO, hx  # noqa: E402

LAV, DIM, FG, BULB = hx('a896e1'), hx('5f6878'), hx('cfd8e6'), hx('ffe196')
Z, OX, OY = 1.3, 1044.0, 334.0
PW = 220


def frame2(M, S, x, y, w, h, r, c, b=1.0):
    M.rr(*S(x, y), w * Z, h * Z, r * Z, c, 1.6, b)
    M.rr(*S(x + 4, y + 4), (w - 8) * Z, (h - 8) * Z, max(2, r - 4) * Z, c, 0.8, b * 0.5)


def glass(M, cx, cy, s, c):
    M.ln([(cx - 9 * s, cy - 10 * s), (cx + 9 * s, cy - 10 * s), (cx, cy), (cx - 9 * s, cy - 10 * s)], c, 1.2, 0.8)
    M.ln([(cx, cy), (cx, cy + 9 * s)], c, 1.2, 0.8)
    M.ln([(cx - 5 * s, cy + 9 * s), (cx + 5 * s, cy + 9 * s)], c, 1.2, 0.8)


def draw(M, px, py, start_h, gap1, tree_h, gap2, start_r, tree_r, start_px, tree_px, charm_px, cap):
    S = lambda x, y: (px + (x - OX) * Z, py + (y - OY) * Z)  # noqa: E731
    floor = 582
    y = OY
    # 웨이브 시작 (마키 전구)
    frame2(M, S, OX, y, PW, start_h, start_r, CYAN)
    for k in range(11):
        bx, by = S(OX + 20 + k * (PW - 40) / 10, y + 8)
        M.ln([(bx - 1, by), (bx + 1, by)], BULB, 3.2, 1.0 if k % 3 else 0.3)
    M.word(*S(OX + PW / 2, y + start_h / 2 + 4 + start_px * 0.25), '웨이브 20 시작', WHITE, start_px * Z, font=FONT_KO)
    M.word(*S(OX + PW - 10, y + start_h - 7), 'Space', DIM, 9 * Z, font=FONT_KO, anchor_center=False) if False else None
    y += start_h + gap1
    # 스킬 트리 (포인트 꼬리표)
    frame2(M, S, OX, y, PW, tree_h, tree_r, PINK)
    tx, ty = S(OX + 12, y + tree_h / 2 - 8)
    M.rr(tx, ty, 62 * Z, 16 * Z, 8 * Z, GOLD, 1.0, 0.8)
    M.word(tx + 31 * Z, ty + 12 * Z, '포인트 299', GOLD, 9 * Z, font=FONT_KO)
    M.word(*S(OX + 122, y + tree_h / 2 + tree_px * 0.36), '스킬 트리', WHITE, tree_px * Z, font=FONT_KO)
    y += tree_h + gap2
    # 칵테일 칸 (바닥선까지)
    ch = floor - y
    frame2(M, S, OX, y, PW, ch, 12, LAV, 0.7)
    M.word(*S(OX + 14, y + 22), '칵테일 0/5', CYAN, charm_px * Z, font=FONT_KO, anchor_center=False)
    M.word(*S(OX + PW - 14, y + 22), '다음 20웨이브', DIM, 9 * Z, font=FONT_KO, anchor_center=False) if False else M.word(S(OX + PW - 52, 0)[0], S(0, y + 22)[1], '다음 20웨이브', DIM, 9 * Z, font=FONT_KO)
    for k in range(5):
        cx, cy = S(OX + 26 + k * (PW - 52) / 4, y + 52)
        glass(M, cx, cy, Z, LAV)
    M.ln([S(OX + 14, y + 72), S(OX + PW - 14, y + 72)], LAV, 1.0, 0.5)
    M.word(*S(OX + 14, y + 92), '5웨이브마다 하나를 고른다', DIM, 10 * Z, font=FONT_KO, anchor_center=False)
    M.word(px + PW * Z / 2, py - 22, cap, WHITE, 15, font=FONT_KO)
    M.word(px + PW * Z / 2, py + (floor - OY) * Z + 28, f'시작 {start_h} · 트리 {tree_h} · 칵테일 {ch:.0f}', DIM, 12, font=FONT_KO)


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    M = Sign(0.8)
    M.small_glow = 0.0
    step = PW * Z + 30
    draw(M, 24, 60, 52, 10, 36, 12, 14, 18, 16, 13, 12, '지금')
    draw(M, 24 + step, 60, 44, 10, 44, 10, 12, 12, 15, 15, 12, 'A 같은 버튼 44')
    draw(M, 24 + step * 2, 60, 52, 10, 52, 10, 12, 12, 16, 15, 12, 'B 같은 높이 52')
    draw(M, 24 + step * 3, 60, 52, 10, 40, 10, 12, 12, 16, 13, 13, 'C 틀 규칙 통일')
    M.draw().save(os.path.join(outdir, 'rpanel_mock.png'))
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
