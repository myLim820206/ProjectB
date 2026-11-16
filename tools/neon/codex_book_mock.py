"""도감을 책자 메뉴판으로 (시안). 펼친 메뉴 책 두 쪽, 오른쪽 옆면에 색인 탭: 위에서부터 칵테일 · 조커 · 상위 조커

    python tools/neon/codex_book_mock.py 출력폴더   → codex_book_A.png, codex_book_B.png

A 양쪽 목록: 왼쪽 · 오른쪽 쪽 모두 메뉴 줄 ('그림 이름 ····· 상태'), 줄에 마우스 = 지금처럼 툴팁
B 왼쪽 상세 + 오른쪽 목록: 오른쪽 쪽은 메뉴 줄, 왼쪽 쪽은 고른(마우스 올린) 항목을 크게 — 잔 그림 · 이름 · 효과 · 상태. 툴팁 대신
공통: 두꺼운 메뉴판 겉표지(둥근 사각 두 겹) + 가운데 책등(접힌 선 + 꿰맨 점선), 쪽 아래 쪽수 ‹ 1 / 3 ›, 오른쪽 위 닫기
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, PINK, CYAN, GOLD, WHITE, FONT_KO, FONT_EN, hx  # noqa: E402

LAV, DIM, FG, MINT, BG2 = hx('a896e1'), hx('5f6878'), hx('dce4f0'), hx('8affc4'), hx('2a2236')
COVER = hx('c98a5a')  # 가죽 겉표지 느낌의 구릿빛 테
NAMES = ['핑크 하트 사워', '블랙 스페이드 마티니', '골드 다이아 피즈', '민트 클로버 줄렙', '체인 펀치', '트윈 하이볼', '스트레이트 업', '블루 웨이브',
         '코트 로열', '미니 샷', '럭키 세븐', '더블 푸어', '하우스 리필', '원스 모어', '아이 오프너', '프로스트 다이키리', '플레이밍 볼', '스크루드라이버',
         '에이스 오브 진', '이븐 스플릿', '오드볼', '풀하우스 펀치', '네잎 클로버 클럽', '글라스 캐논']
COLS = [hx('ff6aaa'), hx('e8eeff'), hx('ffd26e'), hx('8fe9ff'), hx('b48cff')]
BX, BY, BW, BH = 40, 60, 1140, 610  # 책 바깥 (오른쪽에 옆면 탭 자리 60)
SPX = BX + BW / 2  # 책등


def glass(M, kind, cx, by, s, c, b=1.0):
    """고르기 화면과 같은 잔 (0 마티니 1 하이볼 2 온더록), by = 바닥"""
    P = lambda x, y: (cx + x * s, by + y * s)  # noqa: E731
    if kind == 0:
        M.ln([P(-26, -70), P(26, -70), P(0, -36), P(-26, -70)], c, 1.6, b)
        M.ln([P(0, -36), P(0, -4)], c, 1.4, b); M.ln([P(-16, -2), P(16, -2)], c, 1.4, b)
        for j in range(4):
            hw = 22 * (1 - j * 6 / 30); M.ln([P(-hw, -64 + j * 6), P(hw, -64 + j * 6)], c, 0.8, 0.5 * b)
    elif kind == 1:
        M.ln([P(-18, -78), P(-16, -2), P(16, -2), P(18, -78)], c, 1.6, b)
        M.ln([P(6, -90), P(-4, -30)], WHITE, 1.0, 0.6 * b)
        for j in range(6): M.ln([P(-15, -62 + j * 10), P(15, -62 + j * 10)], c, 0.8, 0.5 * b)
    else:
        M.ln([P(-24, -46), P(-22, -2), P(22, -2), P(24, -46)], c, 1.6, b)
        for j in range(3): M.ln([P(-21, -34 + j * 10), P(21, -34 + j * 10)], c, 0.8, 0.5 * b)
        M.ln([P(-12, -30), P(-2, -38), P(8, -30), P(-2, -22), P(-12, -30)], WHITE, 1.0, 0.6 * b)


def book(M):
    # 겉표지 (두 겹) + 쪽 (조금 안쪽) + 책등
    M.rr(BX, BY, BW, BH, 18, COVER, 2.2, 0.9)
    M.rr(BX + 6, BY + 6, BW - 12, BH - 12, 14, COVER, 0.8, 0.45)
    for side in (-1, 1):
        x0 = BX + 22 if side < 0 else SPX + 6
        M.rr(x0, BY + 22, BW / 2 - 28, BH - 44, 8, LAV, 1.2, 0.55)
    M.ln([(SPX, BY + 14), (SPX, BY + BH - 14)], COVER, 1.6, 0.8)
    for y in range(BY + 30, BY + BH - 24, 14): M.ln([(SPX - 1, y), (SPX - 1, y + 6)], COVER, 1.0, 0.5)  # 꿰맨 점선
    # 책갈피 탭: 책 오른쪽 옆면에 붙어 밖으로 나온 색인 탭, 위에서부터 칵테일 · 조커 · 상위 조커 (고른 탭은 더 길게 나옴)
    tabs = [('칵테일', True), ('조커', False), ('상위 조커', False)]
    ex = BX + BW  # 책 오른쪽 옆면
    for i, (nm, on) in enumerate(tabs):
        y0, h = BY + 40 + i * 96, 84
        w = 74 if on else 64
        c = PINK if on else LAV
        M.ln([(ex, y0), (ex + w - 10, y0), (ex + w, y0 + 10), (ex + w, y0 + h - 10), (ex + w - 10, y0 + h), (ex, y0 + h)], c, 1.8 if on else 1.2, 1.0 if on else 0.55)
        if on: M.ln([(ex, y0 + 4), (ex, y0 + h - 4)], hx('06060b'), 3.0, 1.0)  # 고른 탭은 책 테와 이어진 듯 (경계선 지움)
        M.word(ex + w / 2 + 2, y0 + h / 2 + 5, nm, WHITE if on else FG, 13 if len(nm) > 3 else 14, font=FONT_KO)
    # 닫기
    M.rr(1234, 24, 32, 28, 14, PINK, 1.2, 0.7)
    M.ln([(1245, 33), (1255, 43)], WHITE, 1.2, 0.9); M.ln([(1255, 33), (1245, 43)], WHITE, 1.2, 0.9)


def head(M, x, w, title, sub):
    M.word(x + w / 2, BY + 66, title, PINK, 28, font=FONT_KO)
    M.word(x + w / 2, BY + 88, sub, PINK, 11, font=FONT_KO)
    for s in (-1, 1):
        M.ln([(x + w / 2 + s * 70, BY + 56), (x + w / 2 + s * 150, BY + 56)], PINK, 1.0, 0.6)


def rows(M, x0, y0, w, names, start, hover=None):
    for i, nm in enumerate(names):
        k = start + i
        y = y0 + i * 34
        have, opened = k in (1, 5), k < 14
        c = COLS[k % 5] if opened else DIM
        glass(M, k % 3, x0 + 14, y + 13, 0.26, c, 1.0 if opened else 0.4)
        nc = WHITE if hover == k else GOLD if have else CYAN if opened else DIM
        name = nm if opened else '???'
        tw = M.word(x0 + 34, y + 6, name, nc, 13, font=FONT_KO, anchor_center=False)
        right = '가진 칵테일' if have else '열림' if opened else '잠김'
        rc = GOLD if have else MINT if opened else DIM
        rw = M.word(x0 + w, y + 6, right, rc, 12, font=FONT_KO, anchor_center=False) if False else len(right) * 12
        M.word(x0 + w - rw, y + 6, right, rc, 12, font=FONT_KO, anchor_center=False)
        for dx in range(int(x0 + 40 + tw), int(x0 + w - rw - 8), 6): M.ln([(dx, y + 2), (dx + 1.2, y + 2)], DIM, 1.2, 0.6)
        if hover == k: M.ln([(x0 + 34, y + 12), (x0 + w, y + 12)], PINK, 1.0, 0.8)


def footer(M):
    M.word(SPX, BY + BH - 34, '‹   1 / 3 쪽   ›', DIM, 13, font=FONT_KO)


def variant_A(out):
    M = Sign(0.8); M.small_glow = 0.0
    book(M)
    lw = BW / 2 - 28
    lx, rx = BX + 22, SPX + 6
    head(M, lx, lw, '칵테일', 'COCKTAILS  ·  칵테일 14 / 41 열림')
    M.word(rx + lw / 2, BY + 78, '— 하우스 스페셜 —', LAV, 14, font=FONT_KO)
    rows(M, lx + 36, BY + 120, lw - 72, NAMES[:12], 0, hover=5)
    rows(M, rx + 36, BY + 120, lw - 72, NAMES[12:24], 12)
    footer(M)
    # 툴팁 (지금처럼)
    tx, ty = lx + 300, BY + 120 + 5 * 34 - 150
    M.rr(tx, ty, 300, 130, 8, GOLD, 1.2, 0.85)
    glass(M, 2, tx + 34, ty + 52, 0.5, COLS[0])
    M.word(tx + 64, ty + 38, '트윈 하이볼', WHITE, 18, font=FONT_KO, anchor_center=False)
    M.word(tx + 20, ty + 76, '같은 숫자 두 장이 맞닿으면 둘 다 공격 +30%', FG, 12, font=FONT_KO, anchor_center=False)
    M.word(tx + 20, ty + 108, '가진 칵테일', GOLD, 14, font=FONT_KO, anchor_center=False)
    M.word(640, 712, 'A 양쪽 목록 — 두 쪽에 메뉴 줄 24개, 줄에 마우스 = 툴팁 (지금 도감과 같은 정보)', DIM, 13, font=FONT_KO)
    M.draw().save(os.path.join(out, 'codex_book_A.png'))


def variant_B(out):
    M = Sign(0.8); M.small_glow = 0.0
    book(M)
    lw = BW / 2 - 28
    lx, rx = BX + 22, SPX + 6
    # 왼쪽 쪽: 고른 항목 크게
    cx = lx + lw / 2
    M.word(cx, BY + 70, 'TODAY\'S PICK', LAV, 12, font=FONT_EN, track=2)
    glass(M, 2, cx, BY + 300, 2.2, COLS[0])
    M.word(cx, BY + 350, '트윈 하이볼', WHITE, 30, font=FONT_KO)
    M.word(cx, BY + 376, 'TWIN HIGHBALL', PINK, 12, font=FONT_EN, track=2)
    M.ln([(cx - 160, BY + 396), (cx + 160, BY + 396)], LAV, 1.0, 0.5)
    M.word(cx, BY + 428, '같은 숫자 두 장이 맞닿으면 둘 다 공격 +30%', FG, 15, font=FONT_KO)
    M.word(cx, BY + 456, '연 칵테일만 5웨이브 보상에 나온다', DIM, 12, font=FONT_KO)
    M.rr(cx - 70, BY + 482, 140, 30, 15, GOLD, 1.3, 0.85)
    M.word(cx, BY + 503, '가진 칵테일', GOLD, 14, font=FONT_KO)
    # 오른쪽 쪽: 메뉴 줄
    head(M, rx, lw, '칵테일', 'COCKTAILS  ·  14 / 41 열림')
    rows(M, rx + 36, BY + 120, lw - 72, NAMES[:12], 0, hover=5)
    footer(M)
    M.word(640, 712, 'B 왼쪽 상세 + 오른쪽 목록 — 오른쪽 줄에 마우스(또는 클릭) = 왼쪽 쪽에 크게. 툴팁 없음, 한 쪽에 12줄', DIM, 13, font=FONT_KO)
    M.draw().save(os.path.join(out, 'codex_book_B.png'))


def main(out):
    os.makedirs(out, exist_ok=True)
    variant_A(out)
    variant_B(out)
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
