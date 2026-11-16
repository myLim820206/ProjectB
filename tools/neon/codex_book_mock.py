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


def book(M, active=0, ext=None):
    # 겉표지 (두 겹) + 쪽 (조금 안쪽) + 책등
    M.rr(BX, BY, BW, BH, 18, COVER, 2.2, 0.9)
    M.rr(BX + 6, BY + 6, BW - 12, BH - 12, 14, COVER, 0.8, 0.45)
    for side in (-1, 1):
        x0 = BX + 22 if side < 0 else SPX + 6
        M.rr(x0, BY + 22, BW / 2 - 28, BH - 44, 8, LAV, 1.2, 0.55)
    M.ln([(SPX, BY + 14), (SPX, BY + BH - 14)], COVER, 1.6, 0.8)
    for y in range(BY + 30, BY + BH - 24, 14): M.ln([(SPX - 1, y), (SPX - 1, y + 6)], COVER, 1.0, 0.5)  # 꿰맨 점선
    # 책갈피 탭: 책 오른쪽 옆면에 붙어 밖으로 나온 색인 탭, 위에서부터 칵테일 · 조커 · 상위 조커 (고른 탭은 더 길게 나옴)
    tabs = [(nm, i == active) for i, nm in enumerate(('칵테일', '조커', '상위 조커', '보스'))]  # 보스는 4번째 탭 (사용자 2026-11-16)
    ex = BX + BW  # 책 오른쪽 옆면
    for i, (nm, on) in enumerate(tabs):
        y0, h = BY + 30 + i * 92, 82
        w = 64 + (ext[i] if ext else (10 if on else 0))  # ext: 탭마다 더 나온 길이 (움직임용)
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


def footer(M, press=0.0):
    """쪽 넘기기는 책등에서 떼어 바깥 아래 귀퉁이에 (‹ 왼쪽 쪽 왼쪽 아래, › 오른쪽 쪽 오른쪽 아래), 쪽 번호는 각 쪽 아래 가운데 (책처럼)"""
    lw = BW / 2 - 28
    lx, rx = BX + 22, SPX + 6
    y = BY + BH - 44
    for x, s in ((lx + 18, '‹'), (rx + lw - 18 - 44, '›')):
        pr = press if s == '›' else 0.0
        M.rr(x, y - 14 + 2 * pr, 44, 26, 13, PINK if pr > 0 else CYAN, 1.2 + 0.8 * pr, 0.6 + 0.4 * pr)  # press: 눌림 (살짝 내려가며 분홍)
        M.word(x + 22, y + 6, s, FG, 18, font=FONT_KO)
    M.word(lx + lw / 2, y + 5, '— 1 —', DIM, 12, font=FONT_KO)
    M.word(rx + lw / 2, y + 5, '— 2 —', DIM, 12, font=FONT_KO)


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


JOKERS = [('광대', '무늬 4종 공격을 번갈아 씀', '하이 카드', '×1.0'), ('모자 장수', '주변 8칸 타워 공격력 +25% · 사거리 +10%', '원 페어', '×1.2'),
          ('웃는 가면', '주변 가장 강한 타워의 공격을 따라 함', '투 페어', '×1.3'), ('희비극', '두 적을 동시에 공격', '트리플', '×1.5'),
          ('할리퀸', '판 위 같은 무늬 타워 전부 공격력 +30%', '플러시', '×2.0'), ('지휘봉', '주변 8칸 타워 연사 +40%', '스트레이트', '×2.0'),
          ('잭', '아주 강한 한 방 (공격력 ×3, 연사 ×0.5)', '풀 하우스', '×2.5'), ('체셔', '사거리 무한: 가장 앞선 적 저격', '포카드', '×3.0'),
          ('방울', '8초마다 종: 사거리 안 적 1.5초 멈춤', '스티플', '×3.5'), ('스타', '판 전체 레이저: 모든 적에게 피해', '로열', '×4.0')]
KNOWN = {0, 1, 2, 4, 5}


def jester(M, cx, cy, r, c, b=1.0):
    """조커 그림 자리: 둥근 사각 틀 안 광대 모자 (세 갈래 + 방울) — 게임에선 JokerTower 스프라이트"""
    from neon import circle
    M.rr(cx - r, cy - r * 1.2, 2 * r, 2.4 * r, r * 0.3, c, 1.4, b)
    s = r * 0.62
    pts = [(cx - s, cy + s * 0.6), (cx - s * 0.9, cy - s * 0.5), (cx - s * 0.3, cy + s * 0.1), (cx, cy - s), (cx + s * 0.3, cy + s * 0.1), (cx + s * 0.9, cy - s * 0.5), (cx + s, cy + s * 0.6), (cx - s, cy + s * 0.6)]
    M.ln(pts, c, 1.4, b)
    for px, py in ((cx - s * 0.9, cy - s * 0.5), (cx, cy - s), (cx + s * 0.9, cy - s * 0.5)): M.ln(circle(px, py - s * 0.18, s * 0.14, 10), c, 1.1, b)


def chip(M, x, y, s, c):
    w = len(s) * 13 + 18
    M.rr(x, y - 13, w, 22, 11, c, 1.1, 0.75)
    M.word(x + w / 2, y + 3, s, c, 12, font=FONT_KO)
    return w


def variant_joker(out):
    M = Sign(0.8); M.small_glow = 0.0
    book(M, active=1)
    lw = BW / 2 - 28
    lx, rx = BX + 22, SPX + 6
    JP = hx('ff9ad5')
    # 오른쪽 쪽: 조커 10줄 (한 쪽에 다 들어가 쪽 넘김 없음)
    head(M, rx, lw, '조커', 'JOKERS  ·  5 / 10 발견')
    hover = 1
    for i, (nm, ab, cap, tier) in enumerate(JOKERS):
        y = BY + 128 + i * 42
        known = i in KNOWN
        c = JP if known else DIM
        jester(M, rx + 50, y - 4, 9, c, 1.0 if known else 0.4)
        nc = WHITE if i == hover else CYAN if known else DIM
        tw = M.word(rx + 72, y + 2, nm if known else '???', nc, 14, font=FONT_KO, anchor_center=False)
        M.word(rx + 72, y + 18, f'{cap} 대장', DIM, 10, font=FONT_KO, anchor_center=False)
        right = tier if known else '?'
        M.word(rx + lw - 36 - len(right) * 9, y + 2, right, MINT if known else DIM, 14, font=FONT_EN, anchor_center=False)
        for dx in range(int(rx + 80 + tw), int(rx + lw - 50 - len(right) * 9), 6): M.ln([(dx, y - 2), (dx + 1.2, y - 2)], DIM, 1.2, 0.6)
        if i == hover: M.ln([(rx + 72, y + 24), (rx + lw - 36, y + 24)], PINK, 1.0, 0.8)
    # 왼쪽 쪽: 고른 조커 크게
    cx = lx + lw / 2
    nm, ab, cap, tier = JOKERS[hover]
    jester(M, cx, BY + 170, 46, JP)
    M.word(cx, BY + 268, nm, WHITE, 30, font=FONT_KO)
    M.word(cx, BY + 292, f'{cap} 대장  ·  위력 {tier}', GOLD, 13, font=FONT_KO)
    M.ln([(cx - 170, BY + 310), (cx + 170, BY + 310)], LAV, 1.0, 0.5)
    M.word(cx, BY + 340, ab, FG, 15, font=FONT_KO)
    M.word(cx, BY + 364, f'지휘: {cap} 타워 위력 +30% (겹침)', JP, 12, font=FONT_KO)
    # 레시피: 족보 알약
    M.word(lx + 60, BY + 410, '레시피', LAV, 12, font=FONT_KO, anchor_center=False)
    x = lx + 60
    M.word(x, BY + 440, '3개', DIM, 11, font=FONT_KO, anchor_center=False)
    x += 30
    for k, h in enumerate(('원 페어', '투 페어', '하이 카드')):
        x += chip(M, x, BY + 436, h, CYAN) + 6
        if k < 2: M.word(x - 1, BY + 440, '+', DIM, 12, font=FONT_EN, anchor_center=False); x += 12
    x = lx + 60
    M.word(x, BY + 476, '2개', DIM, 11, font=FONT_KO, anchor_center=False)
    x += 30
    for k, h in enumerate(('원 페어', '트리플')):
        x += chip(M, x, BY + 472, h, CYAN) + 6
        if k < 1: M.word(x - 1, BY + 476, '+', DIM, 12, font=FONT_EN, anchor_center=False); x += 12
    M.word(cx, BY + 520, '족보 3개(또는 2개)를 골라 레시피와 맞으면 조커 · 판 어디에 있어도 된다', DIM, 11, font=FONT_KO)
    M.word(640, 712, 'B 조커 쪽 — 오른쪽: 10줄 (그림 · 이름 · 대장 족보 ····· 위력), 왼쪽: 고른 조커 크게 + 능력 · 지휘 · 레시피 알약. 모르는 조커는 ??? (레시피는 보임)', DIM, 12, font=FONT_KO)
    M.draw().save(os.path.join(out, 'codex_book_joker.png'))


def main(out):
    os.makedirs(out, exist_ok=True)
    variant_A(out)
    variant_B(out)
    variant_joker(out)
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
