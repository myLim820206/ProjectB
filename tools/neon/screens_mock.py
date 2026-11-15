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


# ---------------------------------------------------------------- 6 스킬 트리 (실제 트리: Source/ProjectB/PDTreeData.inl 그대로)
KEY_NAMES = ['외골수', '잡식가', '큰 판', '작은 판', '대장장이', '광대의 길', '고독한 저격수', '군중', '왕관의 무게', '서커스']
THEME_C = {0: (232, 238, 255), 1: (255, 106, 138), 2: (255, 217, 106), 3: (143, 233, 255), 4: (127, 214, 255), 6: (255, 154, 213)}


def load_tree():
    import re
    src = open(os.path.join(os.path.dirname(__file__), '..', '..', 'Source', 'ProjectB', 'PDTreeData.inl'), encoding='utf-8-sig').read()
    nodes = []
    for m in re.finditer(r'\{EPDNode::(\w+), (\d+), ([\d.]+)f, ([\d.]+)f, \{\{EPDStat::(\w+), (\d+),', src):
        nodes.append((m.group(1), int(m.group(2)), float(m.group(3)), float(m.group(4)), m.group(5), int(m.group(6))))
    li = src.index('GPDTreeLinks')
    links = [(int(x), int(y)) for x, y in re.findall(r'\{(\d+), (\d+)\}', src[li:])]
    return nodes, links


def m_tree():
    M = Sign(0.85); M.small_glow = 0.0
    for r in range(0, 22):
        y = 20 + r * 34
        M.ln([(0, y), (1280, y)], BR, 1.0, 0.5)
        for k in range(21):
            xx = k * 64 + (32 if r % 2 else 0)
            M.ln([(xx, y), (xx, y + 34)], BR, 1.0, 0.5)
    nodes, links = load_tree()
    # 찍은 노드 예: 시작 → ♠ 줄 → 외골수 쪽 키스톤 + ♥ 줄 조금
    owned = {0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 18, 19, 20, 21}
    sealed = set()
    for i, n in enumerate(nodes):  # 찍은 키스톤의 짝은 막힘
        if n[0] == 'Keystone' and i in owned:
            pair = n[5] ^ 1
            for j, m in enumerate(nodes):
                if m[0] == 'Keystone' and m[5] == pair: sealed.add(j)
    def col(i):
        n = nodes[i]
        if n[0] == 'Keystone': return GOLD
        return THEME_C.get(n[1], (232, 240, 255))
    nxt = {b for a, b in links if a in owned and b not in owned} | {a for a, b in links if b in owned and a not in owned}
    for a, b in links:
        on = a in owned and b in owned
        c = col(b if a == 0 else a)
        M.ln([(nodes[a][2], nodes[a][3]), (nodes[b][2], nodes[b][3])], c, 2.2 if on else 1.0, 0.95 if on else 0.28)
    labels = []
    for i, (kind, th, x, y, stat, prm) in enumerate(nodes):
        on, nx, se = i in owned, i in nxt, i in sealed
        c = RED if se else col(i)
        b = 1.0 if on else 0.75 if nx else 0.35
        if kind == 'Start':
            M.ln(circle(x, y, 15, 32), PINK, 2.2); M.word(x, y + 5, '★', PINK, 13, None, font=KO)
        elif kind == 'Small':  # 코스터: 바깥 원 + 안쪽 원
            M.ln(circle(x, y, 10, 24), c, 1.6 if on else 1.1, b); M.ln(circle(x, y, 6, 18), c, 0.9, b * 0.6)
            if on: M.dots.append((x, y, 1.2))
        elif kind == 'Notable':  # 네모 코스터
            M.ln(rr_pts(x - 14, y - 14, 28, 28, 6), c, 1.8 if on else 1.2, b); M.ln(rr_pts(x - 9, y - 9, 18, 18, 4), c, 1.0, b * 0.6)
        elif kind == 'Keystone':  # 금색 팔각 큰 코스터 + 이름
            oct_ = [(x + math.cos(math.radians(22.5 + 45 * k)) * 22, y + math.sin(math.radians(22.5 + 45 * k)) * 22) for k in range(9)]
            M.ln(oct_, c, 2.4 if on else 1.6, 1.0 if on else 0.7)
            M.ln(circle(x, y, 13, 24), c, 1.0, 0.6 if on else 0.35)
            M.word(x, y + 6, '★', c, 13, [1.0 if on else 0.6], font=KO)
            name = KEY_NAMES[prm]
            # 이름은 트리 가운데에서 먼 바깥쪽에 (선 · 노드와 안 겹치게, D-290)
            dx, dy = x - 640, y - 392
            w_ = len(name) * 12 + 14
            if abs(dy) > abs(dx) * 0.6: lx, ly = x, y + (36 if dy > 0 else -30)  # 위아래 쪽 키스톤: 위 / 아래
            else: lx, ly = x + (1 if dx > 0 else -1) * (28 + w_ / 2), y + 5     # 옆쪽 키스톤: 바깥 옆
            labels.append((lx, ly, name, c, on))
            if se:
                M.ln([(x - 16, y - 16), (x + 16, y + 16)], RED, 1.8); M.ln([(x + 16, y - 16), (x - 16, y + 16)], RED, 1.8)
        elif kind == 'Joker':  # 조커 칸: 둥근 상자 + 광대 모자
            M.ln(rr_pts(x - 16, y - 16, 32, 32, 6), (255, 154, 213), 1.6, b)
            M.ln([(x - 10, y + 8), (x - 9, y - 4), (x - 3, y + 2), (x, y - 9), (x + 3, y + 2), (x + 9, y - 4), (x + 10, y + 8), (x - 10, y + 8)], (255, 154, 213), 1.2, b)
    img_labels = labels
    # 걸린 간판 + 정보 알약
    wires(M, 470, 810, 8)
    tube(M, 440, 8, 400, 54, 12, (180, 140, 255), 1.0, 2.2)
    M.word(640, 45, 'SKILL TREE', (180, 140, 255), 26, None, track=5, font=BLOCK)
    for k, (t, c) in enumerate((('레벨 16', GOLD), ('남은 포인트 2', MINT), ('키스톤 1 / 3', GOLD), ('찍은 노드 14', FG))):
        px = 40 + k * 120
        M.ln(rr_pts(px, 22, 110, 26, 13), c, 1.3, 0.8)
        M.word(px + 55, 40, t, c, 11, None, font=KO)
    M.word(1240, 40, '이번 판에서만 · 레벨이 오를 때마다 포인트 1', DIM, 10, None, font=KO, anchor_center=False) if False else M.word(1060, 40, '이번 판에서만 · 레벨마다 포인트 1', DIM, 10, None, font=KO)
    # 툴팁: 키스톤 위에 마우스
    kx, ky = nodes[24][2], nodes[24][3]
    heart(M, kx, ky - 34)
    tx, ty = kx - 300, ky + 50
    M.ln(rr_pts(tx, ty, 290, 92, 10), GOLD, 1.8, 0.9)
    M.word(tx + 14, ty + 24, '키스톤 · ' + KEY_NAMES[nodes[24][5]], GOLD, 14, None, font=KO, anchor_center=False)
    M.word(tx + 14, ty + 46, '짝 키스톤과 둘 중 하나만 찍을 수 있다', FG, 11, None, font=KO, anchor_center=False)
    M.word(tx + 14, ty + 66, '포인트 1 · 이어진 노드를 먼저 찍어야', DIM, 10, None, font=KO, anchor_center=False)
    btn(M, 1120, 660, 140, 40, '닫기 K', c=LAV)
    img = M.draw()
    # 키스톤 이름: 어두운 판 위에 (뒤 선이 비치지 않게)
    from PIL import ImageDraw
    d = ImageDraw.Draw(img, 'RGBA')
    T = Sign(0.85); T.small_glow = 0.0
    for (lx, ly, name, c, on) in img_labels:
        w = len(name) * 12 + 14
        d.rounded_rectangle([lx - w / 2, ly - 14, lx + w / 2, ly + 5], radius=6, fill=(8, 8, 14, 235))
        T.ln(rr_pts(lx - w / 2, ly - 14, w, 19, 6), c, 1.0, 0.6 if on else 0.35)
        T.word(lx, ly, name, c, 11, [1.0 if on else 0.8] * len(name), font=KO)
    from PIL import ImageChops, Image
    from logo_mock import BG
    return ImageChops.add(img, ImageChops.subtract(T.draw(), Image.new('RGB', img.size, BG)))


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    for name, fn in (('s1_menu', m_menu), ('s2_settings', m_settings), ('s3_mode', m_mode), ('s4_gameover', m_over), ('s5_clear', m_clear), ('s6_tree', m_tree)):
        fn().save(os.path.join(outdir, name + '.png'))
        print(name)


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
