"""주크박스 2안(0.85배, 판 틀 선 위로 띄움)을 옆으로 늘린 시안. 높이는 0.85배(80) 그대로, 폭만 넓힘 → 유리창도 넓어져 곡 이름이 덜 흐른다

    python tools/neon/jukebox_wide_mock.py 출력폴더   → jukebox_wide_mock.png

가운데 자리는 지금 그대로(x 361). 왼쪽 로고 배지(~248) · 판 왼쪽 틀(256) · 오른쪽 WAVE 간판(470) 사이
2   폭 109 (0.85배 그대로)
2a  폭 150
2b  폭 180 (거의 최대: 271 ~ 451)
화면 좌표 x 240 ~ 476 을 1.75배로
"""
import os
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(__file__))
import jukebox_mock as J  # noqa: E402
import jukebox_shelf_mock as SH  # noqa: E402
from logo_mock import Sign, CYAN, WHITE, FONT_KO, hx  # noqa: E402

J.KO = FONT_KO
DIM = hx('5f6878')
SH.Z, SH.OX = 1.75, 240.0
SH.TOP = 100
PANEL_W, GAP = 413, 20
K, CXJ, BOT = 0.85, 361.0, 82.0


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    M, T = Sign(0.8), []
    M.small_glow = 0.0
    for n, (wpx, cap) in enumerate(((109, '2  폭 109 (지금 비율)'), (150, '2a  폭 150'), (180, '2b  폭 180 (거의 최대)'))):
        px = 20 + n * (PANEL_W + GAP)
        M.word(px + PANEL_W / 2, 66, cap, WHITE, 15, font=FONT_KO)
        M.rr(px - 6, SH.TOP - 6, PANEL_W + 12, 150 * SH.Z + 12, 10, DIM, 1.0, 0.35)
        SH.board(M, px)
        J.JW = wpx / K
        x0 = CXJ - wpx / 2
        J.jukebox(M, T, *SH.S(px, x0, BOT - J.JH * K), K * SH.Z, 0.3, 3, -10, 1)
    J.JW = 128
    M.word(640, 470, '높이는 모두 80 (0.85배) · 바닥 y 82 로 판 틀 선(y 92) 위에 뜸. 넓힐수록 유리창이 넓어져 곡 이름이 안 흐르고 다 보임', DIM, 13, font=FONT_KO)
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
    img.crop((0, 40, 1280, 490)).save(os.path.join(outdir, 'jukebox_wide_mock.png'))
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
