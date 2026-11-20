"""족보 메뉴 접기 · 펼치기 시안 3종 (Alice 인게임 색감, D-420)

    python tools/neon/hands_fold_mock.py 출력폴더   → hands_fold_mock.png

A 묶음 접기  : '5장 · 2~4장 · 조커/먹이' 머리글을 누르면 그 묶음이 접힌다 (▾ 펼침 / ▸ 접힘). 접힌 묶음은 머리글 한 줄 + 판에 있는 족보 색 점 · 개수
B 한 줄 보기 : 오른쪽 위 '설명' 단추로 모든 줄을 이름 ···· 배수 한 줄로 접는다. 마우스를 올린 줄(과 판에 있는 족보 줄)만 설명이 펼쳐진다
C 패널 접기  : 패널 오른쪽 위 ◂ 단추로 패널 전체를 좁은 띠로. 띠에는 로고 잔 · 세로 '족보 메뉴' · 판에 있는 족보 색 점, 누르면(또는 마우스를 올리면) 다시 펼침
상황: 판에 스트레이트 1 · 원 페어 1 · 조커 0, 마우스는 '풀하우스' 줄
"""
import os
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

ROOT = os.path.join(os.path.dirname(__file__), '..', '..')
KO = 'C:/Windows/Fonts/malgun.ttf'
KOB = 'C:/Windows/Fonts/malgunbd.ttf'
S = 2  # 그리는 배율

# Alice 인게임에서 잰 색 (D-405, D-409)
BG = (6, 9, 26)
PANEL0, PANEL1 = (16, 10, 32), (11, 7, 26)
EDGE = (190, 115, 255)
ROWF, ROWE = (19, 14, 32), (140, 135, 130)
HOVF, HOVE = (85, 82, 69), (230, 224, 154)
TXT, TXTH, DESC, DESCH = (228, 224, 205), (240, 238, 210), (165, 160, 180), (202, 198, 187)
HEAD, HEADL = (230, 226, 200), (160, 140, 80)
TITLE = (240, 226, 140)
HANDS = [  # 이름, 배수, 설명, 막대 색(바랜 족보 색), 판에 있는 개수
    ('로열 스트레이트 플러시', 'x2.5', '공격력 x2.5 / 연사 x2.5 / 넓은 범위', (99, 97, 107), 0),
    ('파이브 카드', 'x5', '공격력 x5 (와일드 · 조커가 있어야)', (130, 110, 50), 0),
    ('스트레이트 플러시', 'x2', '공격력 x2 / 연사 x2 / 범위', (120, 60, 110), 0),
    ('풀하우스', 'x1.5', '공격력 x1.5 / 연사 x1.5 / 범위', (142, 80, 44), 0),
    ('플러시 (3~5장)', '', '범위 공격 (장수만큼 넓게)', (80, 50, 130), 0),
    ('스트레이트 (3~5장)', 'x1.5', '연사 x1.5 / x1.75 / x2', (50, 70, 130), 1),
    ('포카드', 'x3.5', '공격력 x3.5', (130, 45, 50), 0),
    ('트리플', 'x2', '공격력 x2', (60, 120, 80), 0),
    ('투 페어', 'x1.5', '공격력 x1.5 / 사거리 x1.25', (60, 100, 130), 0),
    ('원 페어', 'x1.5', '공격력 x1.5', (110, 110, 120), 1),
    ('조커', 'x0', '클릭: 만들 수 있는 조커 목록', (120, 60, 110), 0),
    ('먹이기', 'x0', '클릭: 조커에 족보 먹이기', (120, 60, 110), 0),
]
BRIGHT = {5: (106, 140, 255), 9: (207, 216, 230)}  # 판에 있는 족보 점 색
GROUPS = [('5장', range(0, 6)), ('2~4장', range(6, 10)), ('조커/먹이', range(10, 12))]
HOVER = 3

_F = {}


def F(px, bold=False):
    k = (px, bold)
    if k not in _F: _F[k] = ImageFont.truetype(KOB if bold else KO, int(px * S))
    return _F[k]


class Canvas:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.img = Image.new('RGB', (w * S, h * S), BG)
        self.glow = Image.new('RGB', (w * S, h * S), (0, 0, 0))
        self.d, self.g = ImageDraw.Draw(self.img), ImageDraw.Draw(self.glow)

    def rr(self, x, y, w, h, r, fill=None, edge=None, wd=1.2, glow=0.0):
        box = [x * S, y * S, (x + w) * S, (y + h) * S]
        if fill: self.d.rounded_rectangle(box, r * S, fill=fill)
        if edge:
            self.d.rounded_rectangle(box, r * S, outline=edge, width=max(1, int(wd * S)))
            if glow: self.g.rounded_rectangle(box, r * S, outline=tuple(int(c * glow) for c in edge), width=int(wd * S * 4))

    def grad(self, x, y, w, h, r, c0, c1):
        lay = Image.new('RGB', self.img.size, BG)
        ld = ImageDraw.Draw(lay)
        for i in range(int(h)):
            t = i / max(1, h - 1)
            c = tuple(int(c0[k] + (c1[k] - c0[k]) * t) for k in range(3))
            ld.line([(x * S, (y + i) * S), ((x + w) * S, (y + i) * S)], fill=c, width=S)
        m = Image.new('L', self.img.size, 0)
        ImageDraw.Draw(m).rounded_rectangle([x * S, y * S, (x + w) * S, (y + h) * S], r * S, fill=255)
        self.img.paste(lay, (0, 0), m)

    def ln(self, pts, c, wd=1.2, glow=0.0):
        P = [(px * S, py * S) for px, py in pts]
        self.d.line(P, fill=c, width=max(1, int(wd * S)), joint='curve')
        if glow: self.g.line(P, fill=tuple(int(v * glow) for v in c), width=int(wd * S * 4))

    def t(self, x, y, s, c, px, anchor='ls', bold=False):
        self.d.text((x * S, y * S), s, font=F(px, bold), fill=c, anchor=anchor)
        return F(px, bold).getlength(s) / S

    def dots(self, x0, x1, y, c):
        x = x0
        while x < x1:
            self.d.ellipse([(x - 0.8) * S, (y - 0.8) * S, (x + 0.8) * S, (y + 0.8) * S], fill=c)
            x += 7

    def chevron(self, x, y, down, c):
        if down: self.ln([(x - 4, y - 2), (x, y + 2), (x + 4, y - 2)], c, 1.4)
        else: self.ln([(x - 2, y - 4), (x + 2, y), (x - 2, y + 4)], c, 1.4)

    def done(self):
        out = ImageChops.add(self.img, self.glow.filter(ImageFilter.GaussianBlur(5 * S)))
        return out


def panel(C, x, y, w, h):
    C.grad(x, y, w, h, 14, PANEL0, PANEL1)
    C.rr(x, y, w, h, 14, edge=EDGE, wd=1.6, glow=0.5)
    logo = os.path.join(ROOT, 'Content', 'UIArt', 'logo.png')
    if os.path.exists(logo):
        im = Image.open(logo).convert('RGBA')
        im = im.resize((int(94.7 * S), int(36.5 * S)), Image.LANCZOS)
        base = C.img.crop((int((x + 8.5) * S), int((y + 7.3) * S), int((x + 8.5) * S) + im.width, int((y + 7.3) * S) + im.height)).convert('RGBA')
        a = im.split()[3]
        comp = ImageChops.add(base.convert('RGB'), Image.merge('RGB', im.split()[:3]))  # 미리 곱한 알파
        C.img.paste(Image.composite(comp, base.convert('RGB'), a.point(lambda v: 255 if v > 0 else 0)), (int((x + 8.5) * S), int((y + 7.3) * S)))


def row(C, x, y, w, i, compact=False, expand=False):
    n, mul, desc, bar, cnt = HANDS[i]
    hov = i == HOVER
    h = 31 if (not compact or expand) else 22
    C.rr(x, y, w, h, 6, fill=HOVF if hov else ROWF, edge=HOVE if hov else ROWE, wd=1.1 if hov else 0.9, glow=0.45 if hov else 0.0)
    C.rr(x + 4, y + 5, 3, h - 10, 1.5, fill=bar)
    ny = y + (13 if h == 31 else 15)
    nw = C.t(x + 13, ny, n, TXTH if hov else TXT, 11)
    mw = C.t(x + w - 8, ny, mul, TXTH if hov else TXT, 11.5, 'rs') if mul else 0
    C.dots(x + 18 + nw, x + w - 12 - mw, ny - 4, tuple(int(c * 0.7) for c in TXT))
    if h == 31: C.t(x + 13, y + 25, desc, DESCH if hov else DESC, 8.4)
    return h


def head(C, cx, y, title, chevron=None, x0=None, x1=None, summary=None):
    tw = F(13).getlength(title) / S
    if chevron is not None:
        C.chevron(cx - tw / 2 - 12, y - 1, chevron, HEAD)
    C.t(cx, y + 5, title, HEAD, 13, 'ms')
    lx0 = cx - tw / 2 - (22 if chevron is not None else 10)
    C.ln([(x0, y), (lx0, y)], HEADL, 1.4, 0.3)
    rx0 = cx + tw / 2 + 10
    if summary:  # 접힌 묶음: 판에 있는 족보 색 점 + 개수
        sx = rx0 + 4
        for col in summary[0]:
            C.d.ellipse([(sx - 3) * S, (y - 3) * S, (sx + 3) * S, (y + 3) * S], fill=col)
            sx += 9
        C.t(sx + 2, y + 4, summary[1], DESC, 8.5)
        sx += F(8.5).getlength(summary[1]) / S + 8
        C.ln([(sx, y), (x1, y)], HEADL, 1.4, 0.3)
    else:
        C.ln([(rx0, y), (x1, y)], HEADL, 1.4, 0.3)


def title(C, x, y, w):
    C.t(x + w / 2 + 4, y + 66, '족보 메뉴', TITLE, 24, 'ms', True)


def footer(C, x, y):
    C.t(x + 16, y + 690, '연쇄', TXT, 11.5, bold=True)
    C.t(x + 48, y + 690, '족보끼리 맞닿을 경우, 개당 +25%', DESC, 9)


def mock_A(C, x, y):
    W, H = 240, 700
    panel(C, x, y, W, H)
    title(C, x, y, W)
    folded = {1}
    yy = y + 104
    for gi, (gname, idx) in enumerate(GROUPS):
        if gi in folded:
            on = [BRIGHT[i] for i in idx if HANDS[i][4]]
            head(C, x + W / 2 + 1, yy, gname, False, x + 14, x + W - 14, (on, '판 %d · 접힘' % len(on)))
            yy += 34
            continue
        head(C, x + W / 2 + 1, yy, gname, True, x + 14, x + W - 14)
        yy += 19
        for i in idx:
            row(C, x + 12, yy, W - 22, i)
            yy += 37.4
        yy += 20
    C.t(x + W / 2, yy + 8, '머리글을 누르면 그 묶음을 접는다', DESC, 8.5, 'ms')
    footer(C, x, y)


def mock_B(C, x, y):
    W, H = 240, 700
    panel(C, x, y, W, H)
    title(C, x, y, W)
    # 오른쪽 위 '설명' 단추 (접힘 상태)
    C.rr(x + W - 66, y + 86, 52, 18, 9, fill=(30, 24, 44), edge=(150, 140, 170), wd=1.0)
    C.t(x + W - 48, y + 99, '설명', TXT, 9, 'ms')
    C.chevron(x + W - 26, y + 95, False, TXT)
    yy = y + 118
    for gname, idx in GROUPS:
        head(C, x + W / 2 + 1, yy, gname, None, x + 14, x + W - 14)
        yy += 16
        for i in idx:
            ex = i == HOVER or HANDS[i][4] > 0
            yy += row(C, x + 12, yy, W - 22, i, compact=True, expand=ex) + 5
        yy += 18
    C.t(x + W / 2, yy + 6, '한 줄 보기: 마우스 줄 · 판에 있는 족보만 설명', DESC, 8.5, 'ms')
    footer(C, x, y)


def mock_C(C, x, y):
    # 접힌 띠 + 옆에 판 일부 (자리 감)
    W, H = 56, 700
    C.grad(x, y, W, H, 14, PANEL0, PANEL1)
    C.rr(x, y, W, H, 14, edge=EDGE, wd=1.6, glow=0.5)
    # 로고 잔
    gx, gy = x + W / 2, y + 30
    C.ln([(gx - 10, gy - 8), (gx + 10, gy - 8), (gx, gy + 4), (gx - 10, gy - 8)], (90, 225, 255), 1.4, 0.5)
    C.ln([(gx, gy + 4), (gx, gy + 13)], (90, 225, 255), 1.3)
    C.ln([(gx - 6, gy + 13), (gx + 6, gy + 13)], (90, 225, 255), 1.3)
    # 펼치기 단추
    C.rr(x + 12, y + 56, 32, 24, 8, fill=(30, 24, 44), edge=(150, 140, 170), wd=1.0)
    C.chevron(x + W / 2, y + 68, False, TXT)
    # 세로 글자
    for k, ch in enumerate('족보메뉴'):
        C.t(x + W / 2, y + 112 + k * 26, ch, TITLE, 17, 'ms', True)
    # 판에 있는 족보 (색 점 + 짧은 이름)
    yy = y + 240
    for i in (5, 9):
        C.d.ellipse([(x + W / 2 - 6) * S, (yy - 6) * S, (x + W / 2 + 6) * S, (yy + 6) * S], outline=BRIGHT[i], width=2 * S)
        C.t(x + W / 2, yy + 22, ['스트', '페어'][0 if i == 5 else 1], TXT, 9, 'ms')
        yy += 44
    C.t(x + W / 2, y + 690, '연쇄', TXT, 10, 'ms', True)
    # 판 일부 (펼쳐진 패널이 덮을 자리)
    bx = x + W + 14
    C.rr(bx, y + 76, 210, 480, 8, edge=(70, 200, 230), wd=1.4, glow=0.4)
    for k in range(1, 5): C.ln([(bx + k * 48, y + 76), (bx + k * 48, y + 556)], (30, 40, 60), 1)
    for k in range(1, 10): C.ln([(bx, y + 76 + k * 48), (bx + 210, y + 76 + k * 48)], (30, 40, 60), 1)
    C.t(bx + 105, y + 600, '접으면 판 왼쪽이 넓어진다', DESC, 9, 'ms')
    C.t(bx + 105, y + 616, '(판을 왼쪽으로 옮기거나, 펼친 패널이 판 위를 덮음)', DESC, 8, 'ms')


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    C = Canvas(860, 760)
    C.t(130, 30, 'A 묶음 접기', TXT, 15, 'ms', True)
    C.t(430, 30, 'B 한 줄 보기', TXT, 15, 'ms', True)
    C.t(720, 30, 'C 패널 접기', TXT, 15, 'ms', True)
    mock_A(C, 10, 50)
    mock_B(C, 310, 50)
    mock_C(C, 580, 50)
    C.done().save(os.path.join(outdir, 'hands_fold_mock.png'))
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
