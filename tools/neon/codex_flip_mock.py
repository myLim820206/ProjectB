"""도감 책자 B안: 쪽 넘기는 움직임 (시안 GIF)

    python tools/neon/codex_flip_mock.py 출력폴더   → codex_flip.gif (960×540, 20fps)

› 를 누르면 오른쪽 쪽 한 장이 책등을 축으로 넘어간다:
  - 넘어가는 장의 바깥 끝 x = 책등 + 쪽폭 × cos(π u), u = 0 → 1 (0.45초, 시작 · 끝 부드럽게)
  - 바깥 끝이 눈 쪽으로 들리는 원근: 바깥으로 갈수록 위아래로 최대 10% 커짐 (sin(π u))
  - 앞면(목록) 은 u < 0.5 동안, 뒷면(왼쪽 상세) 은 u > 0.5 동안. 넘어가는 장 아래로 다음 쪽이 드러남
  - 장이 지나가는 쪽 위에 옅은 그림자 띠
탭을 바꾸면(조커) 여러 장이 빠르게 (장마다 0.3초, 0.12초 간격으로 겹쳐) 넘어간다
게임에선 장 · 글자 대신 선 · 상자를 그 원근 그대로 찍는다 (글자는 넘어가는 동안 숨김, 장이 다 넘어가면 나타남)
"""
import math
import os
import sys

from PIL import Image, ImageChops, ImageDraw

sys.path.insert(0, os.path.dirname(__file__))
import codex_book_mock as CB  # noqa: E402
from logo_mock import Sign, PINK, WHITE, FONT_KO, hx  # noqa: E402

LW = CB.BW / 2 - 28
LX, RX = CB.BX + 22, CB.SPX + 6
TOP, BOT = CB.BY + 22, CB.BY + CB.BH - 22
LAV, DIM, FG, JP = hx('a896e1'), hx('5f6878'), hx('dce4f0'), hx('ff9ad5')


def ease(u):
    return u * u * (3 - 2 * u)


def detail_cocktail(M, k):
    cx = LX + LW / 2
    CB.glass(M, k % 3, cx, CB.BY + 300, 2.2, CB.COLS[k % 5])
    M.word(cx, CB.BY + 350, CB.NAMES[k], WHITE, 30, font=FONT_KO)
    M.ln([(cx - 160, CB.BY + 396), (cx + 160, CB.BY + 396)], LAV, 1.0, 0.5)
    M.word(cx, CB.BY + 428, '설명 한 줄 · 효과', FG, 15, font=FONT_KO)


def detail_joker(M, k):
    cx = LX + LW / 2
    nm, ab, cap, tier = CB.JOKERS[k]
    CB.jester(M, cx, CB.BY + 170, 46, JP)
    M.word(cx, CB.BY + 268, nm, WHITE, 30, font=FONT_KO)
    M.word(cx, CB.BY + 292, f'{cap} 대장  ·  위력 {tier}', CB.GOLD, 13, font=FONT_KO)
    M.word(cx, CB.BY + 340, ab, FG, 15, font=FONT_KO)


def list_cocktail(M, page):
    CB.head(M, RX, LW, '칵테일', f'COCKTAILS  ·  {page + 1} / 3 쪽')
    CB.rows(M, RX + 36, CB.BY + 120, LW - 72, CB.NAMES[page * 12:page * 12 + 12], page * 12)


def list_joker(M):
    CB.head(M, RX, LW, '조커', 'JOKERS  ·  5 / 10 발견')
    for i, (nm, ab, cap, tier) in enumerate(CB.JOKERS):
        y = CB.BY + 128 + i * 42
        known = i in CB.KNOWN
        CB.jester(M, RX + 50, y - 4, 9, JP if known else DIM, 1.0 if known else 0.4)
        M.word(RX + 72, y + 2, nm if known else '???', CB.CYAN if known else DIM, 14, font=FONT_KO, anchor_center=False)


def sheet_lines(side):
    """넘어가는 장 위 그림 (책등에서 잰 거리 q, 높이 y): 앞면 = 목록 줄, 뒷면 = 상세 (큰 그림 + 줄 몇 개). 반환 [(점들, 색, 굵기, 밝기)]"""
    L = []
    if side == 'front':
        L.append(([(LW / 2 - 60, CB.BY + 66), (LW / 2 + 60, CB.BY + 66)], PINK, 2.0, 0.8))
        for i in range(12):
            y = CB.BY + 126 + i * 34
            L.append(([(36, y), (50, y), (50, y - 12), (36, y - 12), (36, y)], CB.CYAN, 1.0, 0.6))
            L.append(([(64, y - 5), (64 + 90 + (i * 37) % 70, y - 5)], CB.CYAN, 1.6, 0.6))
            L.append(([(LW - 76, y - 5), (LW - 40, y - 5)], CB.MINT, 1.6, 0.5))
    else:
        c = LW / 2
        L.append(([(c - 50, CB.BY + 170), (c + 50, CB.BY + 170), (c + 44, CB.BY + 300), (c - 44, CB.BY + 300), (c - 50, CB.BY + 170)], PINK, 1.6, 0.7))
        L.append(([(c - 90, CB.BY + 345), (c + 90, CB.BY + 345)], WHITE, 3.0, 0.6))
        L.append(([(c - 150, CB.BY + 425), (c + 150, CB.BY + 425)], FG, 1.6, 0.5))
    return L


def sheet(M, u):
    """넘어가는 장 하나: 테두리 + 그 면의 그림을 원근으로. 반환: 덮을 다각형(화면 좌표)"""
    a = math.pi * u
    ca, sa = math.cos(a), math.sin(a)
    cy = (TOP + BOT) / 2

    def P(q, y):  # q = 책등에서 잰 거리 (0 ~ LW)
        k = 1 + 0.10 * (q / LW) * sa
        return (CB.SPX + q * ca, cy + (y - cy) * k)

    poly = [P(0, TOP), P(LW, TOP), P(LW, BOT), P(0, BOT)]
    side = 'front' if u < 0.5 else 'back'
    for pts, c, wd, b in sheet_lines(side):
        if side == 'back':  # 뒷면은 왼쪽 쪽 그림: 책등에서 잰 거리로 바꿈
            pts = [(LW - x, y) for x, y in pts]
        M.ln([P(x, y) for x, y in pts], c, wd, b * (0.5 + 0.5 * abs(ca)))
    edge = [P(0, TOP), P(LW, TOP), P(LW, BOT), P(0, BOT), P(0, TOP)]
    M.ln(edge, LAV, 1.2, 0.7)
    return poly


def frame(state):
    """state: dict(tab, left(fn, arg), right(fn, arg), sheets=[u ...])"""
    base = Sign(0.8); base.small_glow = 0.0
    CB.book(base, active=state['tab'])
    state['left'][0](base, *state['left'][1:])
    state['right'][0](base, *state['right'][1:])
    CB.footer(base)
    img = base.draw()
    for u in state.get('sheets', []):
        top = Sign(0.8); top.small_glow = 0.0
        poly = sheet(top, u)
        d = ImageDraw.Draw(img)
        d.polygon(poly, fill=(9, 9, 15))
        # 그림자 띠: 장이 덮고 있는 쪽 위 (장 바깥 끝 근처)
        ex = poly[1][0]
        sh = Image.new('RGB', img.size, (0, 0, 0))
        sd = ImageDraw.Draw(sh)
        for k in range(18):
            x = ex + (k if u < 0.5 else -k) * 2
            sd.line([(x, TOP), (x, BOT)], fill=(int(30 * (1 - k / 18)),) * 3, width=2)
        img = ImageChops.subtract(img, sh)
        lay = top.draw()
        img = ImageChops.add(img, ImageChops.subtract(lay, Image.new('RGB', lay.size, (6, 6, 11))))
    return img.resize((960, 540), Image.LANCZOS)


def main(out):
    os.makedirs(out, exist_ok=True)
    F = []
    s1 = dict(tab=0, left=(detail_cocktail, 5), right=(list_cocktail, 0))
    F += [frame(s1)] * 8
    # › : 칵테일 1쪽 → 2쪽 (0.45초 = 9장)
    for f in range(9):
        u = ease((f + 1) / 9)
        F.append(frame(dict(tab=0, left=(detail_cocktail, 5), right=(list_cocktail, 1), sheets=[u])))
    s2 = dict(tab=0, left=(detail_cocktail, 12), right=(list_cocktail, 1))
    F += [frame(s2)] * 10
    # 조커 탭: 세 장이 빠르게 겹쳐 넘어감 (장마다 6장, 2장 간격)
    for f in range(12):
        us = [ease(min(1, max(0, (f - k * 2 + 1) / 6))) for k in range(3)]
        us = [u for u in us if 0 < u < 1]
        F.append(frame(dict(tab=1, left=(detail_cocktail, 12), right=(list_joker,), sheets=us)))
    s3 = dict(tab=1, left=(detail_joker, 1), right=(list_joker,))
    F += [frame(s3)] * 16
    pal = [f.convert('P', palette=Image.ADAPTIVE, colors=200) for f in F]
    pal[0].save(os.path.join(out, 'codex_flip.gif'), save_all=True, append_images=pal[1:], duration=50, loop=0)
    F[12].save(os.path.join(out, 'codex_flip_mid.png'))
    print('ok', len(F))


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
