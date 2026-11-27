"""첫 화면 · 버튼 · 영구 강화 화면 시안 3종 (DOC/art/15 네온 화풍)

    python tools/neon/title_mock.py 출력폴더

A 카드 테이블: 금색 + 에메랄드, 버튼 = 카드 모양, 제목 뒤 부채꼴 카드
B 이상한 나라: 분홍 + 하늘 두 색, 버튼 = 두 줄 알약, 체셔 웃음과 모자
C 금빛 미니멀: 금색 + 흰빛 한 톤, 버튼 = 왼쪽 막대가 있는 띠, 얇은 두 겹 틀
영구 강화는 줄 목록 대신 8칸 카드 (아이콘 + 레벨 고리 + 효과 + 버튼)
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from neon import _Mock, suit, joker, jester_hat, circle, arc  # noqa: E402

W, H = 1280, 720
hx = _Mock.hx
SUITS = ['spade', 'heart', 'diamond', 'club']
SCOL = {'spade': hx('7fb8ff'), 'heart': hx('ff6a8a'), 'diamond': hx('ffd96a'), 'club': hx('8affc4')}
FG, GRY, DIM = hx('cfd8e6'), hx('8a94a6'), hx('5f6878')
MENU = ['이어하기', '게임 시작', '영구 강화', '잠금 해제', '환경 설정', '게임 종료']
UPG = [('공격', '모든 타워 공격력 +3%', 'sword', 17), ('공격 속도', '모든 타워 연사 +2%', 'bolt', 11), ('치명타 확률', '+1%p', 'star', 12), ('치명타 배율', '+0.05', 'burst', 8),
       ('스페이드', '스페이드 공격력 +4%', 'spade', 20), ('하트', '하트 공격력 +4%', 'heart', 5), ('다이아', '다이아 공격력 +4%', 'diamond', 14), ('클로버', '클로버 공격력 +4%', 'club', 0)]


def shape(M, kind, cx, cy, r, c, wd=1.6):
    if kind in SCOL:
        for p in suit(kind)(cx, cy, r): M.ln(p, c, wd)
    elif kind == 'sword':
        M.ln([(cx - r * 0.7, cy + r * 0.7), (cx + r * 0.7, cy - r * 0.7)], c, wd)
        M.ln([(cx - r * 0.25, cy - r * 0.05), (cx + r * 0.05, cy + r * 0.25)], c, wd)
        M.ln([(cx + r * 0.7, cy - r * 0.7), (cx + r * 0.45, cy - r * 0.75), (cx + r * 0.75, cy - r * 0.45), (cx + r * 0.7, cy - r * 0.7)], c, wd)
    elif kind == 'bolt':
        M.ln([(cx + r * 0.2, cy - r), (cx - r * 0.45, cy + r * 0.1), (cx + r * 0.05, cy + r * 0.1), (cx - r * 0.2, cy + r)], c, wd)
    elif kind == 'star':
        P = []
        for k in range(11):
            a = -math.pi / 2 + k * math.pi / 5
            rr = r if k % 2 == 0 else r * 0.45
            P.append((cx + math.cos(a) * rr, cy + math.sin(a) * rr))
        M.ln(P, c, wd)
    elif kind == 'burst':
        for k in range(8):
            a = k * math.pi / 4
            M.ln([(cx + math.cos(a) * r * 0.35, cy + math.sin(a) * r * 0.35), (cx + math.cos(a) * r, cy + math.sin(a) * r)], c, wd)


def card(M, x, y, w, h, c, wd=1.6, ang=0.0, label=None, sk=None):
    """기울어진 카드 테두리 (+ 가운데 무늬)"""
    cx, cy = x + w / 2, y + h / 2
    ca, sa = math.cos(ang), math.sin(ang)
    def T(px, py): return (cx + (px - cx) * ca - (py - cy) * sa, cy + (px - cx) * sa + (py - cy) * ca)
    r = 8
    P = []
    for (ox, oy, a0) in ((x + w - r, y + r, -90), (x + w - r, y + h - r, 0), (x + r, y + h - r, 90), (x + r, y + r, 180)):
        for k in range(7):
            a = math.radians(a0 + k * 15)
            P.append(T(ox + math.cos(a) * r, oy + math.sin(a) * r))
    P.append(P[0])
    M.ln(P, c, wd)
    if sk:
        for p in suit(sk)(cx, cy, w * 0.18): M.ln([T(px, py) for px, py in p], c, wd)
    if label:
        lx, ly = T(x + 12, y + 16)
        M.tx(lx, ly, label, c, 13, 'mm')


def hud_chip(M, x, y, txt, c):
    w = 40 + len(txt) * 11
    M.rr(x, y, w, 30, 15, c, 1.4); M.box(x, y, w, 30, c, 0.08)
    shape(M, 'diamond', x + 16, y + 15, 7, c, 1.3)
    M.tx(x + 30, y + 15, txt, c, 13)


# ---------------------------------------------------------------- A 카드 테이블
def title_A(M):
    G, EM = hx('ffd96a'), hx('5fe0b0')
    # 바닥: 옅은 둥근 테이블 테두리
    M.ln(circle(W / 2, H + 120, 560, 90, 1.25), hx('1d4a3c'), 2.0)
    M.ln(circle(W / 2, H + 120, 540, 90, 1.25), hx('163a30'), 1.0)
    # 부채꼴 카드 다섯
    for k, sk in enumerate(['spade', 'heart', 'club', 'diamond', 'spade']):
        a = math.radians(-24 + k * 12)
        cx = W / 2 + math.sin(a) * 260
        cy = 150 - math.cos(a) * 40 + 20
        card(M, cx - 42, cy - 60, 84, 120, SCOL[sk] if k != 4 else G, 1.6, a * 0.9, None, sk)
    M.box(W / 2 - 330, 200, 660, 80, hx('06060b'), 0.9)
    M.tx(W / 2, 236, 'POKER DEFENSE', G, 56, 'mm')
    M.ln([(W / 2 - 260, 276), (W / 2 - 40, 276)], EM, 1.2); M.ln([(W / 2 + 40, 276), (W / 2 + 260, 276)], EM, 1.2)
    shape(M, 'diamond', W / 2, 276, 7, EM, 1.3)
    M.tx(W / 2, 302, '카드를 놓아 족보를 만들고, 끝없이 몰려오는 적을 막아라', FG, 14, 'mm')
    hud_chip(M, W - 190, 24, '칩 1,240', G)
    for i, n in enumerate(MENU):
        y = 336 + i * 50
        sel = i == 1
        x0 = W / 2 - 160
        c = G if i <= 1 else EM
        M.box(x0, y, 320, 42, hx('0b1512'), 0.9)
        if sel: M.box(x0, y, 320, 42, G, 0.12)
        M.rr(x0, y, 320, 42, 10, c, 2.2 if sel else 1.2)
        # 왼쪽: 작은 카드 귀퉁이 + 무늬
        M.rr(x0 + 10, y + 7, 22, 28, 4, c, 1.2)
        shape(M, SUITS[i % 4], x0 + 21, y + 21, 6, c, 1.2)
        M.tx(x0 + 46, y + 21, n, (255, 255, 255) if sel else c, 17)
        if sel:
            M.tx(x0 + 300, y + 21, 'Enter', DIM, 10, 'rm')
            for s in (-1, 1): M.ln([(W / 2 + s * 180, y + 21), (W / 2 + s * 196, y + 21)], G, 1.6)
    M.tx(W / 2, 680, '최고 기록 42웨이브', GRY, 12, 'mm')


def upgrade_A(M):
    G, EM = hx('ffd96a'), hx('5fe0b0')
    M.tx(60, 54, '영구 강화', G, 26)
    M.tx(60, 84, '판이 끝나면 칩을 받는다  ·  웨이브 + 2 × 오른 레벨 + 클리어 덤', GRY, 12)
    hud_chip(M, W - 220, 40, '칩 1,240', G)
    M.rr(W - 74, 40, 50, 30, 10, GRY, 1.2); M.tx(W - 49, 55, 'Esc', GRY, 11, 'mm')
    for i, (n, e, ic, lv) in enumerate(UPG):
        x = 60 + (i % 4) * 296
        y = 120 + (i // 4) * 280
        c = SCOL.get(ic, G if i < 2 else hx('ff9a6a'))
        M.box(x, y, 276, 260, hx('0b1512'), 0.92)
        M.rr(x, y, 276, 260, 14, c, 1.4)
        # 카드 귀퉁이 무늬
        shape(M, ic, x + 26, y + 30, 11, c, 1.5)
        M.tx(x + 48, y + 30, n, c, 17)
        # 레벨 고리
        cx, cy = x + 138, y + 116
        M.ln(circle(cx, cy, 44, 48), hx('1c2a26'), 5)
        if lv: M.ln(arc(cx, cy, 44, -90, -90 + 360 * lv / 20, 40), c, 5)
        M.tx(cx, cy - 6, 'Lv %d' % lv, (255, 255, 255), 20, 'mm')
        M.tx(cx, cy + 16, '/ 20', DIM, 11, 'mm')
        M.tx(x + 138, y + 182, e, FG, 12, 'mm')
        for k, (lab, cost) in enumerate((('+1', 5 + 5 * lv), ('+10', '1,050'), ('+100', '-'), ('최대', '-'))):
            bx = x + 14 + k * 63
            on = k < 2
            M.rr(bx, y + 208, 58, 38, 9, c if on else DIM, 1.3)
            if k == 0: M.box(bx, y + 208, 58, 38, c, 0.12)
            M.tx(bx + 29, y + 220, lab, c if on else DIM, 12, 'mm')
            M.tx(bx + 29, y + 236, str(cost), G if on else DIM, 9, 'mm')


# ---------------------------------------------------------------- B 이상한 나라
def title_B(M):
    P, C = hx('ff8fd0'), hx('7fe0ff')
    # 체셔 웃음 (큰 호) + 눈 두 개, 모자
    M.ln(arc(W / 2, 96, 80, 20, 160, 40), P, 2.0)
    M.ln(arc(W / 2, 96, 66, 30, 150, 36), P, 1.2)
    for k in range(9):
        a = math.radians(30 + k * 15)
        M.ln([(W / 2 + math.cos(a) * 66, 96 + math.sin(a) * 66), (W / 2 + math.cos(a) * 78, 96 + math.sin(a) * 78)], P, 1.2)
    for s in (-1, 1):
        M.ln(circle(W / 2 + s * 48, 82, 11, 24, 0.55), C, 1.6)
        M.ln([(W / 2 + s * 48, 73), (W / 2 + s * 48, 91)], C, 1.4)
    for p in jester_hat(W / 2 + 230, 120, 40): M.ln(p, C, 1.6)
    for p in joker(W / 2 - 230, 130, 34): M.ln(p, P, 1.6)
    M.tx(W / 2, 236, 'POKER DEFENSE', (255, 255, 255), 56, 'mm')
    M.tx(W / 2, 236, 'POKER DEFENSE', P, 56, 'mm')
    M.tx(W / 2, 286, '— 이상한 나라의 카드 병정을 막아라 —', C, 15, 'mm')
    hud_chip(M, W - 190, 24, '칩 1,240', C)
    for i, n in enumerate(MENU):
        y = 326 + i * 54
        sel = i == 1
        x0 = W / 2 - 170
        c = P if sel else (C if i else hx('b89cff'))
        M.rr(x0, y, 340, 44, 22, c, 2.2 if sel else 1.3)
        M.rr(x0 + 5, y + 5, 330, 34, 17, c, 0.8)
        if sel: M.box(x0 + 8, y + 8, 324, 28, P, 0.14)
        M.tx(W / 2, y + 22, n, (255, 255, 255) if sel else c, 17, 'mm')
        if sel:
            shape(M, 'heart', x0 - 22, y + 22, 8, P, 1.5); shape(M, 'heart', x0 + 362, y + 22, 8, P, 1.5)
    M.tx(W / 2, 680, '최고 기록 42웨이브', GRY, 12, 'mm')


def upgrade_B(M):
    P, C = hx('ff8fd0'), hx('7fe0ff')
    M.tx(60, 54, '영구 강화', P, 26)
    M.tx(60, 84, '묘약을 한 방울씩 더하듯, 칩으로 판 밖의 힘을 키운다', C, 12)
    hud_chip(M, W - 220, 40, '칩 1,240', C)
    M.rr(W - 74, 40, 50, 30, 15, GRY, 1.2); M.tx(W - 49, 55, 'Esc', GRY, 11, 'mm')
    for i, (n, e, ic, lv) in enumerate(UPG):
        x = 60 + (i % 4) * 296
        y = 120 + (i // 4) * 280
        c = SCOL.get(ic, P if i < 2 else C)
        M.rr(x, y, 276, 260, 26, c, 1.4); M.rr(x + 6, y + 6, 264, 248, 21, c, 0.6)
        # 묘약 병 (몸통 + 목 + 차오른 높이)
        bx, by = x + 138, y + 110
        M.ln([(bx - 12, by - 58), (bx - 12, by - 38), (bx - 40, by - 10), (bx - 40, by + 42), (bx + 40, by + 42), (bx + 40, by - 10), (bx + 12, by - 38), (bx + 12, by - 58)], c, 1.6)
        M.ln([(bx - 16, by - 62), (bx + 16, by - 62)], c, 2)
        lvl = by + 42 - 52 * lv / 20
        if lv: M.box(bx - 38, lvl, 76, by + 40 - lvl, c, 0.22); M.ln([(bx - 38, lvl), (bx + 38, lvl)], c, 1.2)
        shape(M, ic, bx, by + 12, 12, (255, 255, 255), 1.3)
        M.tx(x + 24, y + 30, n, c, 16)
        M.tx(x + 252, y + 30, 'Lv %d' % lv, (255, 255, 255), 14, 'rm')
        M.tx(x + 138, y + 182, e, FG, 12, 'mm')
        for k, lab in enumerate(('+1', '+10', '+100', '최대')):
            bx2 = x + 14 + k * 63
            on = k < 2
            M.rr(bx2, y + 208, 58, 36, 18, c if on else DIM, 1.3)
            M.tx(bx2 + 29, y + 226, lab, c if on else DIM, 12, 'mm')


# ---------------------------------------------------------------- C 금빛 미니멀
def title_C(M):
    G, WH = hx('ffd96a'), hx('f4f0e6')
    # 얇은 두 겹 큰 틀 + 귀퉁이 장식
    M.rr(40, 40, W - 80, H - 80, 4, hx('3a3222'), 1.0); M.rr(52, 52, W - 104, H - 104, 4, hx('2a2418'), 0.8)
    for (x, y, sx, sy) in ((40, 40, 1, 1), (W - 40, 40, -1, 1), (40, H - 40, 1, -1), (W - 40, H - 40, -1, -1)):
        M.ln([(x, y + sy * 40), (x, y), (x + sx * 40, y)], G, 1.6)
        shape(M, 'diamond', x + sx * 26, y + sy * 26, 5, G, 1.2)
    for k, sk in enumerate(SUITS):
        shape(M, sk, W / 2 - 96 + k * 64, 150, 14, G, 1.4)
    M.tx(W / 2, 228, 'POKER DEFENSE', WH, 54, 'mm')
    M.ln([(W / 2 - 300, 268), (W / 2 + 300, 268)], G, 1.0)
    M.tx(W / 2, 292, '카드를 놓아 족보를 만들고, 끝없이 몰려오는 적을 막아라', GRY, 14, 'mm')
    hud_chip(M, W - 220, 70, '칩 1,240', G)
    for i, n in enumerate(MENU):
        y = 336 + i * 48
        sel = i == 1
        x0 = W / 2 - 150
        if sel:
            M.box(x0, y, 300, 38, G, 0.1)
            M.ln([(x0, y), (x0, y + 38)], G, 3)
            M.ln([(x0, y + 38), (x0 + 300, y + 38)], G, 1.0)
        else:
            M.ln([(x0, y + 38), (x0 + 300, y + 38)], hx('2e2a20'), 1.0)
        M.tx(x0 + 22, y + 19, n, WH if sel else GRY, 17)
        M.tx(x0 + 290, y + 19, ['', '▶', '', '', '', ''][i] if sel else '', G, 12, 'rm')
    M.tx(W / 2, 664, '최고 기록 42웨이브', DIM, 12, 'mm')


def upgrade_C(M):
    G, WH = hx('ffd96a'), hx('f4f0e6')
    M.tx(60, 54, '영구 강화', WH, 26)
    M.ln([(60, 74), (360, 74)], G, 1.0)
    M.tx(60, 92, '판이 끝나면 칩을 받는다  ·  웨이브 + 2 × 오른 레벨 + 클리어 덤', GRY, 12)
    hud_chip(M, W - 290, 40, '칩 1,240', G)
    M.tx(W - 40, 55, 'Esc  닫기', GRY, 11, 'rm')
    for i, (n, e, ic, lv) in enumerate(UPG):
        x = 60 + (i % 4) * 296
        y = 128 + (i // 4) * 276
        c = SCOL.get(ic, G)
        M.ln([(x, y), (x + 276, y)], hx('3a3222'), 1.0); M.ln([(x, y + 256), (x + 276, y + 256)], hx('3a3222'), 1.0)
        M.ln([(x, y), (x, y + 256)], c, 2.4)
        shape(M, ic, x + 34, y + 36, 13, c, 1.5)
        M.tx(x + 60, y + 30, n, WH, 17)
        M.tx(x + 60, y + 50, e, GRY, 11)
        M.tx(x + 24, y + 112, '%d' % lv, WH, 44, 'lm')
        M.tx(x + 32 + 29 * len(str(lv)), y + 124, '/ 20', DIM, 13, 'lm')
        # 막대 20칸
        for k in range(20):
            bx = x + 24 + k * 12
            M.ln([(bx, y + 160), (bx, y + 176)], c if k < lv else hx('2a2418'), 3)
        for k, (lab, cost) in enumerate((('+1', 5 + 5 * lv), ('+10', '1,050'), ('+100', '-'), ('최대', '-'))):
            bx = x + 24 + k * 62
            on = k < 2
            M.ln([(bx, y + 230), (bx + 54, y + 230)], c if on else hx('2a2418'), 1.6)
            M.tx(bx + 27, y + 208, lab, WH if on else DIM, 13, 'mm')
            M.tx(bx + 27, y + 244, str(cost), G if on else DIM, 9, 'mm')


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    for name, fn in (('A_title', title_A), ('A_upgrade', upgrade_A), ('B_title', title_B), ('B_upgrade', upgrade_B), ('C_title', title_C), ('C_upgrade', upgrade_C)):
        M = _Mock(S=2)
        fn(M)
        img = M.draw()
        img.resize((W, H)).save(os.path.join(outdir, 'mock_%s.png' % name))
        print(name)


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
