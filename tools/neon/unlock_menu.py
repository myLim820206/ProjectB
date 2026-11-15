"""잠금 해제 화면 시안 — 바 메뉴판 느낌 (D-256)

    python tools/neon/unlock_menu.py 출력폴더

A 벽 메뉴 보드: 금색 두 겹 틀의 큰 판 하나, 묘약 60개를 3단 × 20줄로 '이름 ······ 가격', 아래 띠에 고른 줄 설명 + 사기 버튼
B 메뉴북: 펼친 두 쪽. 왼쪽 쪽 = 2단 목록 (쪽 넘김), 오른쪽 쪽 = 고른 항목 크게 (그림 틀, 설명, 가격, 사기 버튼)
C 걸린 메뉴판 셋: 가운데 큰 판이 지금 탭(묘약), 양옆 작은 판은 조커 · 상위 조커 (눌러서 바꿈), 줄은 2단 + 작은 그림
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, spade_crown, PINK, CYAN, GOLD, WHITE  # noqa: E402
from logo_retro import KO, BLOCK, SCRIPT  # noqa: E402
from neon import suit, circle  # noqa: E402

CHARMS = ['하트 애호가', '스페이드 애호가', '다이아 애호가', '클로버 애호가', '연결고리', '쌍둥이', '일직선', '넓은 물결', '그림 카드', '작은 수', '행운의 7', '큰 손', '보급', '재활용', '조준경', '서리', '연쇄 폭발', '관통탄',
          '에이스', '짝수', '홀수', '풀하우스 팬', '네잎클로버', '왕실 근위', '유리 대포', '장거리 사격', '대폭발', '과부하', '조커 팬', '먹보', '대장간 불꽃', '현상금',
          '여왕 친위대', '왕의 눈', '연쇄 열기', '긴 손', '셋의 힘', '외톨이', '와일드 하트', '전령', '보스 사냥꾼', '청소부', '잭 경호원', '2의 반란', '무지개', '스트레이트 플러시 팬', '투 페어 팬', '트리플 팬', '포카드 팬', '두 장의 힘', '네 장의 힘',
          '재빠른 합성', '큰 조커', '저격 망원경', '얼음 결정', '연발', '낡은 회중시계', '첫 박자', '먹이 사슬', '손재주']
DESC = '♥ 타워 공격력 +40%'
JOKERS = ['광대', '모자 장수', '웃는 가면', '희비극', '할리퀸', '지휘봉', '잭', '체셔', '방울', '스타']
DIMC = (95, 105, 125)
CHIPS = 1240
SEL = 13  # 고른 줄 (재활용: 잠김, 살 수 있음)
SEL_DESC = '카드를 버리면 1장 뽑기'


def price(j): return 0 if j < 12 else 60 + (j % 7) * 30 + (j // 20) * 50


def is_open(j): return j < 12 or j in (14, 19, 23)


def chip(M, x, y, txt, c=GOLD):
    w = 40 + len(txt) * 11
    M.rr(x, y, w, 30, 15, c, 1.4)
    for p in suit('diamond')(x + 16, y + 15, 6): M.ln(p, c, 1.2)
    M.word(x + 30, y + 21, txt, c, 14, None, font=KO, anchor_center=False)


def tube2(M, x, y, w, h, r, c, b=1.0, wd=2.2):
    """네온 간판 두 겹 틀 (D-254/255): 바깥 + 안쪽 가는 튜브, 양 끝 볼트"""
    M.rr(x, y, w, h, r, c, wd, b)
    M.rr(x + 6, y + 6, w - 12, h - 12, max(2, r - 6), c, 1.0, 0.75 * b)
    for bx, by in ((x + 3 + 6, y + 3 + 6), (x + w - 9, y + 9), (x + 9, y + h - 9), (x + w - 9, y + h - 9)):
        M.ln(circle(bx, by, 2.0, 10), (120, 125, 140), 1.0, 0.6)


def leader(M, x0, x1, y, c, b=0.5):
    """이름과 가격 사이 점선 (메뉴판 리더)"""
    x = x0
    while x < x1:
        M.dots.append((x, y, 0.55)) if b > 0.7 else M.ln([(x, y), (x + 1.2, y)], c, 1.0, b)
        x += 6


def row(M, x, y, w, j, sel=False, px=13):
    op, pr = is_open(j), price(j)
    can = not op and CHIPS >= pr
    name = CHARMS[j]
    nc = CYAN if op else GOLD if can else DIMC
    nb = 0.75 if op else 1.0 if can else 0.55
    if sel:
        nc, nb = WHITE, 1.0
        for p in suit('heart')(x - 12, y - 5, 5): M.ln(p, PINK, 1.2)
        M.ln([(x - 2, y + 6), (x + w, y + 6)], PINK, 1.4)
    M.word(x, y, name, nc, px, [nb] * len(name), font=KO, anchor_center=False)
    tw = len(name) * px * 0.95
    pt = '열림' if op else f'{pr}'
    pc = (138, 255, 196) if op else GOLD if can else DIMC
    M.word(x + w - len(pt) * px * 0.62, y, pt, pc, px, [1.0 if (can or op or sel) else 0.6] * len(pt), font=KO, anchor_center=False)
    leader(M, x + tw + 6, x + w - len(pt) * px * 0.62 - 8, y - 4, DIMC, 0.45)


def menu_header(M, cx, y, title, sub, c=GOLD):
    M.word(cx, y, title, c, 30, None, track=6, font=KO)
    M.word(cx, y + 24, sub, c, 12, [0.8] * len(sub), track=8, font=BLOCK)
    M.ln([(cx - 190, y - 10), (cx - 90, y - 10)], c, 1.2, 0.7); M.ln([(cx + 90, y - 10), (cx + 190, y - 10)], c, 1.2, 0.7)
    for s in (-1, 1):
        for p in suit('spade')(cx + s * 205, y - 10, 6): M.ln(p, c, 1.2)


def tabs(M, cx, y, on):
    names = ['묘약', '조커', '상위 조커']
    w = 130
    x0 = cx - (3 * w + 2 * 14) / 2
    for i, n in enumerate(names):
        x = x0 + i * (w + 14)
        c = PINK if i == on else CYAN
        tube2(M, x, y, w, 36, 18, c, 1.0 if i == on else 0.8, 2.0 if i == on else 1.6)
        M.word(x + w / 2, y + 24, n, WHITE if i == on else (216, 244, 255), 14, None, font=KO)


def buy_btn(M, x, y, w, h, txt, c=PINK):
    tube2(M, x, y, w, h, h / 2, c, 1.0)
    M.word(x + w / 2, y + h / 2 + 6, txt, WHITE, 16, None, font=KO)


# ---------------------------------------------------------------- A 벽 메뉴 보드
def unlock_A():
    M = Sign(0.75)
    M.small_glow = 0.0  # 글이 많은 메뉴판: 작은 글자는 빛번짐 없이 (D-257)
    chip(M, 1090, 20, f'칩 {CHIPS:,}')
    tube2(M, 40, 70, 1200, 560, 18, GOLD, 0.85)
    menu_header(M, 640, 112, '잠금 해제', 'TODAY  ·  MENU')
    tabs(M, 640, 140, 0)
    colw, x0 = 340, 110
    for j in range(60):
        c, r = j // 20, j % 20
        row(M, x0 + c * 380, 212 + r * 19.5, colw, j, j == SEL, 12)
    # 아래 띠: 고른 줄 설명 + 사기
    M.ln([(70, 610), (1210, 610)], GOLD, 1.0, 0.5)
    M.word(70, 662, CHARMS[SEL], WHITE, 18, None, font=KO, anchor_center=False)
    M.word(70, 688, SEL_DESC + '  ·  연 묘약만 5웨이브 보상에 나온다', (200, 210, 225), 12, None, font=KO, anchor_center=False)
    buy_btn(M, 1010, 644, 200, 48, f'{price(SEL)}칩  열기')
    return M.draw()


# ---------------------------------------------------------------- B 메뉴북 (두 쪽)
def unlock_B():
    M = Sign(0.75)
    M.small_glow = 0.0  # 글이 많은 메뉴판: 작은 글자는 빛번짐 없이 (D-257)
    chip(M, 1090, 20, f'칩 {CHIPS:,}')
    tabs(M, 640, 22, 0)
    # 책: 두 쪽 + 가운데 접힘
    tube2(M, 60, 80, 1160, 600, 16, GOLD, 0.85)
    M.ln([(640, 92), (640, 668)], GOLD, 1.2, 0.5)
    M.word(350, 128, '묘약', GOLD, 24, None, track=6, font=KO)
    M.word(350, 150, 'POTIONS', GOLD, 11, [0.7] * 7, track=8, font=BLOCK)
    for k in range(30):
        c, r = k // 15, k % 15
        row(M, 100 + c * 270, 190 + r * 30, 240, k, k == SEL, 13)
    # 쪽 넘김 점
    for i in range(2):
        M.ln(circle(330 + i * 20, 655, 4, 12), GOLD, 1.2, 1.0 if i == 0 else 0.35)
    M.word(120, 660, '‹', GOLD, 22, None, font=KO); M.word(580, 660, '›', GOLD, 22, None, font=KO)
    # 오른쪽 쪽: 고른 항목
    cx = 930
    tube2(M, cx - 70, 130, 140, 140, 16, CYAN, 1.0)
    # 재활용 그림: 카드 한 장 + 도는 화살표
    M.rr(cx - 16, 176, 32, 46, 5, CYAN, 1.6)
    arc_pts = [(cx + 42 * math.cos(math.radians(a)), 200 + 42 * math.sin(math.radians(a))) for a in range(-60, 230, 10)]
    M.ln(arc_pts, CYAN, 1.8)
    ex, ey = arc_pts[-1]
    M.ln([(ex - 10, ey - 2), (ex, ey), (ex + 2, ey - 11)], CYAN, 1.8)
    M.word(cx, 320, CHARMS[SEL], WHITE, 26, None, font=KO)
    M.word(cx, 354, SEL_DESC, (210, 220, 235), 15, None, font=KO)
    M.ln([(cx - 200, 384), (cx + 200, 384)], GOLD, 1.0, 0.4)
    M.word(cx, 420, '연 묘약만 5웨이브 보상에 나온다', DIMC, 12, None, font=KO)
    M.word(cx, 476, f'{price(SEL)} 칩', GOLD, 30, None, font=KO)
    buy_btn(M, cx - 120, 510, 240, 54, '열기')
    M.word(cx, 610, '열린 묘약 15 / 60', (138, 255, 196), 13, None, font=KO)
    return M.draw()


# ---------------------------------------------------------------- C 걸린 메뉴판 셋
def joker_icon(M, x, y, r, c, b=1.0):
    """광대 모자 (세 갈래 + 방울)"""
    M.ln([(x - r, y + r * 0.6), (x - r * 0.9, y - r * 0.5), (x - r * 0.3, y + r * 0.1), (x, y - r), (x + r * 0.3, y + r * 0.1), (x + r * 0.9, y - r * 0.5), (x + r, y + r * 0.6), (x - r, y + r * 0.6)], c, 1.5, b)
    for px, py in ((x - r * 0.9, y - r * 0.5), (x, y - r), (x + r * 0.9, y - r * 0.5)):
        M.ln(circle(px, py - 3, 2.5, 10), c, 1.2, b)


def unlock_C():
    M = Sign(0.75)
    M.small_glow = 0.0  # 글이 많은 메뉴판: 작은 글자는 빛번짐 없이 (D-257)
    chip(M, 1090, 20, f'칩 {CHIPS:,}')
    # 천장 줄 (판을 매단 선)
    for x in (300, 980): M.ln([(x, 0), (x, 70)], DIMC, 1.0, 0.5)
    for x in (95, 185, 1095, 1185): M.ln([(x, 0), (x, 200)], DIMC, 1.0, 0.4)
    # 양옆 작은 판: 조커 · 상위 조커
    for i, (x, t, sub) in enumerate(((40, '조커', 'JOKERS'), (1040, '상위 조커', 'UPPER'))):
        tube2(M, x, 200, 200, 150, 14, CYAN, 0.75, 1.6)
        joker_icon(M, x + 100, 250, 18, CYAN, 0.8)
        M.word(x + 100, 302, t, (216, 244, 255), 18, None, font=KO)
        M.word(x + 100, 324, sub, CYAN, 10, [0.7] * len(sub), track=6, font=BLOCK)
        M.word(x + 100, 380, '눌러서 보기', DIMC, 11, None, font=KO)
    # 가운데 큰 판: 묘약
    tube2(M, 260, 70, 760, 620, 18, PINK, 1.0)
    menu_header(M, 640, 116, '묘약', 'POTIONS  ·  60', PINK)
    # 줄: 2단 × 15 + 작은 아이콘 (무늬)
    sts = ['heart', 'spade', 'diamond', 'club']
    cols = [PINK, CYAN, GOLD, (138, 255, 196)]
    for k in range(30):
        c, r = k // 15, k % 15
        x, y = 310 + c * 360, 180 + r * 30
        M.rr(x, y - 15, 20, 20, 5, cols[k % 4], 1.0, 0.7 if is_open(k) else 0.35)
        for p in suit(sts[k % 4])(x + 10, y - 5, 5): M.ln(p, cols[k % 4], 1.0, 0.8 if is_open(k) else 0.4)
        row(M, x + 30, y, 290, k, k == SEL, 13)
    M.ln([(290, 640), (990, 640)], PINK, 1.0, 0.4)
    M.word(300, 672, f'{CHARMS[SEL]}  ·  {SEL_DESC}', WHITE, 14, None, font=KO, anchor_center=False)
    buy_btn(M, 820, 650, 170, 40, f'{price(SEL)}칩 열기')
    M.word(640, 712, '1 / 2 쪽  ·  휠: 넘기기', DIMC, 11, None, font=KO)
    return M.draw()


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    for name, fn in (('unlock_A_board', unlock_A), ('unlock_B_book', unlock_B), ('unlock_C_hanging', unlock_C)):
        fn().save(os.path.join(outdir, name + '.png'))
        print(name)


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
