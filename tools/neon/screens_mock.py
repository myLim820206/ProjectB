"""남은 옛 모양 화면 6개를 네온 바로 (D-288) — 메뉴(Esc) · 설정 · 모드 고르기 · 게임 오버 · 클리어 · 스킬 트리

    python tools/neon/screens_mock.py 출력폴더

1 메뉴: 'BREAK TIME' 걸린 간판 + 두 겹 튜브 버튼 다섯 (고른 것 분홍 + 하트), 뒤는 어둡게
2 설정: 주크박스 패널 — 위 반원 아치 + 줄마다 이름 ···· 튜브 알약 선택지, 소리는 믹서 페이더
3 모드 고르기: 바의 부스 넷 'BOOTH 30 · 60 · 100 · ∞' 간판, 잠긴 부스는 불 꺼진 튜브 + 자물쇠, 클리어한 부스엔 금별
4 게임 오버: 'LAST CALL' 간판이 꺼져 가고, 아래 바 영수증(점선 리더)에 웨이브 · 처치 · 레벨 · 받은 칩 합계
5 클리어: 'CHEERS!' 간판 + 잔 부딪치기 + 불꽃, 영수증에 클리어 덤 · 새로 열린 모드
6 스킬 트리: 벽돌 벽 위 코스터(둥근 받침) 노드를 네온 튜브로 이음, 찍은 노드는 켜짐, 키스톤은 금색 큰 코스터, 위에 걸린 간판
"""
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, PINK, CYAN, GOLD, WHITE  # noqa: E402
from logo_retro import KO, BLOCK, bulbs  # noqa: E402
from neon import circle, suit  # noqa: E402
from crt_mock import rr_pts  # noqa: E402

LAV = (168, 150, 225)
FG = (207, 216, 230)
DIM = (95, 104, 120)
RED = (255, 90, 100)
ORANGE = (255, 150, 70)
MINT = (156, 255, 176)
BR = (74, 60, 72)


def tube(M, x, y, w, h, r, c, b=0.85, wd=2.0):
    M.ln(rr_pts(x, y, w, h, r), c, wd, b)
    M.ln(rr_pts(x + 5, y + 5, w - 10, h - 10, max(2, r - 5)), c, 1.0, 0.7 * b)


def back(M, dim=True):
    """뒤에 인게임 화면을 아주 흐리게 (판 틀 · 패널 자리)"""
    for r in range(0, 18):
        y = 20 + r * 34
        if y > 590: break
        M.ln([(0, y), (1280, y)], BR, 1.0, 0.25)
    M.ln(rr_pts(256, 92, 780, 492, 10), LAV, 1.2, 0.18)
    M.ln(rr_pts(16, 66, 232, 638, 12), LAV, 1.2, 0.15)
    M.ln(rr_pts(1044, 98, 220, 488, 12), LAV, 1.2, 0.15)


def wires(M, x0, x1, y):
    for x in (x0, x1): M.ln([(x, 0), (x, y)], DIM, 1.0, 0.5)


def hanging_sign(M, cx, y, w, h, text, col, px=34, track=4, sub=None, bulb=True):
    x = cx - w / 2
    wires(M, x + 30, x + w - 30, y)
    tube(M, x, y, w, h, 14, col, 1.0, 2.4)
    if bulb: bulbs(M, [(x + 14, y + 12), (x + w - 14, y + 12), (x + w - 14, y + h - 12), (x + 14, y + h - 12), (x + 14, y + 12)], 16, 0)
    M.word(cx, y + h / 2 + px * 0.36, text, col, px, None, track=track, font=BLOCK)
    if sub: M.word(cx, y + h + 26, sub, FG, 13, None, font=KO)


def heart(M, x, y, r=7, c=PINK):
    for p in suit('heart')(x, y, r): M.ln(p, c, 1.4)


def btn(M, x, y, w, h, label, on=False, c=CYAN, key=None):
    col = PINK if on else c
    tube(M, x, y, w, h, h / 2, col, 1.0 if on else 0.8, 2.4 if on else 2.0)
    M.word(x + w / 2, y + h / 2 + 6, label, WHITE if on else (216, 244, 255), 17, None, font=KO)
    if key: M.word(x + w - 22, y + h / 2 + 4, key, DIM, 9, None, font=BLOCK)
    if on: heart(M, x - 22, y + h / 2)


# ---------------------------------------------------------------- 1 메뉴
def m_menu():
    M = Sign(0.85); M.small_glow = 0.0
    back(M)
    hanging_sign(M, 640, 150, 360, 80, 'BREAK TIME', ORANGE, 30, sub='잠깐 쉬어 가자 · 판은 그대로 멈춰 있다')
    for i, n in enumerate(['계속하기', '새 게임', '메인 메뉴로', '설정', '게임 종료']):
        btn(M, 490, 290 + i * 58, 300, 44, n, on=i == 0, c=RED if i == 4 else CYAN)
    M.word(640, 600, '↑↓ 고르기 · Enter 결정 · Esc 닫기', DIM, 11, None, font=KO)
    return M.draw()


# ---------------------------------------------------------------- 2 설정 (주크박스)
def m_settings():
    M = Sign(0.85); M.small_glow = 0.0
    back(M)
    x0, y0, w, h = 330, 70, 620, 600
    # 주크박스 몸통: 위 반원 아치 + 두 겹
    arch = [(x0, y0 + 110)] + [(640 + math.cos(math.radians(a)) * 310, y0 + 110 - math.sin(math.radians(a)) * 100) for a in range(180, -1, -6)] + [(x0 + w, y0 + 110), (x0 + w, y0 + h), (x0, y0 + h), (x0, y0 + 110)]
    M.ln(arch, PINK, 2.4, 0.9)
    arch2 = [(x0 + 8, y0 + 112)] + [(640 + math.cos(math.radians(a)) * 300, y0 + 112 - math.sin(math.radians(a)) * 92) for a in range(180, -1, -6)] + [(x0 + w - 8, y0 + 112), (x0 + w - 8, y0 + h - 8), (x0 + 8, y0 + h - 8), (x0 + 8, y0 + 112)]
    M.ln(arch2, PINK, 1.0, 0.5)
    bulbs(M, [(640 + math.cos(math.radians(a)) * 280, y0 + 110 - math.sin(math.radians(a)) * 80) for a in range(170, 9, -8)], 18, 0)
    M.word(640, y0 + 92, 'JUKEBOX', GOLD, 26, None, track=6, font=BLOCK)
    M.word(640, y0 + 120, '설정', FG, 13, None, font=KO)
    rows = [('언어', ['한국어', 'EN', '日本語', '简', '繁'], 0), ('창 모드', ['창', '테두리 없음', '전체'], 1), ('카드 색', ['네온', '흑백', '빨강·하양'], 0),
            ('프레임', ['60', '120', '무제한'], 0), ('수직 동기화', ['켬', '끔'], 0), ('월드 이펙트', ['켬', '끔'], 0), ('화면 흔들림', ['켬', '끔'], 0)]
    y = y0 + 150
    for name, opts, sel in rows:
        M.word(x0 + 40, y + 6, name, FG, 13, None, font=KO, anchor_center=False)
        for dx in range(int(x0 + 40 + len(name) * 14 + 8), int(x0 + 250), 6): M.ln([(dx, y + 2), (dx + 1.2, y + 2)], DIM, 1.0, 0.5)
        ox = x0 + 260
        for k, o in enumerate(opts):
            ow = 30 + len(o) * 11
            on = k == sel
            M.ln(rr_pts(ox, y - 12, ow, 24, 12), PINK if on else LAV, 1.6 if on else 1.1, 1.0 if on else 0.55)
            M.word(ox + ow / 2, y + 5, o, WHITE if on else FG, 11, None, font=KO)
            ox += ow + 8
        y += 38
    # 소리: 믹서 페이더 셋
    M.word(x0 + 40, y + 10, '소리', FG, 13, None, font=KO, anchor_center=False)
    for k, (n, v) in enumerate((('효과음', 0.8), ('음악', 0.6), ('전체', 0.9))):
        fx = x0 + 280 + k * 100
        M.ln([(fx, y - 10), (fx, y + 70)], LAV, 1.2, 0.5)
        ky = y + 70 - v * 80
        M.ln(rr_pts(fx - 14, ky - 6, 28, 12, 4), GOLD, 1.6, 1.0)
        M.word(fx, y + 92, n, FG, 10, None, font=KO)
        M.word(fx, y - 18, f'{int(v * 100)}', GOLD, 10, None, font=BLOCK)
    btn(M, 540, y0 + h - 64, 200, 40, '닫기', on=False, c=LAV)
    return M.draw()


# ---------------------------------------------------------------- 3 모드 고르기 (부스)
def m_mode():
    M = Sign(0.85); M.small_glow = 0.0
    back(M)
    hanging_sign(M, 640, 70, 380, 70, 'PICK A BOOTH', GOLD, 26, sub='목표 웨이브를 버티면 클리어 · 30 을 깨면 60 과 무한, 60 을 깨면 100')
    booths = [('30', '웨이브 버티기', '처음은 여기서', True, True, CYAN), ('60', '웨이브 버티기', '더 센 최종 보스', True, False, GOLD), ('100', '웨이브 버티기', '30 · 60 을 깨면 열림', False, False, GOLD), ('∞', '끝없이', '어디까지 버티나', True, False, PINK)]
    for i, (big, sub, hint, open_, cleared, c) in enumerate(booths):
        x = 640 - 2 * 190 + i * 190 + 10
        y = 240
        on = i == 0
        col = PINK if on else (c if open_ else DIM)
        b = 1.0 if on else (0.8 if open_ else 0.35)
        # 부스: 위 차양(지그재그) + 문틀
        M.ln([(x, y + 30)] + [(x + k * 17, y + 30 + (8 if k % 2 else 0)) for k in range(11)], col, 1.4, b)
        tube(M, x, y, 170, 250, 14, col, b, 2.2 if on else 1.8)
        M.word(x + 85, y + 22, 'BOOTH', col, 11, [b] * 5, track=4, font=BLOCK)
        M.word(x + 85, y + 120, big, col if open_ else DIM, 54, [b] * len(big), font=BLOCK if big.isascii() else KO)
        M.word(x + 85, y + 158, sub, FG if open_ else DIM, 12, None, font=KO)
        M.word(x + 85, y + 200, hint, (FG if open_ else RED) if True else FG, 10, [0.8 if open_ else 0.7] * len(hint), font=KO)
        if not open_:  # 자물쇠
            M.ln(rr_pts(x + 73, y + 58, 24, 18, 3), DIM, 1.4, 0.8)
            M.ln([(x + 78, y + 58), (x + 78, y + 50), (x + 85, y + 44), (x + 92, y + 50), (x + 92, y + 58)], DIM, 1.4, 0.8)
        if cleared: M.word(x + 148, y + 54, '★', GOLD, 18, None, font=KO)
        if on: heart(M, x + 85, y - 16)
    M.word(640, 560, '←→ 고르기 · Enter 시작 · Esc 닫기', DIM, 11, None, font=KO)
    return M.draw()


# ---------------------------------------------------------------- 영수증
def receipt(M, cx, y, lines, total, col=FG):
    w, lh = 300, 24
    h = 70 + len(lines) * lh + 60
    x = cx - w / 2
    # 톱니 모양 아래 가장자리
    pts = [(x, y), (x + w, y), (x + w, y + h)] + [(x + w - k * 12 - (6 if k % 2 else 0), y + h - (6 if k % 2 else 0)) for k in range(26)] + [(x, y + h), (x, y)]
    M.ln(pts, LAV, 1.6, 0.8)
    M.word(cx, y + 30, 'NEON PALLAS · BAR', FG, 12, None, track=3, font=BLOCK)
    M.ln([(x + 16, y + 44), (x + w - 16, y + 44)], DIM, 1.0, 0.6)
    yy = y + 70
    for n, v in lines:
        M.word(x + 20, yy, n, FG, 12, None, font=KO, anchor_center=False)
        vf = BLOCK if v.isascii() else KO  # 한글 · 기호는 맑은 고딕
        M.word(x + w - 20 - len(v) * (7 if v.isascii() else 10), yy, v, WHITE, 12, None, font=vf, anchor_center=False)
        for dx in range(int(x + 26 + len(n) * 12), int(x + w - 30 - len(v) * 7), 6): M.ln([(dx, yy - 4), (dx + 1.2, yy - 4)], DIM, 1.0, 0.5)
        yy += lh
    M.ln([(x + 16, yy - 8), (x + w - 16, yy - 8)], DIM, 1.0, 0.6)
    M.word(x + 20, yy + 18, '받은 칩', GOLD, 14, None, font=KO, anchor_center=False)
    M.word(x + w - 20 - len(total) * 9, yy + 18, total, GOLD, 16, None, font=BLOCK, anchor_center=False)
    return y + h


# ---------------------------------------------------------------- 4 게임 오버
def m_over():
    M = Sign(0.85); M.small_glow = 0.0
    back(M)
    x, y, w, h = 640 - 220, 60, 440, 90
    wires(M, x + 40, x + w - 40, y)
    tube(M, x, y, w, h, 14, RED, 0.75, 2.2)
    M.word(640, y + 62, 'LAST CALL', RED, 46, [1, 1, 0.15, 1, 0.5, 1, 1, 0.1, 1], track=6, font=BLOCK)  # 꺼져 가는 글자
    M.word(640, y + h + 26, '웨이브 23에서 무너졌다', FG, 15, None, font=KO)
    bot = receipt(M, 640, 200, [('웨이브', '23'), ('처치', '1,284'), ('레벨', '41'), ('최고 기록', '23 (갱신!)'), ('개인 랭킹', '3위')], '+ 186')
    btn(M, 640 - 250, bot + 30, 230, 44, '다시 한 판 (R)', on=True)
    btn(M, 640 + 20, bot + 30, 230, 44, '메인 메뉴로', c=LAV)
    return M.draw()


# ---------------------------------------------------------------- 5 클리어
def m_clear():
    M = Sign(0.85); M.small_glow = 0.0
    back(M)
    hanging_sign(M, 640, 50, 400, 90, 'CHEERS!', GOLD, 46, track=6)
    # 잔 부딪치기 + 불꽃
    for s in (-1, 1):
        cx = 640 + s * 40
        a = math.radians(-14 * s)
        def R(px, py): return (cx + px * math.cos(a) - py * math.sin(a), 230 + px * math.sin(a) + py * math.cos(a))
        M.ln([R(-22, -40), R(22, -40), R(0, -6), R(-22, -40)], PINK if s < 0 else CYAN, 1.8)
        M.ln([R(0, -6), R(0, 22)], PINK if s < 0 else CYAN, 1.6)
        M.ln([R(-12, 24), R(12, 24)], PINK if s < 0 else CYAN, 1.6)
    rnd = random.Random(3)
    for _ in range(14):
        a = rnd.uniform(-math.pi, 0); r1, r2 = rnd.uniform(8, 14), rnd.uniform(24, 46)
        M.ln([(640 + math.cos(a) * r1, 188 + math.sin(a) * r1), (640 + math.cos(a) * r2, 188 + math.sin(a) * r2)], GOLD, 1.3, 0.9)
    M.word(640, 290, '30 웨이브 모드 클리어 · 팔라스의 축배', FG, 15, None, font=KO)
    bot = receipt(M, 640, 310, [('처치', '2,940'), ('레벨', '58'), ('클리어 덤', '+ 60'), ('개인 랭킹', '1위'), ('새로 열림', '60 · 무한')], '+ 312')
    btn(M, 640 - 115, bot + 24, 230, 44, '시작 화면으로', on=True)
    return M.draw()


# ---------------------------------------------------------------- 6 스킬 트리
def m_tree():
    M = Sign(0.85); M.small_glow = 0.0
    for r in range(0, 22):
        y = 20 + r * 34
        M.ln([(0, y), (1280, y)], BR, 1.0, 0.55)
        for k in range(21):
            xx = k * 64 + (32 if r % 2 else 0)
            M.ln([(xx, y), (xx, y + 34)], BR, 1.0, 0.55)
    # 걸린 간판
    wires(M, 470, 810, 14)
    tube(M, 440, 14, 400, 60, 12, (180, 140, 255), 1.0, 2.2)
    M.word(640, 54, 'SKILL TREE', (180, 140, 255), 28, None, track=5, font=BLOCK)
    M.word(940, 40, '레벨 12 · 남은 포인트 3', GOLD, 12, None, font=KO)
    M.word(940, 58, '키스톤 1 / 3 · 찍은 노드 9', FG, 11, None, font=KO)
    # 노드: 가운데에서 바깥으로 (코스터)
    rnd = random.Random(8)
    cx, cy = 640, 400
    nodes = [(cx, cy, 'start')]
    for ring, (r, n) in enumerate(((90, 6), (170, 12), (250, 18))):
        for k in range(n):
            a = math.tau * k / n + ring * 0.2
            nodes.append((cx + math.cos(a) * r * 1.5, cy + math.sin(a) * r, 'key' if ring == 2 and k % 6 == 0 else 'n'))
    owned = {0, 1, 2, 7, 8, 9, 19, 20, 21}
    for i, (x, y, kind) in enumerate(nodes):  # 이음 선 (안쪽 고리 → 바깥)
        if i == 0: continue
        best = min(range(len(nodes)), key=lambda j: (nodes[j][0] - x) ** 2 + (nodes[j][1] - y) ** 2 if j != i and math.hypot(nodes[j][0] - cx, nodes[j][1] - cy) < math.hypot(x - cx, y - cy) - 20 else 1e9)
        on = i in owned and best in owned
        M.ln([(x, y), (nodes[best][0], nodes[best][1])], (180, 140, 255) if on else LAV, 2.0 if on else 1.0, 0.9 if on else 0.3)
    for i, (x, y, kind) in enumerate(nodes):
        on = i in owned
        if kind == 'key':
            M.ln(circle(x, y, 22, 32), GOLD, 2.2, 1.0 if on else 0.5); M.ln(circle(x, y, 16, 32), GOLD, 1.0, 0.6 if on else 0.3)
            M.word(x, y + 6, '★', GOLD, 14, None, font=KO)
        elif kind == 'start':
            M.ln(circle(x, y, 26, 32), PINK, 2.2); M.word(x, y + 5, '시작', WHITE, 11, None, font=KO)
        else:
            c = (180, 140, 255)
            M.ln(circle(x, y, 13, 24), c, 1.8 if on else 1.1, 1.0 if on else 0.4)
            M.ln(circle(x, y, 8, 20), c, 1.0, 0.6 if on else 0.2)  # 코스터 안쪽 원
            if on: M.dots.append((x, y, 1.4))
    # 마우스 올린 노드 툴팁
    tx, ty = nodes[10][0] + 24, nodes[10][1] - 70
    M.ln(rr_pts(tx, ty, 240, 64, 10), GOLD, 1.6, 0.9)
    M.word(tx + 14, ty + 24, '연쇄 강화 II', WHITE, 14, None, font=KO, anchor_center=False)
    M.word(tx + 14, ty + 46, '연쇄 보너스 +10% · 포인트 1', FG, 11, None, font=KO, anchor_center=False)
    heart(M, nodes[10][0], nodes[10][1] - 24)
    btn(M, 1110, 650, 140, 40, '닫기 K', c=LAV)
    return M.draw()


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    for name, fn in (('s1_menu', m_menu), ('s2_settings', m_settings), ('s3_mode', m_mode), ('s4_gameover', m_over), ('s5_clear', m_clear), ('s6_tree', m_tree)):
        fn().save(os.path.join(outdir, name + '.png'))
        print(name)


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
