"""도감 책자 B안: 쪽 넘기는 움직임 (시안 GIF, 다듬은 판)

    python tools/neon/codex_flip_mock.py 출력폴더   → codex_flip.gif (960×540, 25fps) + codex_flip_mid.png

움직임 (› 한 장, 0.5초):
  1 › 버튼이 눌려 살짝 내려가며 분홍 (0.08초)
  2 넘어가는 장이 휜다: 장의 각 줄은 책등에서 바깥으로 갈수록 각도가 앞서 (전반) / 뒤처져 (후반) 종이가 둥글게 말림.
    위 바깥 모서리가 아래보다 조금 먼저 들림 (대각선으로 말림)
  3 원근: 많이 들린 곳일수록 위아래로 커짐 (최대 10%)
  4 장 면 밝기는 눕힌 정도(|cos|)로: 똑바로 설 때 가장 어둡게, 접힌 곳(바깥 끝)에 흰 빛줄
  5 장 아래 드러나는 쪽엔 장 쪽으로 진해지는 그림자 (sin(π u))
  6 넘어가는 장 위엔 글자 대신 선 · 상자만. 아래로 드러나는 쪽 글자는 그대로 보이고, 장이 내려앉으면 왼쪽 상세가 0.16초 동안 바뀜(겹쳐 사라짐)
탭 바꾸기(조커): 고른 탭이 0.2초 동안 밀려 나오고(앞 탭은 들어감), 세 장이 0.1초 간격으로 겹쳐 넘어감
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
FPS = 25


def ease(u):
    u = min(1.0, max(0.0, u))
    return u * u * (3 - 2 * u)


# ---------------------------------------------------------------- 쪽 내용
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


# ---------------------------------------------------------------- 넘어가는 장 (휨 · 원근)
def sheet_map(u):
    """장 위 점 (q = 책등에서 잰 거리 0 ~ LW, y) → 화면. 줄마다 각도가 바깥으로 갈수록 달라져 종이가 말린다"""
    s = math.sin(math.pi * u)
    cy = (TOP + BOT) / 2
    lead = 0.55 * s * (1 if u < 0.5 else -1) * (1 - abs(2 * u - 1)) ** 0.3  # 전반: 바깥이 앞섬, 후반: 바깥이 뒤처짐
    N = 14

    def ang(t, y):  # t = q / LW
        tilt = 0.22 * s * ((BOT - y) / (BOT - TOP) - 0.5)  # 위 모서리가 먼저
        return math.pi * u + (lead + tilt) * t ** 1.6

    def P(q, y):
        t = q / LW
        x, depth = 0.0, 0.0
        for i in range(N):
            a = ang((i + 0.5) / N * t, y)
            x += math.cos(a) * q / N
            depth += math.sin(a) * q / N
        k = 1 + 0.10 * depth / LW
        return (CB.SPX + x, cy + (y - cy) * k), ang(t, y)
    return P


def sheet_lines(side):
    """장 위 그림 (q, y): 앞면 = 목록 줄, 뒷면 = 왼쪽 상세. 반환 [(점들, 색, 굵기, 밝기)]"""
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


def subdiv(pts, n=8):
    out = []
    for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
        for k in range(n): out.append((x0 + (x1 - x0) * k / n, y0 + (y1 - y0) * k / n))
    out.append(pts[-1])
    return out


def draw_sheet(img, u):
    P = sheet_map(u)
    M = Sign(0.8); M.small_glow = 0.0
    side = 'front' if u < 0.5 else 'back'
    ca = abs(math.cos(math.pi * u))
    face = 0.35 + 0.65 * ca  # 눕힌 정도로 면 밝기
    for pts, c, wd, b in sheet_lines(side):
        if side == 'back': pts = [(LW - x, y) for x, y in pts]
        M.ln([P(x, y)[0] for x, y in subdiv(pts, 4)], c, wd, b * face)
    # 테두리 (휜 모양) + 바깥 끝 흰 빛줄
    qs = [LW * k / 16 for k in range(17)]
    ys = [TOP + (BOT - TOP) * k / 16 for k in range(17)]
    outline = [P(q, TOP)[0] for q in qs] + [P(LW, y)[0] for y in ys] + [P(q, BOT)[0] for q in reversed(qs)] + [P(0, y)[0] for y in reversed(ys)]
    M.ln(outline, LAV, 1.2, 0.5 + 0.4 * face)
    M.ln([P(LW - 3, y)[0] for y in ys], WHITE, 1.0, 0.25 + 0.45 * math.sin(math.pi * u))
    # 덮기 (장 면 색: 눕힌 정도로 조금 밝게) → 그림자 → 장 그림
    d = ImageDraw.Draw(img)
    shade = int(8 + 7 * face)
    # 그림자: 장 아래 드러나는 쪽, 장 쪽으로 진해짐
    s = math.sin(math.pi * u)
    sh = Image.new('RGB', img.size, (0, 0, 0))
    sd = ImageDraw.Draw(sh)
    edge_x = P(LW, (TOP + BOT) / 2)[0][0]
    for k in range(40):
        x = edge_x + (k if u < 0.5 else -k) * 2.5
        if (u < 0.5 and x > RX + LW) or (u >= 0.5 and x < LX): break
        v = int(26 * s * (1 - k / 40) ** 2)
        sd.line([(x, TOP), (x, BOT)], fill=(v, v, v), width=3)
    img = ImageChops.subtract(img, sh)
    d = ImageDraw.Draw(img)
    d.polygon(outline, fill=(shade, shade, shade + 6))
    lay = M.draw()
    return ImageChops.add(img, ImageChops.subtract(lay, Image.new('RGB', lay.size, (6, 6, 11))))


# ---------------------------------------------------------------- 한 장면
def render(tab, left, right, sheets=(), text=1.0, press=0.0, ext=None):
    """text: 쪽 글자 보임 정도 (넘어가는 동안 0, 다 넘어가면 0 → 1)"""
    def layer(with_text):
        M = Sign(0.8); M.small_glow = 0.0
        CB.book(M, active=tab, ext=ext)
        if with_text:
            left[0](M, *left[1:])
            right[0](M, *right[1:])
        CB.footer(M, press)
        return M.draw()
    img = layer(True) if text >= 1 else Image.blend(layer(False), layer(True), text) if text > 0 else layer(False)
    for u in sheets:
        if 0 < u < 1: img = draw_sheet(img, u)
    return img.resize((960, 540), Image.LANCZOS)


def main(out):
    os.makedirs(out, exist_ok=True)
    F = []
    L1, R1 = (detail_cocktail, 5), (list_cocktail, 0)
    L2, R2 = (detail_cocktail, 12), (list_cocktail, 1)
    LJ, RJ = (detail_joker, 1), (list_joker,)
    F += [render(0, L1, R1)] * 8
    F += [render(0, L1, R1, press=p) for p in (0.5, 1.0)]  # 1 › 눌림
    n = int(0.5 * FPS)
    for f in range(n):  # 2 ~ 5 한 장 넘어감 (아래로 다음 쪽이 드러남)
        F.append(render(0, L1, R2, sheets=[ease((f + 1) / n)], press=max(0.0, 1 - f / 3)))
    a, b = render(0, L1, R2), render(0, L2, R2)
    F += [Image.blend(a, b, (k + 1) / 4) for k in range(4)]  # 6 왼쪽 상세가 겹쳐 바뀜
    F += [b] * 12
    # 탭 바꾸기: 조커 탭이 밀려 나오고 세 장이 겹쳐 넘어감
    m = int(0.32 * FPS)
    tot = m + 2 * int(0.1 * FPS) + 1
    for f in range(tot):
        e = ease(min(1.0, (f + 1) / 5))
        ext = [10 * (1 - e), 10 * e, 0, 0]
        us = [ease((f - k * int(0.1 * FPS) + 1) / m) for k in range(3)]
        F.append(render(1, L2, RJ, sheets=us, ext=ext))
    a, b = render(1, L2, RJ, ext=[0, 10, 0, 0]), render(1, LJ, RJ)
    F += [Image.blend(a, b, (k + 1) / 4) for k in range(4)]
    F += [b] * 18
    pal = [f.convert('P', palette=Image.ADAPTIVE, colors=200) for f in F]
    pal[0].save(os.path.join(out, 'codex_flip.gif'), save_all=True, append_images=pal[1:], duration=int(1000 / FPS), loop=0)
    F[10 + n // 2].save(os.path.join(out, 'codex_flip_mid.png'))
    F[10 + n // 4].save(os.path.join(out, 'codex_flip_early.png'))
    print('ok', len(F))


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
