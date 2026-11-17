"""JOLLY BALDY 회사 로고 시안 — 색을 채운 평면 로고 (네온 화풍 아님)

    python tools/neon/logo_jollybaldy.py 출력폴더

A 기본형: 동그란 대머리 + 웃는 눈 + 활짝 웃는 입 + 정수리 광
B 한 가닥: A + 정수리 꼬불 머리카락 한 가닥
C 배지: 노란 동그라미 배지 안에 A
어두운 바탕 / 흰 바탕 둘 다 그린다.
"""
import math
import os
import sys

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(__file__))
from neon import bez  # noqa: E402

W, H, S = 1280, 720, 4
SKIN, SKIN_SH = (247, 200, 160), (232, 172, 132)
INK = (52, 34, 28)
CHEEK = (242, 140, 128)
MOUTH, TONGUE = (122, 34, 48), (240, 110, 120)
SHINE = (255, 255, 255)
YELLOW = (255, 201, 60)
FONT = next((p for p in ['C:/Windows/Fonts/seguibl.ttf', 'C:/Windows/Fonts/ariblk.ttf', 'C:/Windows/Fonts/arialbd.ttf'] if os.path.exists(p)), None)


def sp(pts):
    return [(x * S, y * S) for x, y in pts]


def stroke(d, pts, col, w):
    """둥근 끝 굵은 선"""
    P = sp(pts)
    d.line(P, fill=col, width=int(w * S), joint='curve')
    r = w * S / 2
    for x, y in (P[0], P[-1]):
        d.ellipse([x - r, y - r, x + r, y + r], fill=col)


def head_shape(cx, cy, R):
    P = lambda x, y: (cx + x * R, cy + y * R)
    out = []
    for k in range(121):
        t = k / 120 * 2 * math.pi
        s = math.sin(t)
        out.append(P(math.cos(t) * (0.84 if s < 0 else 0.84 - 0.08 * s), s * (1.0 if s < 0 else 0.9)))
    return out


def sparkle(cx, cy, r, inner=0.25):
    pts = []
    for k in range(8):
        a = math.radians(-90 + k * 45)
        rr = r if k % 2 == 0 else r * inner
        pts.append((cx + math.cos(a) * rr, cy + math.sin(a) * rr))
    return pts


def draw_head(img, cx, cy, R, hair=False):
    d = ImageDraw.Draw(img)
    P = lambda x, y: (cx + x * R, cy + y * R)
    ow = R * 0.055  # 테두리 굵기
    # 귀 (머리 뒤)
    for sg in (-1, 1):
        ex, ey = P(sg * 0.86, 0.12)
        d.ellipse(sp([(ex - 0.17 * R, ey - 0.2 * R), (ex + 0.17 * R, ey + 0.2 * R)]), fill=SKIN, outline=INK, width=int(ow * S))
        stroke(d, bez(P(sg * 0.86, 0.04), P(sg * 0.95, 0.12), P(sg * 0.87, 0.21), 8), SKIN_SH, ow * 0.9)
    # 머리
    hd = head_shape(cx, cy, R)
    d.polygon(sp(hd), fill=SKIN)
    # 아래쪽 그림자 (턱 쪽 반달)
    mask = Image.new('L', img.size, 0)
    ImageDraw.Draw(mask).polygon(sp(hd), fill=255)
    sh = Image.new('L', img.size, 0)
    ImageDraw.Draw(sh).ellipse(sp([P(-1.3, -0.95), P(0.95, 0.72)]), fill=255)
    shade = Image.composite(Image.new('L', img.size, 0), mask, sh)
    img.paste(SKIN_SH, mask=shade)
    d.line(sp(hd), fill=INK, width=int(ow * S), joint='curve')
    # 정수리 광: 하얀 타원 + 반짝 별
    d.ellipse(sp([P(-0.56, -0.86), P(-0.22, -0.62)]), fill=SHINE)
    d.polygon(sp(sparkle(*P(-0.08, -0.82), 0.13 * R)), fill=SHINE)
    # 눈썹 + 웃는 눈
    for sg in (-1, 1):
        stroke(d, bez(P(sg * 0.18, -0.32), P(sg * 0.32, -0.44), P(sg * 0.47, -0.34), 10), INK, ow * 0.9)
        stroke(d, bez(P(sg * 0.2, -0.1), P(sg * 0.32, -0.26), P(sg * 0.44, -0.1), 10), INK, ow * 1.1)
    # 볼
    for sg in (-1, 1):
        x, y = P(sg * 0.52, 0.16)
        d.ellipse(sp([(x - 0.13 * R, y - 0.08 * R), (x + 0.13 * R, y + 0.08 * R)]), fill=CHEEK)
    # 코
    stroke(d, bez(P(0.0, -0.02), P(0.13, 0.14), P(-0.03, 0.17), 8), INK, ow * 0.85)
    # 입: 어두운 속 + 혀 (입 모양으로 잘라 냄) + 윗니
    mouth = bez(P(-0.44, 0.27), P(0, 0.37), P(0.44, 0.27), 16) + bez(P(0.44, 0.27), P(0, 0.88), P(-0.44, 0.27), 20)[1:]
    mm = Image.new('L', img.size, 0)
    ImageDraw.Draw(mm).polygon(sp(mouth), fill=255)
    inner = Image.new('RGB', img.size, MOUTH)
    di = ImageDraw.Draw(inner)
    di.ellipse(sp([P(-0.28, 0.48), P(0.28, 0.8)]), fill=TONGUE)
    di.polygon(sp(bez(P(-0.46, 0.25), P(0, 0.37), P(0.46, 0.25), 16) + bez(P(0.4, 0.33), P(0, 0.47), P(-0.4, 0.33), 16)), fill=(255, 255, 255))
    img.paste(inner, mask=mm)
    d.line(sp(mouth + [mouth[0]]), fill=INK, width=int(ow * S), joint='curve')
    if hair:
        stroke(d, bez(P(0.06, -0.98), P(0.0, -1.22), P(0.18, -1.3), 12) + bez(P(0.18, -1.3), P(0.36, -1.32), P(0.32, -1.16), 10)[1:]
               + bez(P(0.32, -1.16), P(0.28, -1.06), P(0.2, -1.12), 6)[1:], INK, ow)


def wordmark(img, cx, y, px, dark):
    d = ImageDraw.Draw(img)
    f = ImageFont.truetype(FONT, int(px * S))
    a, b = 'JOLLY ', 'BALDY'
    wa, wb = f.getlength(a), f.getlength(b)
    x0 = cx * S - (wa + wb) / 2
    d.text((x0, y * S), a, font=f, fill=YELLOW, anchor='ls')
    d.text((x0 + wa, y * S), b, font=f, fill=(255, 255, 255) if dark else INK, anchor='ls')


def logo(variant='A', dark=True):
    bg = (16, 16, 24) if dark else (255, 255, 255)
    img = Image.new('RGB', (W * S, H * S), bg)
    cx, cy, R = 640, 290, 160
    if variant == 'B':
        cy += 22
    if variant == 'C':
        d = ImageDraw.Draw(img)
        d.ellipse(sp([(cx - 215, cy - 215), (cx + 215, cy + 215)]), fill=YELLOW)
        R = 145
        cy += 8
    draw_head(img, cx, cy, R, hair=(variant == 'B'))
    wordmark(img, 640, 640, 88, dark)
    return img.resize((W, H), Image.LANCZOS)


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    sheet = Image.new('RGB', (1920, 720))
    for i, v in enumerate('ABC'):
        for j, dark in enumerate((True, False)):
            im = logo(v, dark)
            if v == 'A':
                im.save(os.path.join(outdir, 'jollybaldy_A_%s.png' % ('dark' if dark else 'light')))
            sheet.paste(im.resize((640, 360), Image.LANCZOS), (i * 640, j * 360))
    sheet.save(os.path.join(outdir, 'jollybaldy_sheet.png'))
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
