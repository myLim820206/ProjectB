"""JOLLY BALDY 회사 로고 — 글자 아트 시안 (평면, 네온 아님)

    python tools/neon/logo_jollybaldy_type.py 출력폴더

A 'O' 머리: JOLLY 의 O 가 반짝이는 대머리 얼굴
B 글자 얼굴: JOLLY 가 둥근 정수리로 휘고, ^ ^ 눈 + 누운 D 입, 아래 BALDY
C 글자 그림: 대머리 모양 안을 JOLLYBALDY 글자로 채우고 눈·입·광은 빈칸으로
"""
import math
import os
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFont

W, H, S = 1280, 720, 3
BG = (16, 16, 24)
YELLOW, CREAM, WHITE, INK = (255, 201, 60), (255, 236, 200), (255, 255, 255), (16, 16, 24)
SKIN, PINKC = (247, 200, 160), (242, 140, 128)
FONT = next(p for p in ['C:/Windows/Fonts/seguibl.ttf', 'C:/Windows/Fonts/ariblk.ttf', 'C:/Windows/Fonts/arialbd.ttf'] if os.path.exists(p))


def F(px):
    return ImageFont.truetype(FONT, int(px * S))


def sparkle(cx, cy, r, inner=0.25):
    return [(cx + math.cos(math.radians(-90 + k * 45)) * (r if k % 2 == 0 else r * inner),
             cy + math.sin(math.radians(-90 + k * 45)) * (r if k % 2 == 0 else r * inner)) for k in range(8)]


def sc(pts):
    return [(x * S, y * S) for x, y in pts]


def face(d, cx, cy, r, line):
    """동그란 얼굴 위에 웃는 눈 · 입 · 광 (r = 반지름, 설계 px)"""
    w = max(2, int(r * 0.11 * S))
    for sg in (-1, 1):
        ex, ey = cx + sg * r * 0.36, cy - r * 0.05
        d.arc(sc([(ex - r * 0.17, ey - r * 0.14), (ex + r * 0.17, ey + r * 0.2)]), 200, 340, fill=line, width=w)
    d.chord(sc([(cx - r * 0.5, cy + r * 0.0), (cx + r * 0.5, cy + r * 0.72)]), 0, 180, fill=line)
    d.ellipse(sc([(cx - r * 0.62, cy - r * 0.8), (cx - r * 0.22, cy - r * 0.52)]), fill=WHITE)
    d.polygon(sc(sparkle(cx - r * 0.02, cy - r * 0.78, r * 0.16)), fill=WHITE)


def text_w(s, px, track=0.0):
    f = F(px)
    return sum(f.getlength(c) for c in s) / S + track * (len(s) - 1)


def draw_text(d, x, y, s, px, col, track=0.0):
    f = F(px)
    for c in s:
        d.text((x * S, y * S), c, font=f, fill=col, anchor='ls')
        x += f.getlength(c) / S + track
    return x


def logo_A():
    img = Image.new('RGB', (W * S, H * S), BG)
    d = ImageDraw.Draw(img)
    px, base = 150, 420
    f = F(px)
    cap = (f.getbbox('L', anchor='ls')[3] - f.getbbox('L', anchor='ls')[1]) / S
    r = cap * 0.62
    total = text_w('J', px) + r * 2 + 10 + text_w('LLY', px)
    x = 640 - total / 2
    x = draw_text(d, x, base, 'J', px, CREAM) + 4
    cx, cy = x + r, base - cap / 2
    d.ellipse(sc([(cx - r, cy - r), (cx + r, cy + r)]), fill=YELLOW)
    face(d, cx, cy, r, INK)
    draw_text(d, x + r * 2 + 6, base, 'LLY', px, CREAM)
    bw = text_w('BALDY', 96, 28)
    draw_text(d, 640 - bw / 2, base + 130, 'BALDY', 96, YELLOW, 28)
    return img.resize((W, H), Image.LANCZOS)


def arc_text(img, s, cx, cy, R, px, col, a_mid=-90, spread=None):
    """글자를 원 위쪽 호를 따라 (글자 밑이 원 쪽)"""
    f = F(px)
    widths = [f.getlength(c) / S for c in s]
    gap = px * 0.06
    tot = sum(widths) + gap * (len(s) - 1)
    ang = math.degrees(tot / R)
    a = a_mid - ang / 2
    for c, w in zip(s, widths):
        am = a + math.degrees((w / 2) / R)
        tile = Image.new('L', (int(px * 2 * S), int(px * 2 * S)), 0)
        ImageDraw.Draw(tile).text((tile.width / 2, tile.height / 2), c, font=f, fill=255, anchor='ms')
        tile = tile.rotate(-(am + 90), resample=Image.BICUBIC)
        x, y = cx + math.cos(math.radians(am)) * R, cy + math.sin(math.radians(am)) * R
        img.paste(Image.new('RGB', tile.size, col), (int(x * S - tile.width / 2), int(y * S - tile.height / 2)), tile)
        a += math.degrees((w + gap) / R)


def logo_B():
    img = Image.new('RGB', (W * S, H * S), BG)
    d = ImageDraw.Draw(img)
    cx, cy = 640, 330
    arc_text(img, 'JOLLY', cx, cy + 40, 190, 118, YELLOW)
    # 눈: ^ ^ , 입: 누운 D (평평한 쪽이 위)
    f = F(150)
    for sg in (-1, 1):
        d.text(((cx + sg * 78) * S, (cy + 120) * S), '^', font=f, fill=CREAM, anchor='ms')
    tile = Image.new('L', (int(240 * S), int(240 * S)), 0)
    ImageDraw.Draw(tile).text((tile.width / 2, tile.height / 2), 'D', font=F(170), fill=255, anchor='mm')
    tile = tile.rotate(-90, resample=Image.BICUBIC)
    img.paste(Image.new('RGB', tile.size, CREAM), (int(cx * S - tile.width / 2), int((cy + 150) * S - tile.height / 2)), tile)
    d.polygon(sc(sparkle(cx - 150, cy - 175, 26)), fill=WHITE)
    bw = text_w('BALDY', 84, 26)
    draw_text(d, 640 - bw / 2, 680, 'BALDY', 84, YELLOW, 26)
    return img.resize((W, H), Image.LANCZOS)


def head_mask(size, cx, cy, R):
    m = Image.new('L', size, 0)
    md = ImageDraw.Draw(m)
    pts = []
    for k in range(121):
        t = k / 120 * 2 * math.pi
        s = math.sin(t)
        pts.append((cx + math.cos(t) * R * (0.84 if s < 0 else 0.84 - 0.08 * s), cy + s * R * (1.0 if s < 0 else 0.9)))
    md.polygon(sc(pts), fill=255)
    for sg in (-1, 1):   # 귀
        md.ellipse(sc([(cx + sg * R * 0.86 - R * 0.15, cy + R * 0.12 - R * 0.19), (cx + sg * R * 0.86 + R * 0.15, cy + R * 0.12 + R * 0.19)]), fill=255)
    return m


def logo_C():
    img = Image.new('RGB', (W * S, H * S), BG)
    cx, cy, R = 640, 300, 230
    # 글자 판: 줄마다 JOLLYBALDY 를 엇갈려, 두 색 번갈아
    tex = Image.new('RGB', img.size, BG)
    td = ImageDraw.Draw(tex)
    px, lh = 14, 15
    f = F(px)
    for i, y in enumerate(range(cy - R - 40, cy + R + 40, lh)):
        x = cx - R - 60 - (i * 37) % 120
        s = 'JOLLY BALDY ' * 6
        td.text((x * S, (y + px) * S), s, font=f, fill=YELLOW if i % 2 == 0 else CREAM, anchor='ls')
    m = head_mask(img.size, cx, cy, R)
    # 눈 · 입 · 광은 빈칸 (마스크에서 지움)
    md = ImageDraw.Draw(m)
    w = int(R * 0.1 * S)
    for sg in (-1, 1):
        ex, ey = cx + sg * R * 0.33, cy - R * 0.1
        md.arc(sc([(ex - R * 0.17, ey - R * 0.14), (ex + R * 0.17, ey + R * 0.2)]), 200, 340, fill=0, width=w)
    md.chord(sc([(cx - R * 0.5, cy + R * 0.06), (cx + R * 0.5, cy + R * 0.8)]), 0, 180, fill=0)
    md.ellipse(sc([(cx - R * 0.55, cy - R * 0.84), (cx - R * 0.22, cy - R * 0.62)]), fill=0)
    md.polygon(sc(sparkle(cx - R * 0.06, cy - R * 0.8, R * 0.12)), fill=0)
    img.paste(tex, mask=m)
    d = ImageDraw.Draw(img)
    # 광은 흰색으로 다시
    d.ellipse(sc([(cx - R * 0.55, cy - R * 0.84), (cx - R * 0.22, cy - R * 0.62)]), fill=WHITE)
    d.polygon(sc(sparkle(cx - R * 0.06, cy - R * 0.8, R * 0.12)), fill=WHITE)
    bw = text_w('JOLLY BALDY', 64, 10)
    draw_text(d, 640 - bw / 2, 670, 'JOLLY BALDY', 64, WHITE, 10)
    return img.resize((W, H), Image.LANCZOS)


def logo_D():
    """엄지 척 이모티콘 b(^▽^)d: b 와 d 가 엄지, 가운데 대머리"""
    img = Image.new('RGB', (W * S, H * S), BG)
    d = ImageDraw.Draw(img)
    px, base, r = 230, 400, 92
    f = F(px)
    wb, wd = f.getlength('b') / S, f.getlength('d') / S
    gap = 14
    x0 = 640 - (wb + gap * 2 + r * 2 + wd) / 2
    d.text((x0 * S, base * S), 'b', font=f, fill=CREAM, anchor='ls')
    cx, cy = x0 + wb + gap + r, base - r * 0.95
    d.ellipse(sc([(cx - r, cy - r), (cx + r, cy + r)]), fill=YELLOW)
    face(d, cx, cy, r, INK)
    d.text(((cx + r + gap) * S, base * S), 'd', font=f, fill=CREAM, anchor='ls')
    bw = text_w('JOLLY BALDY', 64, 12)
    draw_text(d, 640 - bw / 2, 560, 'JOLLY BALDY', 64, YELLOW, 12)
    return img.resize((W, H), Image.LANCZOS)


def logo_E():
    """j 의 점이 반짝이는 대머리"""
    img = Image.new('RGB', (W * S, H * S), BG)
    d = ImageDraw.Draw(img)
    s, px, base = 'jolly baldy', 150, 440
    f = F(px)
    tw = f.getlength(s) / S
    x0 = 640 - tw / 2
    d.text((x0 * S, base * S), s, font=f, fill=CREAM, anchor='ls')
    xh = -f.getbbox('x', anchor='ls')[1] / S          # x 높이
    jb = f.getbbox('j', anchor='ls')
    jx0, jx1 = x0 + jb[0] / S, x0 + jb[2] / S
    d.rectangle(sc([(jx0 - 4, base - px * 1.2), (jx1 + 4, base - xh - 6)]), fill=BG)   # j 의 점 지우기
    r = px * 0.2
    cx, cy = (jx0 + jx1) / 2 + 6, base - xh - r - 16
    d.ellipse(sc([(cx - r, cy - r), (cx + r, cy + r)]), fill=YELLOW)
    face(d, cx, cy, r, INK)
    return img.resize((W, H), Image.LANCZOS)


def arc_text_bottom(img, s, cx, cy, R, px, col, gap_k=0.06):
    """원 아래 호를 따라 왼쪽→오른쪽 (글자 위가 원 가운데 쪽): 웃는 입처럼"""
    f = F(px)
    widths = [f.getlength(c) / S for c in s]
    gap = px * gap_k
    tot = sum(widths) + gap * (len(s) - 1)
    a = 90 + math.degrees(tot / R) / 2
    for c, w in zip(s, widths):
        am = a - math.degrees((w / 2) / R)
        tile = Image.new('L', (int(px * 2 * S), int(px * 2 * S)), 0)
        ImageDraw.Draw(tile).text((tile.width / 2, tile.height / 2), c, font=f, fill=255, anchor='mt')
        tile = tile.rotate(-(am - 90), resample=Image.BICUBIC)
        x, y = cx + math.cos(math.radians(am)) * R, cy + math.sin(math.radians(am)) * R
        img.paste(Image.new('RGB', tile.size, col), (int(x * S - tile.width / 2), int(y * S - tile.height / 2)), tile)
        a -= math.degrees((w + gap) / R)


def logo_F():
    """글자 고리 얼굴: 위 호 BALDY = 정수리, 아래 호 JOLLY = 웃는 입, 가운데 ^ ^"""
    img = Image.new('RGB', (W * S, H * S), BG)
    d = ImageDraw.Draw(img)
    cx, cy, R = 640, 360, 205
    arc_text(img, 'BALDY', cx, cy, R, 104, YELLOW)
    arc_text_bottom(img, 'JOLLY', cx, cy, R - 10, 96, CREAM)
    for sg in (-1, 1):   # 귀 (양옆 점)
        d.ellipse(sc([(cx + sg * (R + 40) - 16, cy - 22), (cx + sg * (R + 40) + 16, cy + 22)]), fill=YELLOW)
    f = F(170)
    for sg in (-1, 1):
        d.text(((cx + sg * 70) * S, (cy + 92) * S), '^', font=f, fill=CREAM, anchor='ms')
    d.polygon(sc(sparkle(cx - 120, cy - 112, 22)), fill=WHITE)
    return img.resize((W, H), Image.LANCZOS)


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    for group, names, fns in (('', 'ABC', (logo_A, logo_B, logo_C)), ('2', 'DEF', (logo_D, logo_E, logo_F))):
        sheet = Image.new('RGB', (1920, 360))
        for i, (n, fn) in enumerate(zip(names, fns)):
            im = fn()
            im.save(os.path.join(outdir, 'type_%s.png' % n))
            sheet.paste(im.resize((640, 360), Image.LANCZOS), (i * 640, 0))
        sheet.save(os.path.join(outdir, 'type_sheet%s.png' % group))
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
