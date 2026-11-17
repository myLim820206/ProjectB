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


MONO = next((p for p in ['C:/Windows/Fonts/consolab.ttf', 'C:/Windows/Fonts/consola.ttf', 'C:/Windows/Fonts/courbd.ttf'] if os.path.exists(p)), FONT)


def logo_G():
    """아스키 아트: 머리 윤곽 · 눈 · 입 · 광을 JOLLYBALDY 글자를 차례로 늘어놓아 그림"""
    img = Image.new('RGB', (W * S, H * S), BG)
    d = ImageDraw.Draw(img)
    cx, cy, R = 640, 300, 235
    cw, chh = 11, 14                                # 글자 칸 (설계 px)
    # 선 그림 (칸 크기 굵기): 여기에 걸리는 칸에 글자를 놓는다
    line = Image.new('L', (W, H), 0)
    ld = ImageDraw.Draw(line)
    pts = []
    for k in range(121):
        t = k / 120 * 2 * math.pi
        s = math.sin(t)
        pts.append((cx + math.cos(t) * R * (0.84 if s < 0 else 0.84 - 0.08 * s), cy + s * R * (1.0 if s < 0 else 0.9)))
    ld.line(pts, fill=255, width=18, joint='curve')
    for sg in (-1, 1):
        ld.arc([cx + sg * R * 0.86 - R * 0.13, cy + R * 0.12 - R * 0.17, cx + sg * R * 0.86 + R * 0.13, cy + R * 0.12 + R * 0.17],
               90 if sg < 0 else -90, 270 if sg < 0 else 90, fill=255, width=16)
        ex, ey = cx + sg * R * 0.33, cy - R * 0.08
        ld.arc([ex - R * 0.2, ey - R * 0.16, ex + R * 0.2, ey + R * 0.24], 195, 345, fill=255, width=20)
    ld.chord([cx - R * 0.48, cy + R * 0.1, cx + R * 0.48, cy + R * 0.8], 0, 180, fill=255)
    hl = Image.new('L', (W, H), 0)                    # 광 (흰 글자)
    hd = ImageDraw.Draw(hl)
    hd.ellipse([cx - R * 0.56, cy - R * 0.86, cx - R * 0.24, cy - R * 0.62], fill=255)
    hd.polygon(sparkle(cx - R * 0.08, cy - R * 0.8, R * 0.14, 0.3), fill=255)
    f = ImageFont.truetype(MONO, int(14 * S))
    word, i = 'JOLLYBALDY', 0
    for y in range(cy - R - 30, cy + R + 10, chh):
        for x in range(cx - R - 50, cx + R + 50, cw):
            box = (x + 2, y + 2, x + cw - 2, y + chh - 2)   # 칸 안쪽 대부분이 선에 걸리면
            on = sum(1 for v in line.crop(box).getdata() if v) > (cw - 4) * (chh - 4) * 0.35
            white = sum(1 for v in hl.crop(box).getdata() if v) > (cw - 4) * (chh - 4) * 0.35
            if on or white:
                col = WHITE if white else (YELLOW if (i // 10) % 2 == 0 else CREAM)
                d.text((x * S, (y + chh - 3) * S), word[i % 10], font=f, fill=col, anchor='ls')
                i += 1
    bw = text_w('JOLLY BALDY', 58, 12)
    draw_text(d, 640 - bw / 2, 670, 'JOLLY BALDY', 58, YELLOW, 12)
    return img.resize((W, H), Image.LANCZOS)


def logo_H():
    """반짝이는 대머리 글자: 글자마다 정수리에 광 (하얀 타원 + 작은 별)"""
    img = Image.new('RGB', (W * S, H * S), BG)
    d = ImageDraw.Draw(img)
    for row, (s, px, y, col) in enumerate((('JOLLY', 170, 360, YELLOW), ('BALDY', 170, 560, CREAM))):
        f = F(px)
        track = 12
        x = 640 - text_w(s, px, track) / 2
        for c in s:
            d.text((x * S, y * S), c, font=f, fill=col, anchor='ls')
            bb = f.getbbox(c, anchor='ls')
            l, t, r_ = x + bb[0] / S, y + bb[1] / S, x + bb[2] / S
            gw = (r_ - l)
            # 광은 글자 획 안에만 (글자 모양으로 잘라 냄): 왼쪽 위 어깨에 비스듬한 하얀 줄
            glyph = Image.new('L', img.size, 0)
            ImageDraw.Draw(glyph).text((x * S, y * S), c, font=f, fill=255, anchor='ls')
            gl = Image.new('L', img.size, 0)
            gd = ImageDraw.Draw(gl)
            gx, gy = l + px * 0.05, t + px * 0.05
            gd.ellipse(sc([(gx, gy), (gx + px * 0.17, gy + px * 0.1)]), fill=255)
            gd.ellipse(sc([(gx + px * 0.21, gy + px * 0.005), (gx + px * 0.27, gy + px * 0.045)]), fill=255)
            img.paste(WHITE, mask=ImageChops.multiply(glyph, gl))
            x += f.getlength(c) / S + track
    return img.resize((W, H), Image.LANCZOS)


def put_char(img, c, cx, cy, px, col, rot=0.0, sx=1.0, sy=1.0, font=None):
    """글자 하나를 가운데 (cx, cy) 에, 회전 · 늘이기"""
    f = ImageFont.truetype(font or FONT, int(px * S))
    tile = Image.new('L', (int(px * 2 * S), int(px * 2 * S)), 0)
    ImageDraw.Draw(tile).text((tile.width / 2, tile.height / 2), c, font=f, fill=255, anchor='mm')
    if sx != 1.0 or sy != 1.0:
        tile = tile.resize((max(1, int(tile.width * sx)), max(1, int(tile.height * sy))), Image.LANCZOS)
    if rot:
        tile = tile.rotate(rot, resample=Image.BICUBIC, expand=True)
    img.paste(Image.new('RGB', tile.size, col), (int(cx * S - tile.width / 2), int(cy * S - tile.height / 2)), tile)


def logo_I():
    """글자로 만든 얼굴: 누운 D = 대머리 정수리, ( ) = 귀, ^ ^ = 눈, J = 코, U = 입"""
    img = Image.new('RGB', (W * S, H * S), BG)
    d = ImageDraw.Draw(img)
    """누운 D 가 머리 전체: 둥근 쪽이 정수리, 평평한 쪽이 턱, 속 빈칸이 얼굴"""
    cx, cy = 640, 290
    put_char(img, 'D', cx, cy, 520, YELLOW, rot=90, sx=1.0, sy=1.0)
    row = int((cy + 140) * S)                                            # 머리 가운데 x 재기 (회전하면 조금 밀린다)
    xs = [x for x in range(W * S) if img.getpixel((x, row))[0] > 200]
    hx0, hx1 = xs[0] / S, xs[-1] / S
    cx = (hx0 + hx1) / 2
    put_char(img, '(', hx0 - 55, cy + 40, 150, YELLOW)
    put_char(img, ')', hx1 + 55, cy + 40, 150, YELLOW)
    # D 속 빈칸 자리 재기: 가운데 세로줄에서 노랑 → 바탕 → 노랑
    col = [img.getpixel((cx * S, y)) for y in range(0, H * S)]
    yel = [p[0] > 200 and p[1] > 150 for p in col]
    top = yel.index(True)
    h0 = next(y for y in range(top, len(yel)) if not yel[y])
    h1 = next(y for y in range(h0, len(yel)) if yel[y])
    bot = max(y for y in range(len(yel)) if yel[y])
    hy0, hy1, by = h0 / S, h1 / S, bot / S
    ty = top / S                                                         # 정수리 광: 돔 왼쪽 위
    d.ellipse(sc([(cx - 115, ty + 34), (cx - 55, ty + 62)]), fill=WHITE)
    d.polygon(sc(sparkle(cx - 28, ty + 34, 16)), fill=WHITE)
    for sg in (-1, 1):   # 눈은 빈칸 안에
        put_char(img, '^', cx + sg * 50, (hy0 + hy1) / 2 + 22, 100, CREAM)
    put_char(img, 'U', cx, (hy1 + by) / 2 + 4, 96, INK, sx=1.6, sy=0.8)   # 입은 노란 턱 위에
    bw = text_w('JOLLY BALDY', 58, 12)
    draw_text(d, 640 - bw / 2, 660, 'JOLLY BALDY', 58, YELLOW, 12)
    return img.resize((W, H), Image.LANCZOS)


def logo_J():
    """이름 글자로 얼굴: 큰 O = 머리, 누운 B = 웃는 눈 둘(∩∩), 누운 D = 입, L 둘 = 귀, Y = 정수리 광"""
    img = Image.new('RGB', (W * S, H * S), BG)
    d = ImageDraw.Draw(img)
    cx, cy = 640, 300
    thin = next((p for p in ['C:/Windows/Fonts/segoeuisl.ttf', 'C:/Windows/Fonts/segoeui.ttf'] if os.path.exists(p)), FONT)
    put_char(img, 'O', cx, cy, 620, YELLOW, sx=1.05, font=thin)         # 가는 O = 머리 윤곽
    put_char(img, 'B', cx, cy - 25, 120, CREAM, rot=90)                 # 둥근 쪽이 위: ∩∩ 웃는 눈
    put_char(img, 'D', cx, cy + 75, 120, CREAM, rot=-90, sx=1.3)        # 둥근 쪽이 아래: 웃는 입
    put_char(img, 'L', cx - 232, cy + 20, 110, YELLOW, sx=0.8)          # 귀
    put_char(img, 'L', cx + 232, cy + 20, 110, YELLOW, sx=0.8)
    put_char(img, 'Y', cx - 95, cy - 140, 60, WHITE, rot=25)            # 광 (Y 가 반짝)
    bw = text_w('JOLLY BALDY', 58, 12)
    draw_text(d, 640 - bw / 2, 660, 'JOLLY BALDY', 58, YELLOW, 12)
    return img.resize((W, H), Image.LANCZOS)


def logo_K():
    """이름이 곧 웃는 입: JOLLY BALDY 를 활짝 웃는 곡선 위에, 위에 웃는 눈 둘 + 정수리 광"""
    img = Image.new('RGB', (W * S, H * S), BG)
    d = ImageDraw.Draw(img)
    cx, cy, R = 640, 40, 520
    arc_text_bottom(img, 'JOLLY BALDY', cx, cy, R, 92, YELLOW, gap_k=0.04)
    w = int(16 * S)
    for sg in (-1, 1):
        ex, ey = cx + sg * 150, 300
        d.arc(sc([(ex - 50, ey - 40), (ex + 50, ey + 50)]), 200, 340, fill=CREAM, width=w)
    d.ellipse(sc([(cx - 210, 120), (cx - 140, 152)]), fill=WHITE)
    d.polygon(sc(sparkle(cx - 108, 118, 18)), fill=WHITE)
    return img.resize((W, H), Image.LANCZOS)


def logo_L():
    """빈칸 대머리: 글자로 꽉 찬 판에서 대머리 모양만 비우고, 얼굴은 노랑으로"""
    img = Image.new('RGB', (W * S, H * S), BG)
    d = ImageDraw.Draw(img)
    x0, y0, x1, y1 = 300, 70, 980, 560
    blk = Image.new('RGB', img.size, BG)
    bd = ImageDraw.Draw(blk)
    px = 62
    f = F(px)
    for i, y in enumerate(range(y0, y1, int(px * 0.92))):
        word = ('JOLLY BALDY ' if i % 2 == 0 else 'BALDY JOLLY ') * 4
        bd.text(((x0 - (i * 53) % 200) * S, (y + px * 0.85) * S), word, font=f, fill=CREAM if i % 2 else (90, 90, 110), anchor='ls')
    frame = Image.new('L', img.size, 0)
    ImageDraw.Draw(frame).rounded_rectangle(sc([(x0, y0), (x1, y1)]), 30 * S, fill=255)
    cx, cy, R = 640, 330, 200
    hm = head_mask(img.size, cx, cy, R)
    frame = ImageChops.subtract(frame, hm)
    img.paste(blk, mask=frame)
    # 얼굴 (빈칸 안에 노랑)
    w = int(R * 0.1 * S)
    for sg in (-1, 1):
        ex, ey = cx + sg * R * 0.33, cy - R * 0.08
        d.arc(sc([(ex - R * 0.17, ey - R * 0.14), (ex + R * 0.17, ey + R * 0.2)]), 200, 340, fill=YELLOW, width=w)
    d.chord(sc([(cx - R * 0.48, cy + R * 0.1), (cx + R * 0.48, cy + R * 0.8)]), 0, 180, fill=YELLOW)
    d.ellipse(sc([(cx - R * 0.56, cy - R * 0.86), (cx - R * 0.24, cy - R * 0.62)]), fill=WHITE)
    d.polygon(sc(sparkle(cx - R * 0.08, cy - R * 0.8, R * 0.14)), fill=WHITE)
    bw = text_w('JOLLY BALDY', 58, 12)
    draw_text(d, 640 - bw / 2, 660, 'JOLLY BALDY', 58, YELLOW, 12)
    return img.resize((W, H), Image.LANCZOS)


# ---------------------------------------------------------------- K 애니메이션
def _clamp01(v):
    return 0.0 if v < 0 else 1.0 if v > 1 else v


def _ease_out_back(v, k=1.9):
    v -= 1
    return 1 + (k + 1) * v ** 3 + k * v ** 2


def _ease_out(v):
    return 1 - (1 - v) ** 3


def _dim(c, a):
    return tuple(int(BG[i] + (c[i] - BG[i]) * a) for i in range(3))


K_LEN = 3.6   # 초


def k_frame(t, s=2):
    """K 시안의 t 초 장면. 0.3 눈 뜸 → 0.7 글자가 하나씩 튀어나와 웃는 입 → 1.75 깜빡 → 2.1 정수리 광 '띵' → 유지"""
    global S
    S0, S = S, s
    img = Image.new('RGB', (W * S, H * S), BG)
    d = ImageDraw.Draw(img)
    cx, cy, R = 640, 40, 520
    # 1) 눈: 납작한 선에서 둥근 웃는 눈으로 (0.3~0.65), 1.75 에 한 번 깜빡
    eo = _ease_out_back(_clamp01((t - 0.3) / 0.35), 1.4)
    bl = _clamp01(1 - abs(t - 1.85) / 0.1)            # 깜빡 (1.75~1.95)
    eh = max(0.0, eo) * (1 - bl * 0.85)
    if t > 0.3:
        w = max(1, int(16 * S))
        for sg in (-1, 1):
            ex, ey = cx + sg * 150, 300
            hh = 45 * eh
            if hh < 4:
                d.line(sc([(ex - 46, ey + 3), (ex + 46, ey + 3)]), fill=_dim(CREAM, _clamp01(eo)), width=w)
            else:
                d.arc(sc([(ex - 50, ey + 5 - hh), (ex + 50, ey + 5 + hh)]), 200, 340, fill=_dim(CREAM, _clamp01(eo)), width=w)
    # 2) 글자: 왼쪽부터 0.065초 간격으로 튀어나옴 (커졌다 제자리, 위에서 살짝 떨어짐)
    s_txt, px, gap_k = 'JOLLY BALDY', 92, 0.04
    f = F(px)
    widths = [f.getlength(c) / S for c in s_txt]
    gap = px * gap_k
    tot = sum(widths) + gap * (len(s_txt) - 1)
    Rt = R
    a = 90 + math.degrees(tot / Rt) / 2
    for i, (c, wd) in enumerate(zip(s_txt, widths)):
        am = a - math.degrees((wd / 2) / Rt)
        a -= math.degrees((wd + gap) / Rt)
        lt = _clamp01((t - 0.7 - i * 0.065) / 0.32)
        if lt <= 0 or c == ' ':
            continue
        sc_ = max(0.05, _ease_out_back(lt))
        drop = (1 - _ease_out(lt)) * 40                   # 원 가운데 쪽(위)에서 떨어져 내려옴
        tile = Image.new('L', (int(px * 2 * S), int(px * 2 * S)), 0)
        ImageDraw.Draw(tile).text((tile.width / 2, tile.height / 2), c, font=f, fill=255, anchor='mt')
        n = max(1, int(tile.width * sc_))
        tile = tile.resize((n, n), Image.LANCZOS).rotate(-(am - 90), resample=Image.BICUBIC)
        rr = Rt - drop
        x, y = cx + math.cos(math.radians(am)) * rr, cy + math.sin(math.radians(am)) * rr
        col = _dim(YELLOW, _clamp01(lt * 2.5))
        img.paste(Image.new('RGB', tile.size, col), (int(x * S - tile.width / 2), int(y * S - tile.height / 2)), tile)
    # 3) 정수리 광: 2.1 에 타원이 스르륵, 별이 크게 번쩍했다 제자리 (빛줄 넷)
    gt = _clamp01((t - 2.1) / 0.25)
    if gt > 0:
        ga = _ease_out(gt)
        ex0, ey0 = cx - 175, 136
        rw, rh = 35 * ga, 16 * ga
        d.ellipse(sc([(ex0 - rw, ey0 - rh), (ex0 + rw, ey0 + rh)]), fill=WHITE)
        st = _clamp01((t - 2.2) / 0.45)
        if st > 0:
            big = 1 + 2.2 * math.sin(math.pi * min(1.0, st * 1.3)) if st < 0.77 else 1.0
            sx_, sy_ = cx - 108, 118
            rot = (1 - _ease_out(st)) * 90
            pts = [(sx_ + math.cos(math.radians(-90 + k * 45 + rot)) * (18 * big if k % 2 == 0 else 18 * big * 0.25),
                    sy_ + math.sin(math.radians(-90 + k * 45 + rot)) * (18 * big if k % 2 == 0 else 18 * big * 0.25)) for k in range(8)]
            d.polygon(sc(pts), fill=WHITE)
            if st < 0.8:   # 번쩍 빛줄
                ra = (1 - st / 0.8)
                for k in range(4):
                    ang = math.radians(45 + k * 90)
                    r0, r1 = 26, 26 + 60 * _ease_out(st / 0.8)
                    d.line(sc([(sx_ + math.cos(ang) * r0, sy_ + math.sin(ang) * r0), (sx_ + math.cos(ang) * r1, sy_ + math.sin(ang) * r1)]),
                           fill=_dim(WHITE, ra), width=max(1, int(4 * S)))
    out = img.resize((W, H), Image.LANCZOS)
    S = S0
    return out


def anim_K(outdir, fps=25):
    os.makedirs(outdir, exist_ok=True)
    n = int(K_LEN * fps)
    frames = [k_frame(i / fps) for i in range(n)]
    for tt in (0.5, 1.0, 1.85, 2.3, 3.5):   # 확인용 낱장
        k_frame(tt).save(os.path.join(outdir, 'K_t%.2f.png' % tt))
    small = [fr.resize((800, 450), Image.LANCZOS).convert('P', palette=Image.ADAPTIVE, colors=64) for fr in frames]
    durs = [int(1000 / fps)] * n
    durs[-1] = 1500   # 끝에서 잠깐 머묾
    small[0].save(os.path.join(outdir, 'K_anim.gif'), save_all=True, append_images=small[1:], duration=durs, loop=0)
    print('anim ok', n, 'frames')


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    for group, names, fns in (('', 'ABC', (logo_A, logo_B, logo_C)), ('2', 'DEF', (logo_D, logo_E, logo_F)), ('3', 'GHI', (logo_G, logo_H, logo_I)), ('4', 'JKL', (logo_J, logo_K, logo_L))):
        sheet = Image.new('RGB', (1920, 360))
        for i, (n, fn) in enumerate(zip(names, fns)):
            im = fn()
            im.save(os.path.join(outdir, 'type_%s.png' % n))
            sheet.paste(im.resize((640, 360), Image.LANCZOS), (i * 640, 0))
        sheet.save(os.path.join(outdir, 'type_sheet%s.png' % group))
    print('ok')


if __name__ == '__main__':
    if len(sys.argv) > 2 and sys.argv[2] == 'animK':   # python logo_jollybaldy_type.py 출력폴더 animK
        anim_K(sys.argv[1])
    else:
        main(sys.argv[1] if len(sys.argv) > 1 else '.')
