"""아치 주크박스가 판 윗테두리(연보라 틀, y 92)에 걸리는 문제 (시안). 주크박스 모양은 지금 그대로

    python tools/neon/jukebox_shelf_mock.py 출력폴더   → jukebox_shelf_mock.png

지금: 본체 y 2 ~ 96, 받침(y 90 ~ 96)이 판 틀 선(y 92)을 가로질러 겹쳐 보인다
1 선반 위: 자기 받침을 빼고 2px 내려 몸통 바닥을 판 틀 선(y 92)에 딱 맞춤 → 판 틀 선이 받침. 그 구간만 선을 밝고 두껍게(선반) + 작은 다리 둘
2 줄이기: 0.85배, 바닥 y 82 (판 틀 선 위 10px 띄움). 가운데 자리는 그대로
화면 좌표 x 250 ~ 450, y 0 ~ 150 을 2배로
"""
import os
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(__file__))
import jukebox_mock as J  # noqa: E402
from logo_mock import Sign, PINK, CYAN, WHITE, FONT_KO, FONT_EN, hx  # noqa: E402

J.KO = FONT_KO
LAV, DIM = hx('a896e1'), hx('5f6878')
Z, OX, OY = 2.0, 250.0, 0.0
PANEL_W, GAP, TOP = 400, 30, 90
FRAME_Y = 92.0  # 판 틀 윗선 (FY - 6)
JX0, JY0, JW, JH = 297.0, 2.0, 128.0, 94.0


def S(px, x, y):  # 화면 좌표 → 시안 좌표
    return (px + (x - OX) * Z, TOP + (y - OY) * Z)


def board(M, px, gap=None, shelf=False):
    """판 틀 윗선 (왼쪽 둥근 모서리부터 오른쪽 끝까지) + 첫 줄 카드"""
    import math
    r = 12
    pts = [S(px, 256, FRAME_Y + 40), S(px, 256, FRAME_Y + r)]
    for k in range(7):
        a = math.radians(180 + k * 15)
        pts.append(S(px, 256 + r + math.cos(a) * r, FRAME_Y + r + math.sin(a) * r))
    if gap:
        M.ln(pts + [S(px, gap[0], FRAME_Y)], LAV, 1.4 * Z * 0.7, 0.5)
        M.ln([S(px, gap[1], FRAME_Y), S(px, 450, FRAME_Y)], LAV, 1.4 * Z * 0.7, 0.5)
    else:
        M.ln(pts + [S(px, 450, FRAME_Y)], LAV, 1.4 * Z * 0.7, 0.5)
    if shelf:  # 선반: 주크박스 밑 구간만 밝고 두껍게 + 양끝 살짝 내려온 받침대
        a, b = JX0 - 6, JX0 + JW + 6
        M.ln([S(px, a, FRAME_Y), S(px, b, FRAME_Y)], LAV, 2.6 * Z * 0.7, 0.95)
        M.ln([S(px, a + 8, FRAME_Y), S(px, a + 12, FRAME_Y + 6)], LAV, 1.2 * Z * 0.7, 0.7)
        M.ln([S(px, b - 8, FRAME_Y), S(px, b - 12, FRAME_Y + 6)], LAV, 1.2 * Z * 0.7, 0.7)
    for i, (s, c) in enumerate((('3', CYAN), ('8', PINK), ('A', WHITE), ('K', PINK))):
        x = 262 + i * 48 + 9
        M.rr(*S(px, x, 104), 30 * Z, 30 * Z, 6 * Z, c, 1.2, 0.7)
        M.word(S(px, x + 15, 0)[0], S(px, 0, 124)[1], s, c, 12 * Z, font=FONT_EN)


def juke(M, T, px, x0, y0, k, nobase=False):
    if nobase:  # 받침(바닥 6px)을 빼면 몸통 바닥 = y0 + JH - 6
        J.jukebox(M, T, *S(px, x0, y0), k * Z, 0.3, 3, -10, 1)
        # 받침 선 지우기: jukebox() 가 그린 마지막 받침 사다리꼴을 찾아 뺀다
        for i in range(len(M.lines) - 1, -1, -1):
            pts = M.lines[i][0]
            if len(pts) == 5 and abs(pts[0][1] - pts[1][1]) < 0.01 and pts[2][1] > pts[0][1] and abs(pts[0][0] - pts[4][0]) < 0.01:
                del M.lines[i]
                break
    else:
        J.jukebox(M, T, *S(px, x0, y0), k * Z, 0.3, 3, -10, 1)


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    M, T = Sign(0.8), []
    M.small_glow = 0.0
    caps = ['지금 — 받침이 판 틀 선에 걸림', '1 선반 위 — 판 틀 선이 받침', '2 줄이기 0.85배 — 선 위로 띄움']
    for n in range(3):
        px = 40 + n * (PANEL_W + GAP)
        M.word(px + PANEL_W / 2, 60, caps[n], WHITE, 15, font=FONT_KO)
        M.rr(px - 6, TOP - 6, PANEL_W + 12, 150 * Z + 12, 10, DIM, 1.0, 0.35)
        if n == 0:
            board(M, px)
            juke(M, T, px, JX0, JY0, 1.0)
        elif n == 1:
            board(M, px, shelf=True)
            juke(M, T, px, JX0, FRAME_Y - (JH - 6), 1.0, nobase=True)
        else:
            board(M, px)
            k = 0.85
            juke(M, T, px, JX0 + JW * (1 - k) / 2, 82 - JH * k, k)
    M.word(640, 470, '1 은 크기 · 모양 그대로, 받침 대신 판 틀이 선반이 된다 (받침만 빼고 2px 내림: 본체 y 4 ~ 92)', DIM, 13, font=FONT_KO)
    M.word(640, 494, '2 는 판 선과 안 닿지만 유리창 안 곡 이름 글자도 0.85배', DIM, 13, font=FONT_KO)
    img = M.draw()
    d = ImageDraw.Draw(img)
    for t in T:
        (x, y, s, c, px_, a), clip = t[:6], (t[6] if len(t) > 6 else None)
        f = ImageFont.truetype(FONT_KO, max(6, px_))
        if clip is None:
            d.text((x, y), s, fill=c, font=f, anchor=a)
            continue
        lay = Image.new('RGB', img.size, (0, 0, 0))
        ImageDraw.Draw(lay).text((x, y), s, fill=c, font=f, anchor=a)
        box = tuple(int(v) for v in clip)
        img.paste(ImageChops.lighter(img.crop(box), lay.crop(box)), box[:2])
    img.crop((0, 30, 1280, 520)).save(os.path.join(outdir, 'jukebox_shelf_mock.png'))
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
