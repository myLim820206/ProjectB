"""스팀덱 작은 글자 시안: 지금 vs '글자 크게'(작은 글자만 키움 + 키 안내 숨김)

    python tools/neon/deck_text_mock.py 출력폴더   → deck_text_mock.png

스팀덱 1280×800 에 설계 화면(1280×720)이 1배로 들어가 설계 1px = 덱 1px.
글자는 언리얼과 같은 글꼴(엔진 Roboto + DroidSansFallback)과 같은 크기 계산(Px × 0.75 pt → 반올림 → 96 DPI 픽셀)으로 그린다.
위: 덱 화면 픽셀 그대로(1:1). 아래: 덱 실제 크기 느낌 — 덱은 약 215 PPI 라 보통 모니터(약 100 ~ 110 PPI)에서 절반으로 줄여야 덱에서 보이는 크기와 비슷하다.
크게 규칙(안): 16px 미만만 Px + (16 − Px) × 0.375  → 9→11.6, 10→12.3, 11→12.9, 12→13.5, 13→14.1, 14→14.8. 16px 이상은 그대로
"""
import os
import sys

from PIL import Image, ImageDraw, ImageFont

ENG = 'C:/Program Files/Epic Games/UE_5.8/Engine/Content/Slate/Fonts/'
LAT = {False: ENG + 'Roboto-Regular.ttf', True: ENG + 'Roboto-Bold.ttf'}
CJK = ENG + 'DroidSansFallback.ttf'
KO_UI = 'C:/Windows/Fonts/malgunbd.ttf'
BG, PANEL = (6, 6, 11), (12, 14, 22)
FG, DIM, GREY = (207, 216, 230), (95, 104, 120), (138, 148, 166)
GOLD, GREEN, PINK, CYAN, LAV = (255, 210, 110), (156, 255, 176), (255, 106, 170), (110, 220, 255), (168, 150, 225)
W, H = 1280, 760


def big(px):
    return px + (16 - px) * 0.375 if px < 16 else px


def font_px(px):
    """언리얼 FCanvas::Font 와 같은 크기: pt = round(Px × 0.75), 96 DPI 픽셀 = pt × 4/3"""
    return max(1, round(px * 0.75)) * 4 / 3


class T:
    def __init__(self, d, k=1.0):
        self.d, self.k = d, k

    def text(self, x, y, s, c, px, bold=False, align=0, scale=False):
        """y = 기준선 (언리얼 Text 와 같이). 한글은 DroidSansFallback, 그 밖은 Roboto"""
        if scale: px = big(px)
        fp = font_px(px) * self.k
        fl, fc = ImageFont.truetype(LAT[bold], max(1, int(round(fp)))), ImageFont.truetype(CJK, max(1, int(round(fp))))
        parts = [(ch, fc if ord(ch) > 0x2E80 else fl) for ch in s]
        width = sum(f.getlength(ch) for ch, f in parts)
        cx = x * self.k - (width / 2 if align == 1 else width if align == 2 else 0)
        for ch, f in parts:
            self.d.text((cx, y * self.k), ch, font=f, fill=c, anchor='ls')
            if bold and f is fc: self.d.text((cx + 0.5, y * self.k), ch, font=f, fill=c, anchor='ls')  # 가짜 굵게 (DroidSansFallback 은 굵은 글꼴이 없다)
            cx += f.getlength(ch)

    def rr(self, x, y, w, h, r, c, wd=1):
        k = self.k
        self.d.rounded_rectangle([x * k, y * k, (x + w) * k, (y + h) * k], radius=r * k, outline=c, width=max(1, int(wd * k + 0.5)))

    def box(self, x, y, w, h, c):
        k = self.k
        self.d.rectangle([x * k, y * k, (x + w) * k, (y + h) * k], fill=c)


def column(t, ox, oy, deck):
    """화면 조각 셋: 칸 메뉴 · 오른쪽 패널(트리 버튼 · 칵테일) · 왼쪽 족보 줄 + 아래 도움말. deck=True 면 크게 + 키 안내 숨김"""
    S = deck
    # 칸 메뉴 (타워 클릭)
    x, y = ox, oy
    t.box(x, y, 290, 236, PANEL); t.rr(x, y, 290, 236, 8, GOLD)
    t.text(x + 12, y + 20, '3 연사 · 합성 레벨 1', GOLD, 13, True, scale=S)
    t.text(x + 12, y + 38, '위력 2.7k/초 (판의 0%)', GREEN, 11, True, scale=S)
    t.text(x + 12, y + 55, '공격력 456 · 초당 2.6발 · 치명 5% ×1.50', FG, 10, scale=S)
    t.text(x + 12, y + 72, '사거리 2.8 · 범위 1.5', GREY, 10, scale=S)
    t.text(x + 12, y + 89, '플러시4 연쇄×7.5 · 투 페어 연쇄×7.5', GOLD, 10, scale=S)
    t.text(x + 12, y + 106, '특성 공격 ×1.2 연사 ×1.1', GOLD, 10, scale=S)
    for i, (lab, right, c) in enumerate((('합성 · 플러시4 연쇄×7.5', '4장', GOLD), ('판매', '+5 경험치', GREY))):
        by = y + 140 + i * 36
        t.rr(x + 9, by, 272, 30, 15, c)
        t.text(x + 25, by + 19, lab, (255, 255, 255) if i == 0 else c, 12, True, scale=S)
        t.text(x + 267, by + 19, right, GREY, 10, align=2, scale=S)
    # 오른쪽 패널 조각: 스킬 트리 버튼 + 칵테일 칸
    x = ox + 310
    t.rr(x, y, 250, 36, 18, LAV)
    t.text(x + 119, y + 23, '스킬 트리', FG, 13, True, align=1, scale=S)
    if not deck: t.text(x + 241, y + 22, 'K', DIM, 9, align=2)
    t.box(x, y + 48, 250, 120, PANEL); t.rr(x, y + 48, 250, 120, 12, LAV)
    t.text(x + 14, y + 70, '칵테일 2/5', GOLD, 12, True, scale=S)
    t.text(x + 236, y + 70, '다음 30웨이브', DIM, 9, align=2, scale=S)
    for k in range(5):
        cx = x + 26 + k * 49.5
        t.rr(cx - 12, y + 92, 24, 30, 5, PINK if k < 2 else DIM)
    t.text(x + 14, y + 150, '잔에 마우스: 이름 · 설명' if not deck else '잔을 누르면 이름 · 설명', DIM, 9, scale=S)
    # 왼쪽 족보 줄 둘
    yy = y + 184
    for i, (nm, mul, sub) in enumerate((('플러시', '×7.5', '같은 무늬 5장'), ('투 페어', '×2.0', '같은 숫자 두 쌍'))):
        ry = yy + i * 36
        t.rr(x, ry, 250, 32, 6, CYAN if i == 0 else DIM)
        t.text(x + 12, ry + 15, nm, FG, 12, True, scale=S)
        t.text(x + 238, ry + 15, mul, GOLD, 11, True, align=2, scale=S)
        t.text(x + 12, ry + 28, sub, DIM, 9, scale=S)
    # 아래 도움말 줄
    if not deck: t.text(ox + 560, oy + 272, 'Tab: 다음 판 · ←→: 쪽 · Esc · 오른쪽 클릭: 닫기', DIM, 10, align=2)
    else: t.text(ox + 560, oy + 272, '(키 안내 줄은 덱에서 숨김 · 족보 줄 설명이 겹침 → 줄 높이 늘려야)', DIM, 10, align=2, scale=S)


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    img = Image.new('RGB', (W, H), BG)
    d = ImageDraw.Draw(img)
    ui = lambda x, y, s, c, px, a='ls': d.text((x, y), s, font=ImageFont.truetype(KO_UI, px), fill=c, anchor=a)  # noqa: E731
    ui(40, 34, '덱 화면 픽셀 그대로 (1:1)', (255, 255, 255), 16)
    ui(40, 60, '지금', FG, 14); ui(680, 60, '글자 크게 (16px 미만만 키움) + 키 안내 숨김', FG, 14)
    t1 = T(d, 1.0)
    column(t1, 40, 76, False)
    column(t1, 680, 76, True)
    d.line([(20, 372), (1260, 372)], fill=DIM, width=1)
    ui(40, 400, '덱 실제 크기 느낌 (보통 모니터에서 ½ 로 줄임 — 덱 215 PPI, 모니터의 약 2배 촘촘)', (255, 255, 255), 16)
    t2 = T(d, 0.5)
    half = Image.new('RGB', (W, H), BG)
    dh = ImageDraw.Draw(half)
    t2 = T(dh, 1.0)
    column(t2, 40, 20, False)
    column(t2, 680, 20, True)
    small = half.crop((0, 0, W, 320)).resize((W // 2, 160), Image.LANCZOS)
    img.paste(small.crop((0, 0, 320, 160)), (60, 430))
    img.paste(small.crop((320, 0, 640, 160)), (700, 430))
    ui(60, 612, '지금', GREY, 13); ui(700, 612, '크게 + 키 안내 숨김', GREY, 13)
    ui(640, 700, '크게 규칙(안): 9→11.6 · 10→12.3 · 11→12.9 · 12→13.5 · 13→14.1 · 14→14.8 (16 이상 그대로). 칸 크기는 지금 그대로라 몇 곳은 자리를 늘려야 함', DIM, 13, 'ms')
    img.save(os.path.join(outdir, 'deck_text_mock.png'))
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
