"""NEON CITY PALLAS 로고 시안 — 네온 간판 (켜짐 · 깜빡임 · 합선 불꽃)

    python tools/neon/logo_mock.py 출력폴더

A 간판: 두 겹 네온 틀 + 볼트, 위 NEON CITY(하늘), 아래 PALLAS(분홍), 왼쪽 스페이드 왕관
B 스카이라인: 도시 실루엣 위로 PALLAS(금), 아래 NEON CITY(하늘)
C 카드 엠블럼: 큰 카드 틀 안에 스페이드 퀸(왕관 + 스페이드), 아래 이름
A 는 켜짐 → 깜빡임 → 합선 불꽃 단계와 움직이는 GIF 도 만든다.
"""
import math
import os
import random
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

sys.path.insert(0, os.path.dirname(__file__))
from neon import suit, circle  # noqa: E402

W, H, S = 1280, 720, 2
BG = (6, 6, 11)
FONT_EN = 'C:/Windows/Fonts/bahnschrift.ttf'
FONT_KO = 'C:/Windows/Fonts/malgunbd.ttf'
PINK, CYAN, GOLD, WHITE = (255, 106, 170), (110, 220, 255), (255, 210, 110), (255, 255, 255)


def hx(h): return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


class Sign:
    """선(빛번짐) · 글자 튜브 · 불꽃을 모았다가 한 번에 그린다. 밝기 b (0 꺼짐 ~ 1 켜짐)"""
    def __init__(self):
        self.lines, self.texts, self.dots = [], [], []

    def ln(self, pts, c, wd=2.0, b=1.0): self.lines.append((pts, c, wd, b))
    def rr(self, x, y, w, h, r, c, wd=2.0, b=1.0):
        P = []
        for (cx, cy, a0) in ((x + w - r, y + r, -90), (x + w - r, y + h - r, 0), (x + r, y + h - r, 90), (x + r, y + r, 180)):
            for k in range(7):
                a = math.radians(a0 + k * 15)
                P.append((cx + math.cos(a) * r, cy + math.sin(a) * r))
        P.append(P[0])
        self.ln(P, c, wd, b)

    def word(self, x, y, s, c, px, bright=None, track=0.0, font=FONT_EN, anchor_center=True):
        """글자마다 따로 (bright[i] 로 글자별 밝기). 튜브 = 속이 빈 외곽선"""
        f = ImageFont.truetype(font, int(px * S))
        widths = [f.getlength(ch) / S for ch in s]
        total = sum(widths) + track * (len(s) - 1)
        cx = x - total / 2 if anchor_center else x
        for i, ch in enumerate(s):
            b = 1.0 if bright is None else bright[i]
            self.texts.append((cx, y, ch, c, px, b, font))
            cx += widths[i] + track
        return total

    def spark(self, x, y, n=14, r=40, seed=1):
        rnd = random.Random(seed)
        for _ in range(n):
            a = rnd.uniform(0, math.tau)
            l1, l2 = rnd.uniform(4, 10), rnd.uniform(r * 0.4, r)
            bend = rnd.uniform(-0.4, 0.4)
            p0 = (x + math.cos(a) * l1, y + math.sin(a) * l1)
            pm = (x + math.cos(a + bend * 0.5) * (l1 + l2) * 0.55, y + math.sin(a + bend * 0.5) * (l1 + l2) * 0.55 + rnd.uniform(0, 6))
            p1 = (x + math.cos(a + bend) * (l1 + l2), y + math.sin(a + bend) * (l1 + l2) + rnd.uniform(4, 16))
            self.ln([p0, pm, p1], (255, 236, 170) if rnd.random() < 0.6 else WHITE, rnd.uniform(1.0, 2.0), 1.0)
        for _ in range(n):
            a = rnd.uniform(0, math.tau)
            d = rnd.uniform(r * 0.5, r * 1.5)
            self.dots.append((x + math.cos(a) * d, y + math.sin(a) * d + rnd.uniform(0, 20), rnd.uniform(1.2, 2.6)))
        self.dots.append((x, y, 7))  # 합선 지점의 흰 섬광

    def draw(self):
        img = Image.new('RGB', (W * S, H * S), BG)
        glow = Image.new('RGB', img.size, (0, 0, 0))
        core = Image.new('RGB', img.size, (0, 0, 0))
        gd, cd = ImageDraw.Draw(glow), ImageDraw.Draw(core)
        def dim(c, b): return tuple(int(v * b) for v in c)
        for (pts, c, wd, b) in self.lines:
            P = [(px * S, py * S) for px, py in pts]
            if b > 0.25: gd.line(P, fill=dim(c, b), width=max(1, int(wd * S * 4)))
            cd.line(P, fill=dim(c, max(0.16, b)), width=max(1, int(wd * S)))
        for (x, y, ch, c, px, b, font) in self.texts:
            f = ImageFont.truetype(font, int(px * S))
            tw = max(2, int(px * S * 0.055))
            if b > 0.25: gd.text((x * S, y * S), ch, font=f, fill=dim(c, b), anchor='ls', stroke_width=tw * 3, stroke_fill=dim(c, b))
            # 튜브: 외곽선만 (속은 배경색으로 다시 칠함)
            cd.text((x * S, y * S), ch, font=f, fill=dim(c, max(0.16, b)), anchor='ls', stroke_width=tw, stroke_fill=dim(c, max(0.16, b)))
            if px < 26:  # 작은 글자는 속을 비우지 않는다 (읽기 쉽게)
                cd.text((x * S, y * S), ch, font=f, fill=dim(c, max(0.16, b)), anchor='ls')
                continue
            cd.text((x * S, y * S), ch, font=f, fill=(0, 0, 0), anchor='ls')
            if b > 0.6:  # 튜브 가운데 흰 심
                cd.text((x * S, y * S), ch, font=f, fill=None, anchor='ls', stroke_width=max(1, tw // 3), stroke_fill=dim((255, 255, 255), 0.35 * b))
                cd.text((x * S, y * S), ch, font=f, fill=(0, 0, 0), anchor='ls')
        for (x, y, r) in self.dots:
            gd.ellipse([(x - r * 3) * S, (y - r * 3) * S, (x + r * 3) * S, (y + r * 3) * S], fill=(255, 220, 150))
            cd.ellipse([(x - r) * S, (y - r) * S, (x + r) * S, (y + r) * S], fill=WHITE)
        img = ImageChops.add(img, glow.filter(ImageFilter.GaussianBlur(10 * S)).point(lambda v: int(v * 0.55)))
        img = ImageChops.add(img, glow.filter(ImageFilter.GaussianBlur(3 * S)).point(lambda v: int(v * 0.45)))
        img = ImageChops.add(img, core)
        return img.resize((W, H), Image.LANCZOS)


def spade_crown(M, cx, cy, r, c, b=1.0):
    for p in suit('spade')(cx, cy + r * 0.25, r * 0.8): M.ln(p, c, 2.2, b)
    M.ln([(cx - r * 0.8, cy - r * 0.55), (cx - r * 0.8, cy - r * 1.05), (cx - r * 0.4, cy - r * 0.78), (cx, cy - r * 1.25), (cx + r * 0.4, cy - r * 0.78), (cx + r * 0.8, cy - r * 1.05), (cx + r * 0.8, cy - r * 0.55), (cx - r * 0.8, cy - r * 0.55)], GOLD, 2.0, b)


def logo_A(state='on', t=0.0):
    """state: on / flicker / short / off"""
    M = Sign()
    br_city = [1.0] * 9
    br_pal = [1.0] * 6
    frame = 1.0
    if state == 'flicker':
        br_city = [1, 1, 0.1, 1, 1, 1, 0.1, 1, 1]
        br_pal = [1, 1, 1, 0.08, 1, 1]
        frame = 0.6
    elif state == 'short':
        br_pal = [1, 1, 1, 0.35, 1, 1]
        br_city = [1, 1, 1, 1, 1, 1, 0.1, 1, 1]
    elif state == 'off':
        br_city = [0.0] * 9; br_pal = [0.0] * 6; frame = 0.0
    # 간판 틀 (두 겹) + 볼트 + 매단 줄
    M.rr(250, 170, 780, 360, 26, CYAN, 2.2, frame)
    M.rr(264, 184, 752, 332, 18, PINK, 1.2, frame * 0.8)
    for (x, y) in ((276, 196), (1004, 196), (276, 504), (1004, 504)): M.ln(circle(x, y, 5, 16), GRY, 1.2, 0.5)
    M.ln([(400, 170), (420, 80)], GRY, 1.0, 0.4); M.ln([(880, 170), (860, 80)], GRY, 1.0, 0.4)
    spade_crown(M, 320, 340, 40, PINK, br_pal[0])
    M.word(670, 262, 'NEON CITY', CYAN, 46, br_city, track=10)
    M.ln([(440, 285), (900, 285)], CYAN, 1.4, frame)
    M.word(690, 430, 'PALLAS', PINK, 138, br_pal, track=6)
    M.word(640, 492, '네온 시티 팔라스', GOLD, 22, None, track=4, font=FONT_KO)
    if state == 'short':
        M.spark(668, 355, n=18, r=60, seed=int(t * 10) + 3)
        M.spark(640, 236, n=8, r=30, seed=int(t * 10) + 9)
    return M.draw()


GRY = (138, 148, 166)


def logo_B():
    M = Sign()
    # 스카이라인 (건물 윤곽)
    xs = [180, 230, 230, 290, 290, 330, 330, 380, 380, 450, 450, 500, 500, 560, 560, 620, 620, 660, 660, 720, 720, 780, 780, 850, 850, 900, 900, 960, 960, 1010, 1010, 1100]
    ys = [y - 70 for y in [430, 430, 330, 330, 380, 380, 260, 260, 360, 360, 300, 300, 400, 400, 220, 220, 340, 340, 280, 280, 370, 370, 250, 250, 350, 350, 300, 300, 390, 390, 430, 430]]
    M.ln(list(zip(xs, ys)), CYAN, 1.6, 0.85)
    for (x, y) in ((345, 220), (575, 180), (800, 210), (465, 260)):
        for k in range(3): M.ln([(x, y + k * 20), (x + 10, y + k * 20)], GOLD, 1.4, 0.7)
    spade_crown(M, 590, 100, 30, PINK)
    M.word(640, 500, 'PALLAS', GOLD, 140, None, track=8)
    M.word(640, 560, 'N E O N   C I T Y', CYAN, 32, None, track=2)
    M.word(640, 610, '네온 시티 팔라스', PINK, 20, None, track=4, font=FONT_KO)
    return M.draw()


def logo_C():
    M = Sign()
    M.rr(520, 90, 240, 340, 22, GOLD, 2.2)
    M.rr(534, 104, 212, 312, 14, GOLD, 1.0, 0.6)
    M.word(552, 140, 'Q', GOLD, 40, None, anchor_center=False)
    for p in suit('spade')(560, 160, 10): M.ln(p, GOLD, 1.4)
    spade_crown(M, 640, 280, 70, PINK)
    M.word(640, 520, 'NEON CITY', CYAN, 40, None, track=12)
    M.word(640, 610, 'PALLAS', PINK, 100, None, track=10)
    M.word(640, 650, '네온 시티 팔라스', GOLD, 18, None, track=4, font=FONT_KO)
    return M.draw()


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    for st in ('on', 'flicker', 'short'):
        logo_A(st).save(os.path.join(outdir, 'logo_A_%s.png' % st))
    logo_B().save(os.path.join(outdir, 'logo_B.png'))
    logo_C().save(os.path.join(outdir, 'logo_C.png'))
    # GIF: 꺼짐 → 깜빡깜빡 켜짐 → 켜짐 → 합선(불꽃) → 잠깐 꺼짐 → 켜짐
    seq = [('off', 6), ('flicker', 2), ('off', 2), ('flicker', 2), ('on', 2), ('flicker', 1), ('on', 14), ('short', 1), ('flicker', 1), ('short', 1), ('off', 1), ('short', 1), ('on', 14)]
    frames, durs = [], []
    t = 0.0
    for st, n in seq:
        for _ in range(n):
            frames.append(logo_A(st, t).resize((640, 360), Image.LANCZOS).convert('P', palette=Image.ADAPTIVE, colors=128))
            durs.append(70)
            t += 0.37
    frames[0].save(os.path.join(outdir, 'logo_A_anim.gif'), save_all=True, append_images=frames[1:], duration=durs, loop=0)
    print('ok', len(frames), 'frames')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
