"""인게임 윗줄 정리 시안: 주크박스를 네모로 + 윗줄 간판들(주크박스 · WAVE · 목숨 · 레벨 · 버튼)을 한 규칙으로

    python tools/neon/topbar_mock.py 출력폴더   → topbar_mock.png

지금: 아치 주크박스 · 두꺼운 WAVE 간판 · 하트 상자 · 레벨 알약이 높이 · 모양 · 굵기가 다 달라 난잡하다.
A 같은 카드 줄: 모두 같은 높이(64) · 같은 둥근 사각 두 겹 테 · 위 작은 머리글 + 큰 값. 색은 값 · 아이콘에만
B 한 장 계기판: 270 ~ 1130 을 긴 판 하나로, 칸 사이 세로 구분선. 주크박스는 왼쪽 칸
C 가운데 WAVE 강조: WAVE 만 크게(높이 76), 양옆은 같은 크기 작은 카드(주크박스 | 목숨 · 레벨) 좌우 대칭
화면 좌표 그대로 (윗줄 0 ~ 98, 판은 98 부터). 세 시안을 위아래로.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, PINK, CYAN, GOLD, WHITE, FONT_KO, FONT_EN, hx  # noqa: E402
from neon import suit  # noqa: E402

FG, DIM, GREY, LAV, RED = hx('dce4f0'), hx('5f6878'), hx('8a94a6'), hx('a896e1'), hx('ff6a6a')
ORANGE = hx('ffa046')
FRAME = hx('7fd6ff')


def fill(M, x, y, w, h, c, b=0.16):
    M.ln([(x, y + h / 2), (x + w, y + h / 2)], c, h, b)


def card(M, x, y, w, h, c=FRAME, b=0.55, r=10):
    """공통 틀: 둥근 사각 두 겹 (바깥 1.4, 안 0.7)"""
    M.rr(x, y, w, h, r, c, 1.4, b)
    M.rr(x + 3, y + 3, w - 6, h - 6, r - 3, c, 0.7, b * 0.45)


def label(M, x, y, s, c=GREY):
    M.word(x, y, s, c, 9, font=FONT_EN, anchor_center=False, track=1.2)


def logo(M, oy):
    M.word(132, oy + 40, 'Alice', PINK, 30, font=FONT_EN)
    M.word(78, oy + 18, 'BAR', CYAN, 11, font=FONT_EN, track=2)


def board_edge(M, oy):
    M.ln([(262, oy + 98), (1030, oy + 98)], DIM, 1.0, 0.5)
    for i, (s, c) in enumerate((('3', CYAN), ('8', PINK), ('A', WHITE), ('K', PINK), ('5', PINK), ('J', PINK))):
        x = 270 + i * 48
        M.rr(x + 8, oy + 104, 32, 32, 6, c, 1.2, 0.7)
        M.word(x + 24, oy + 125, s, c, 12, font=FONT_EN)


def juke_square(M, x, y, w, h, small=False):
    """네모 주크박스: 둥근 사각 몸통 + 위 무지개 띠(직선) + 유리창(레코드 · 곡) + 아래 ◀ 램프 ▶ + 스피커 줄"""
    M.rr(x, y, w, h, 8, LAV, 1.6, 0.9)
    for k, c in enumerate((PINK, GOLD, CYAN)):  # 위 무지개 띠: 곧은 세 줄
        M.ln([(x + 10, y + 4 + k * 2.5), (x + w - 10, y + 4 + k * 2.5)], c, 0.9, 0.85)
    wy, wh = y + 12, h - 30
    M.rr(x + 7, wy, w - 14, wh, 5, FG, 1.0, 0.6)
    rc = (x + 22, wy + wh / 2)
    from neon import circle
    M.ln(circle(rc[0], rc[1], 9, 20), FG, 1.0, 0.8)
    M.ln(circle(rc[0], rc[1], 3, 10), PINK, 1.0, 0.9)
    label(M, x + 37, wy + wh / 2 - 3, 'NOW PLAYING', GOLD)
    M.word(x + 37, wy + wh / 2 + 10, 'Glass Ballad', FG, 10, font=FONT_EN, anchor_center=False)
    by = y + h - 13
    M.ln([(x + 14, by), (x + 20, by - 4), (x + 20, by + 4), (x + 14, by)], CYAN, 1.0, 0.9)
    M.ln([(x + w - 14, by), (x + w - 20, by - 4), (x + w - 20, by + 4), (x + w - 14, by)], CYAN, 1.0, 0.9)
    for k in range(5):
        cx = x + w / 2 - 20 + k * 10
        M.rr(cx - 3, by - 3, 6, 6, 1.5, GOLD if k == 2 else DIM, 1.0, 0.9 if k == 2 else 0.5)


TX = []  # 아치 주크박스 글자 (jukebox_mock 이 모은다)


def juke_arch(M, cx, oy):
    """아치 주크박스 2c (사용자 고름): 폭 150 · 높이 0.9배(85) · 위 끝 y 2 · 바닥 y 87"""
    import jukebox_mock as J
    J.KO = FONT_KO
    k, w = 0.9, 150.0
    J.JW = w / k
    J.jukebox(M, TX, cx - w / 2, oy + 87 - J.JH * k, k, 0.3, 3, -10, 1)
    J.JW = 128


def wave_val(M, cx, y, big=34):
    t = M.word(cx - 18, y, 'WAVE 19', CYAN, big, font=FONT_EN, track=2)
    M.word(cx - 18 + t / 2 + 26, y, '/30', CYAN, big * 0.45, font=FONT_EN)


def heart_icon(M, x, y, r, c):
    for p in suit('heart')(x, y, r): M.ln(p, c, 1.5, 1.0)


def diamond_icon(M, x, y, r, c):
    M.ln([(x, y - r), (x + r * 0.7, y), (x, y + r), (x - r * 0.7, y), (x, y - r)], c, 1.4, 1.0)


def hud_btns(M, x, y, s=30):
    import math
    from neon import circle
    for k, t in enumerate(('II', '×1', 'gear')):
        bx = x + k * (s + 8)
        M.rr(bx, y, s, s, s / 2, FRAME, 1.2, 0.5)
        if t == 'gear':  # 톱니바퀴: 원 + 바퀴살 8
            cx, cy = bx + s / 2, y + s / 2
            M.ln(circle(cx, cy, s * 0.17, 14), FG, 1.1, 0.9)
            for j in range(8):
                a = j * math.pi / 4
                M.ln([(cx + math.cos(a) * s * 0.22, cy + math.sin(a) * s * 0.22), (cx + math.cos(a) * s * 0.32, cy + math.sin(a) * s * 0.32)], FG, 1.6, 0.9)
        else:
            M.word(bx + s / 2, y + s / 2 + 5, t, FG, 12, font=FONT_EN)


def level_bar(M, x, y, w, frac=0.62):
    M.ln([(x, y), (x + w, y)], GOLD, 3, 0.18)
    M.ln([(x, y), (x + w * frac, y)], GOLD, 3, 0.9)


def variant_A(M, oy):
    """같은 카드 줄: 높이 64, y 18"""
    y, h = oy + 18, 64
    logo(M, oy)
    juke_arch(M, 346, oy)
    card(M, 430, y, 380, h)
    label(M, 444, y + 16, 'WAVE')
    wave_val(M, 620, y + 44, 30)
    M.word(620, y + 58, '다음 보스 20웨이브 · 분열 군주 + 호위', ORANGE, 10, font=FONT_KO)
    card(M, 822, y, 130, h)
    label(M, 836, y + 16, 'LIFE')
    heart_icon(M, 850, y + 40, 10, PINK)
    M.word(900, y + 50, '999', PINK, 26, font=FONT_EN)
    card(M, 964, y, 170, h)
    label(M, 978, y + 16, 'LEVEL')
    diamond_icon(M, 984, y + 40, 8, GOLD)
    M.word(998, y + 47, '300', GOLD, 22, font=FONT_EN, anchor_center=False)
    M.word(1120, y + 47, '62%', FG, 10, font=FONT_EN)
    level_bar(M, 978, y + 56, 142)
    hud_btns(M, 1146, y + 17)
    board_edge(M, oy)


def variant_B(M, oy):
    """한 장 계기판"""
    y, h = oy + 16, 68
    logo(M, oy)
    juke_arch(M, 346, oy)
    card(M, 430, y, 706, h, FRAME, 0.6, 12)
    xs = [800, 930]
    for xx in xs: M.ln([(xx, y + 10), (xx, y + h - 10)], FRAME, 1.0, 0.35)
    label(M, 444, y + 16, 'WAVE')
    wave_val(M, 610, y + 46, 30)
    M.word(610, y + 60, '다음 보스 20웨이브 · 분열 군주 + 호위', ORANGE, 10, font=FONT_KO)
    label(M, 812, y + 16, 'LIFE')
    heart_icon(M, 826, y + 42, 10, PINK)
    M.word(880, y + 52, '999', PINK, 26, font=FONT_EN)
    label(M, 942, y + 16, 'LEVEL')
    diamond_icon(M, 948, y + 42, 8, GOLD)
    M.word(962, y + 49, '300', GOLD, 22, font=FONT_EN, anchor_center=False)
    M.word(1116, y + 49, '62%', FG, 10, font=FONT_EN)
    level_bar(M, 942, y + 58, 178)
    hud_btns(M, 1146, y + 19)
    board_edge(M, oy)


def variant_C(M, oy):
    """가운데 WAVE 강조, 양옆 같은 크기 카드"""
    logo(M, oy)
    sy, sh = oy + 24, 56
    juke_arch(M, 375, oy)
    card(M, 470, oy + 10, 340, 78, CYAN, 0.8, 14)
    wave_val(M, 640, oy + 52, 36)
    M.word(640, oy + 74, '다음 보스 20웨이브 · 분열 군주 + 호위', ORANGE, 10, font=FONT_KO)
    card(M, 830, sy, 150, sh)
    label(M, 842, sy + 15, 'LIFE')
    heart_icon(M, 858, sy + 36, 10, PINK)
    M.word(925, sy + 45, '999', PINK, 24, font=FONT_EN)
    card(M, 990, sy, 150, sh)
    label(M, 1002, sy + 15, 'LEVEL')
    diamond_icon(M, 1010, sy + 34, 7, GOLD)
    M.word(1024, sy + 41, '300', GOLD, 20, font=FONT_EN, anchor_center=False)
    M.word(1126, sy + 41, '62%', FG, 10, font=FONT_EN)
    level_bar(M, 1002, sy + 48, 126)
    hud_btns(M, 1150, oy + 104, 26)
    board_edge(M, oy)


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    M = Sign(0.8)
    M.small_glow = 0.0
    for k, (fn, cap) in enumerate(((variant_A, 'A 같은 카드 줄 — 높이 · 테 · 머리글 통일, 색은 값에만'),
                                   (variant_B, 'B 한 장 계기판 — 긴 판 하나 + 세로 구분선'),
                                   (variant_C, 'C 가운데 WAVE 강조 — 양옆 같은 크기 작은 카드, 버튼은 판 위 오른쪽'))):
        oy = 18 + k * 236
        M.word(1262, oy + 6, cap, WHITE, 12, font=FONT_KO, anchor_center=False) if False else M.word(640, oy + 4, cap, WHITE, 13, font=FONT_KO)
        fn(M, oy + 8)
    from PIL import Image, ImageChops, ImageDraw, ImageFont
    img = M.draw()
    d = ImageDraw.Draw(img)
    for t in TX:
        (x, y, s, c, px_, a), clip = t[:6], (t[6] if len(t) > 6 else None)
        f = ImageFont.truetype(FONT_KO, max(6, px_))
        if clip is None:
            d.text((x, y), s, fill=c, font=f, anchor=a)
            continue
        lay = Image.new('RGB', img.size, (0, 0, 0))
        ImageDraw.Draw(lay).text((x, y), s, fill=c, font=f, anchor=a)
        box = tuple(int(v) for v in clip)
        img.paste(ImageChops.lighter(img.crop(box), lay.crop(box)), box[:2])
    img.save(os.path.join(outdir, 'topbar_mock.png'))
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
