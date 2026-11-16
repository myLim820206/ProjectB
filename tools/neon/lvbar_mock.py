"""계기판 목숨 · 레벨 칸: 경험치 막대를 숫자 글자 높이에 맞춰 굵게 (시안). 화면 x 800 ~ 1136, y 16 ~ 84 를 1.75배로

    python tools/neon/lvbar_mock.py 출력폴더   → lvbar_mock.png

지금: 막대 12 (숫자 24px 보다 얇다)
A 막대 18: 숫자 글자 몸통(대문자 높이 ~17) 과 같은 높이
B 막대 24: 숫자 글자 크기(24) 와 같은 높이, % 를 막대 안에 조금 크게
C 막대 24 + 칸 나눔: B 에 10칸 눈금 (네온 계기판 느낌)
"""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, PINK, GOLD, WHITE, FONT_KO, FONT_EN, hx  # noqa: E402
from neon import suit  # noqa: E402

FG, DIM, FRAME = hx('cfd8e6'), hx('5f6878'), hx('7fd6ff')
Z = 1.75
OX, OY = 800.0, 16.0
PW, PH = 336 * Z, 68 * Z


TICKS, LABELS = [], []  # 눈금 · % 글자도 채움 위에 덧그림
RECTS = []  # 채움은 빛번짐 없이 그린 뒤에 덧칠 (x, y, w, h, 색)


def fill(M, x, y, w, h, c, b):
    RECTS.append((x, y, w, h, tuple(int(v * b) for v in c)))


def panel(M, px, py, bar_h, ticks=False, frac=0.62):
    S = lambda x, y: (px + (x - OX) * Z, py + (y - OY) * Z)  # noqa: E731
    M.rr(*S(800, 16), PW, PH, 10 * Z, FRAME, 1.4, 0.55)
    M.ln([S(800, 26), S(800, 74)], FRAME, 1.0, 0.35)
    M.ln([S(930, 26), S(930, 74)], FRAME, 1.0, 0.35)
    mid = 50
    # 목숨
    for p in suit('heart')(*S(826, mid), 11 * Z): M.ln(p, PINK, 1.6, 1.0)
    M.word(*S(885, mid + 10), '999', PINK, 24 * Z, font=FONT_EN)
    # 레벨
    dx, dy = S(956, mid)
    r = 9 * Z
    M.ln([(dx, dy - r), (dx + r * 0.7, dy), (dx, dy + r), (dx - r * 0.7, dy), (dx, dy - r)], GOLD, 1.5, 1.0)
    M.word(*S(1005, mid + 10), '300', GOLD, 24 * Z, font=FONT_EN)
    bx, bw = 1046, 78
    x0, y0 = S(bx, mid - bar_h / 2)
    w, h = bw * Z, bar_h * Z
    M.rr(x0, y0, w, h, min(h / 2, 6 * Z), GOLD, 1.2, 0.6)
    fill(M, x0 + 3, y0 + 3, (w - 6) * frac, h - 6, GOLD, 0.8)
    if ticks:
        for k in range(1, 10):
            tx = x0 + w * k / 10
            TICKS.append((tx, y0 + 3, y0 + h - 3))
    pc = 9 if bar_h < 18 else 11 if bar_h < 24 else 12
    LABELS.append((x0 + w / 2, y0 + h / 2, '62%', pc * Z))


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    M = Sign(0.8)
    M.small_glow = 0.0
    items = [(12, False, '지금  막대 12'), (18, False, 'A  막대 18 (숫자 몸통 높이)'), (24, False, 'B  막대 24 (숫자 크기)'), (24, True, 'C  막대 24 + 10칸 눈금')]
    for i, (bh, tk, cap) in enumerate(items):
        px = 40 + (i % 2) * (PW + 56)
        py = 70 + (i // 2) * (PH + 110)
        M.word(px + PW / 2, py - 18, cap, WHITE, 16, font=FONT_KO)
        panel(M, px, py, bh, tk)
    M.word(640, 690, '목숨 999 · 레벨 300 은 같은 글자 (24px). 막대는 숫자 오른쪽, % 는 막대 안', DIM, 13, font=FONT_KO)
    from PIL import ImageDraw
    img = M.draw()
    d = ImageDraw.Draw(img)
    for (x, y, w, h, c) in RECTS: d.rounded_rectangle([x, y, x + w, y + h], radius=min(h / 2, 8), fill=c)
    from PIL import ImageFont
    for (tx, ya, yb) in TICKS: d.line([(tx, ya), (tx, yb)], fill=(6, 6, 11), width=2)
    for (cx, cy, s, px_) in LABELS: d.text((cx, cy), s, fill=(255, 255, 255), font=ImageFont.truetype(FONT_EN, int(px_)), anchor='mm', stroke_width=2, stroke_fill=(40, 28, 6))
    img.save(os.path.join(outdir, 'lvbar_mock.png'))
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
