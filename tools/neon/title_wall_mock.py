"""시작 화면 간판 벽빛 시안 (D-465): 간판 뒤 벽돌 벽에 비치는 환한 분위기를 넓이 · 세기별로 비교

    python tools/neon/title_wall_mock.py  → DOC/assets/images/title-wall/title_wall_mock.png

게임 PaintTitle · PaintAliceSign 과 같은 자리(TX 110, TW 580)와 색. 간판은 따라 그린 획(tools/ui_art/title_logo_strokes.json),
벽빛 · 관 번짐은 게임 FCanvas::Spill 처럼 (1-t)² 둥근 빛을 반지름 절반 간격으로 겹쳐 찍는다
"""
import json
import math
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(__file__)
ROOT = os.path.join(HERE, '..', '..')
OUT = os.path.join(ROOT, 'DOC', 'assets', 'images', 'title-wall')
W, H = 1280, 720
EDGE = {'cyan': (0x14, 0xd2, 0xeb), 'pink': (0xff, 0x2d, 0xcd), 'yellow': (0xff, 0xd7, 0x14)}
CORE = {'cyan': (0xaf, 0xff, 0xff), 'pink': (0xff, 0xaf, 0xf2), 'yellow': (0xff, 0xfa, 0xa5)}
SRC = json.load(open(os.path.join(HERE, '..', 'ui_art', 'title_logo_strokes.json')))
SRCW = SRC['size'][0]
LK = 1.55
LX, LY = 400.0, H / 2 - 97 * LK
TW = 580.0
K = TW / SRCW
TH = TW * SRC['size'][1] / SRCW
TX, TY = LX - TW / 2, LY + 168 * LK - TH - 4


def strokes():
    for col, lst in SRC['strokes'].items():
        th = SRC['width'][col] * K * 0.8
        for s in lst:
            yield col, th, [(TX + x * K, TY + y * K) for x, y in s['pts']]


def disc_field(acc, cx, cy, r, a):
    """한 둥근 빛의 덮임(알파)을 acc 에 '위에 덮기'로 합친다: acc = 1 - (1-acc)(1-a·(1-t)²)"""
    x0, x1 = max(0, int(cx - r)), min(W, int(cx + r) + 1)
    y0, y1 = max(0, int(cy - r)), min(H, int(cy + r) + 1)
    if x0 >= x1 or y0 >= y1: return
    yy, xx = np.mgrid[y0:y1, x0:x1]
    t = np.sqrt((xx - cx) ** 2 + (yy - cy) ** 2) / r
    f = np.clip(1 - t, 0, 1) ** 2 * a
    acc[y0:y1, x0:x1] = 1 - (1 - acc[y0:y1, x0:x1]) * (1 - f)


def spill(img, pts, col, width, peak):
    """FCanvas::Spill 과 같게: 반지름 width/2, 간격 반지름 절반, 알파 peak × 0.75"""
    r = width / 2; step = r / 2; a1 = peak * 0.75
    acc = np.zeros((H, W), np.float32)
    disc_field(acc, *pts[0], r, a1)
    carry = 0.0
    for (ax, ay), (bx, by) in zip(pts, pts[1:]):
        L = math.hypot(bx - ax, by - ay)
        t = step - carry
        while t <= L:
            u = t / max(L, 1e-4)
            disc_field(acc, ax + (bx - ax) * u, ay + (by - ay) * u, r, a1)
            t += step
        carry = L - (t - step)
    c = np.array(col, np.float32) / 255
    img[:] = img * (1 - acc[..., None]) + c * acc[..., None]


def bricks(img, bw, bh, color, alpha, light=None, gain=0.0):
    """벽돌 선. light(H, W, 3) 가 있으면 선이 그 빛을 받아 밝아진다 (벽돌 선만 비침)"""
    m = Image.new('L', (W, H), 0); d = ImageDraw.Draw(m)
    r = 0
    while 20 + r * bh < H:
        y = 20 + r * bh
        d.line([(0, y), (W, y)], fill=255, width=1)
        k = 0
        while k * bw < W + bw:
            x = k * bw + (bw / 2 if r % 2 else 0)
            d.line([(x, y), (x, y + bh)], fill=255, width=1)
            k += 1
        r += 1
    a = np.asarray(m, np.float32)[..., None] / 255 * alpha
    c = np.array(color, np.float32) / 255
    if light is not None: c = np.clip(c + light * gain, 0, 1)
    img[:] = img * (1 - a) + c * a


def lines(img, pts, col, w):
    m = Image.new('L', (W * 2, H * 2), 0)
    ImageDraw.Draw(m).line([(x * 2, y * 2) for x, y in pts], fill=255, width=max(1, int(w * 2)), joint='curve')
    m = m.resize((W, H), Image.LANCZOS)
    a = np.asarray(m, np.float32)[..., None] / 255
    img[:] = img * (1 - a) + np.array(col, np.float32) / 255 * a


def render(label, wall_w, wall_a, bar_a, wash=None, line_gain=0.0, thick=1.0):
    """wall_w = 관 굵기 배수(벽빛 너비), wall_a = Alice · 잔 세기, bar_a = BAR 세기. wash = (너비, 세기) 넓은 겹 하나 더.
    line_gain > 0 이면 벽빛을 바닥에 칠하지 않고 벽돌 선만 그 빛으로 밝힌다"""
    img = np.zeros((H, W, 3), np.float32); img[:] = np.array((6, 6, 11), np.float32) / 255
    S = list(strokes())
    light = np.zeros((H, W, 3), np.float32)
    for col, th, pts in S:
        st = min(th, 4.5)
        a = bar_a if col == 'cyan' else wall_a
        spill(light, pts, EDGE[col], st * wall_w, a)
        if wash: spill(light, pts, EDGE[col], st * wash[0], wash[1])
    if line_gain > 0:
        bricks(img, 192, 102, (0x3c, 0x30, 0x3a), 0.6, light, line_gain)
        img += light * 0.25  # 바닥에는 옅게만
    else:
        bricks(img, 192, 102, (0x3c, 0x30, 0x3a), 0.6)
        img = 1 - (1 - img) * (1 - light)  # 벽빛을 위에 (Spill 은 알파 덮기라 밝은 쪽으로 쌓인다)
    for col, th, pts in S:  # 관: 번짐 둘 + 색 관 + 파스텔 심 (D-464 시작 화면 값), thick = 글자 굵기 배수
        th *= thick
        spill(img, pts, EDGE[col], th * 15, 0.16)
        spill(img, pts, EDGE[col], th * 6.5, 0.55)
        lines(img, pts, EDGE[col], th)
        lines(img, pts, CORE[col], th * 0.6)
    im = Image.fromarray((np.clip(img, 0, 1) * 255).astype(np.uint8))
    im = im.crop((0, 60, 820, 560))
    d = ImageDraw.Draw(im)
    try: f = ImageFont.truetype('malgun.ttf', 26)
    except OSError: f = ImageFont.load_default()
    d.text((18, 12), label, fill=(235, 235, 235), font=f)
    return im


def brick_albedo(bw=192, bh=102, mortar=3, seed=7):
    """벽돌 면 반사율: 벽돌마다 조금씩 다른 밝기 + 잔 얼룩, 줄눈은 어둡게 (빛을 받으면 벽돌 무늬가 드러난다)"""
    rng = np.random.default_rng(seed)
    al = np.zeros((H, W), np.float32)
    r = -1
    while -20 + r * bh < H:
        y0 = 20 + r * bh
        k = -1
        while k * bw < W + bw:
            x0 = k * bw + (bw / 2 if r % 2 else 0)
            v = rng.uniform(0.7, 1.0)
            ya, yb = int(max(0, y0 + mortar)), int(min(H, y0 + bh - mortar + 1))
            xa, xb = int(max(0, x0 + mortar)), int(min(W, x0 + bw - mortar + 1))
            if ya < yb and xa < xb: al[ya:yb, xa:xb] = v
            k += 1
        r += 1
    noise = rng.normal(0, 1, (H // 24, W // 24)).astype(np.float32)
    noise = np.asarray(Image.fromarray(noise, 'F').resize((W, H), Image.BICUBIC))
    al = al * (1 + 0.05 * noise)
    al[al == 0] = 0.12  # 줄눈
    return np.clip(al, 0, 1.2)


def lamp_light(S, radii, gains):
    """전구를 켠 듯 벽면을 비추는 빛: 관 모양을 색별로 그려 크게 흐린다 (가까울수록 밝고 멀어질수록 부드럽게 옅어짐)"""
    from PIL import ImageFilter
    light = np.zeros((H, W, 3), np.float32)
    for col in EDGE:
        m = Image.new('L', (W, H), 0); d = ImageDraw.Draw(m)
        for c, th, pts in S:
            if c == col: d.line(pts, fill=255, width=max(1, int(th * 2)), joint='curve')
        mf = np.asarray(m, np.float32) / 255  # 실수로 흐려 계단 무늬 없게 (FFT 가우스, 가장자리 여백)
        P = 600
        F = np.fft.rfft2(np.pad(mf, P))
        fy = np.fft.fftfreq(H + 2 * P)[:, None]; fx = np.fft.rfftfreq(W + 2 * P)[None, :]
        for rad, g in zip(radii, gains):
            G = np.exp(-2 * (math.pi * rad) ** 2 * (fx ** 2 + fy ** 2))
            b = np.fft.irfft2(F * G, s=(H + 2 * P, W + 2 * P))[P:P + H, P:P + W].astype(np.float32)
            light += b[..., None] * (np.array(EDGE[col], np.float32) / 255) * g
    return light


def render_lamp(label, radii, gains, thick=1.5):
    img = np.zeros((H, W, 3), np.float32); img[:] = np.array((6, 6, 11), np.float32) / 255
    S = list(strokes())
    bricks(img, 192, 102, (0x3c, 0x30, 0x3a), 0.45)
    light = lamp_light(S, radii, gains)
    img = img + light * brick_albedo()[..., None]  # 빛 × 벽돌 반사율: 벽돌 면은 밝게, 줄눈은 어둡게
    for col, th, pts in S:  # 번짐은 원래 굵기 기준, 관 선(색 관 + 심)만 thick 배
        spill(img, pts, EDGE[col], th * 15, 0.16)
        spill(img, pts, EDGE[col], th * 6.5, 0.55)
        lines(img, pts, EDGE[col], th * thick)
        lines(img, pts, CORE[col], th * thick * 0.6)
    im = Image.fromarray((np.clip(img, 0, 1) * 255).astype(np.uint8)).crop((0, 60, 820, 560))
    d = ImageDraw.Draw(im)
    try: f = ImageFont.truetype('malgun.ttf', 26)
    except OSError: f = ImageFont.load_default()
    d.text((18, 12), label, fill=(235, 235, 235), font=f)
    return im


def main_lamp():
    os.makedirs(OUT, exist_ok=True)
    panels = [
        render_lamp('관 선 원래 굵기', (60, 180), (2.0, 3.6), thick=1.0),
        render_lamp('관 선 1.5배', (60, 180), (2.0, 3.6), thick=1.5),
        render_lamp('관 선 1.8배', (60, 180), (2.0, 3.6), thick=1.8),
        render_lamp('관 선 2배', (60, 180), (2.0, 3.6), thick=2.0),
    ]
    w, h = panels[0].size
    sheet = Image.new('RGB', (w * 2 + 6, h * 2 + 6), (40, 40, 40))
    for i, p in enumerate(panels): sheet.paste(p, ((i % 2) * (w + 6), (i // 2) * (h + 6)))
    sheet.save(os.path.join(OUT, 'title_wall_lamp.png'))
    print(os.path.join(OUT, 'title_wall_lamp.png'))


def main():
    os.makedirs(OUT, exist_ok=True)
    panels = [
        render('C 넓고 환하게', 48, 0.16, 0.24, wash=(120, 0.05)),
        render('D 벽돌 줄눈이 빛을 받음', 48, 0.3, 0.4, line_gain=2.2),
        render('C + 글자 1.3배 굵게', 48, 0.16, 0.24, wash=(120, 0.05), thick=1.3),
        render('D + 글자 1.3배 굵게', 48, 0.3, 0.4, line_gain=2.2, thick=1.3),
    ]
    w, h = panels[0].size
    sheet = Image.new('RGB', (w * 2 + 6, h * 2 + 6), (40, 40, 40))
    for i, p in enumerate(panels): sheet.paste(p, ((i % 2) * (w + 6), (i // 2) * (h + 6)))
    sheet.save(os.path.join(OUT, 'title_wall_mock.png'))
    print(os.path.join(OUT, 'title_wall_mock.png'))


if __name__ == '__main__':
    import sys
    main_lamp() if 'lamp' in sys.argv else main()
