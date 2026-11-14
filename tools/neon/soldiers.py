"""카드병정 적 시안 (네온 화풍, docs/15-art-style.md). 적을 트럼프 카드 몸통의 병정으로.

    python tools/neon/soldiers.py sheet 출력.png   # 종류 8 × 걷기 4프레임
    python tools/neon/soldiers.py gif 출력.gif     # 길을 따라 걷는 움직이는 시안
    python tools/neon/soldiers.py bosses 출력.png  # 보스 13종 × 걷기 4프레임 (D-176)

몸통 = 둥근 카드(가운데 무늬, 왼쪽 위 숫자), 머리 = 동그라미 + 모자, 팔다리 = 선, 무기는 종류마다.
걷기: 다리가 번갈아 앞뒤로(±26도), 팔은 반대로, 몸이 한 걸음마다 살짝 튄다.
"""
import math
import sys

import neon
from PIL import Image

HX = neon._Mock.hx
KINDS = [
    # 이름, 무늬, 색, 몸통 너비·높이 배율, 걸음 빠르기, 무기
    ('보통', 'spade', 'e8f0ff', 1.0, 1.0, 1.0, 'spear'),
    ('빠름', 'diamond', 'ffa86a', 0.85, 0.95, 1.8, 'none'),
    ('탱크', 'club', 'c49aff', 1.2, 1.15, 0.7, 'axe'),
    ('방패병', 'heart', '9fc3e8', 1.0, 1.0, 0.9, 'shield'),
    ('분열체', 'diamond', 'ff9ad5', 1.0, 1.0, 1.1, 'split'),
    ('재생자', 'club', 'b6ff6a', 1.0, 1.0, 0.9, 'regen'),
    ('조각', 'heart', 'ff9ad5', 0.6, 0.6, 1.6, 'none'),
    ('보스: 왕', 'spade', 'ffd96a', 1.35, 1.35, 0.6, 'king'),
]
# 보스 13종 (D-176): 몸통은 큰 카드병정(1.35배), 머리 장식·무기·얼굴로 구분. 색은 게임의 EnemyColor
BOSSES = [
    ('방패 거인', 'heart', '9fc3e8', 1.35, 1.35, 0.6, 'b_shield'),
    ('분열 군주', 'diamond', 'ff9ad5', 1.35, 1.35, 0.6, 'b_split'),
    ('재생 왕', 'club', 'b6ff6a', 1.35, 1.35, 0.6, 'b_regen'),
    ('돌진 기사', 'diamond', 'ffa86a', 1.35, 1.35, 0.9, 'b_charge'),
    ('소환사', 'club', 'c49aff', 1.35, 1.35, 0.6, 'b_summon'),
    ('유령', 'spade', 'cfe4ff', 1.35, 1.35, 0.6, 'b_ghost'),
    ('군단장', 'spade', 'ffd96a', 1.35, 1.35, 0.6, 'b_warlord'),
    ('분노 거인', 'heart', 'ff6a6a', 1.35, 1.35, 0.7, 'b_rage'),
    ('공간 도약자', 'diamond', '7fd6ff', 1.35, 1.35, 0.6, 'b_blink'),
    ('흡수자', 'club', 'e07aff', 1.35, 1.35, 0.6, 'b_devour'),
    ('수호자', 'heart', 'a8f0ff', 1.35, 1.35, 0.6, 'b_warden'),
    ('저주술사', 'spade', '9a7aff', 1.35, 1.35, 0.6, 'b_hex'),
    ('파괴자', 'club', 'ffcf5a', 1.35, 1.35, 0.6, 'b_break'),
]


def rot(p, c, a):
    s, co = math.sin(a), math.cos(a)
    x, y = p[0] - c[0], p[1] - c[1]
    return (c[0] + x * co - y * s, c[1] + x * s + y * co)


def soldier(kind, cx, cy, R, t, flop=False):
    """선 목록 [(점들, 색, 굵기)]. cy = 발바닥. t = 걷기 위상 0..1. flop = 카드가 흐느적 휘며 걷기 (D-182 시안)"""
    name, st, col, wk, hk, spd, wpn = kind
    c = HX(col)
    L = []
    ph = t * 2 * math.pi
    sw = math.sin(ph)
    bob = abs(math.sin(ph)) * 0.05 * R
    lean = math.radians(8) if name == '빠름' else math.radians(12) if wpn == 'b_charge' else 0.0
    ghost = wpn == 'b_ghost'
    boss = wpn.startswith('b_')
    bw, bh = 0.76 * R * wk, 1.18 * R * hk               # 트럼프처럼 세로로 긴 직사각형 (약 1 : 1.55)
    legL = 0.32 * R * hk                                # 짧은 다리
    hip = (cx, cy - legL - bob)                         # 다리 시작
    # 다리 (무릎 없이 곧게, 번갈아)
    for s in (1, -1) if not ghost else ():  # 유령은 다리 없이 떠다닌다
        a = math.radians(26) * sw * s
        foot = (hip[0] + math.sin(a) * legL, hip[1] + math.cos(a) * legL)
        L.append(([(hip[0] - s * 0.12 * bw, hip[1]), foot, (foot[0] + 0.12 * R, foot[1])], c, 1.6))
    if ghost:  # 카드 아래 너울거리는 꼬리
        wv = [(cx - bw / 2 + bw * k / 8, hip[1] + (0.08 * R if k % 2 else 0.0) + 0.04 * R * math.sin(ph + k)) for k in range(9)]
        L.append((wv, c, 1.3))
    # 몸통 카드
    top = hip[1] - bh
    body_c = (cx, top + bh / 2)
    # 흐느적 (D-182 시안): 카드 윗부분이 걸음보다 조금 늦게 좌우로 휘청 (위로 갈수록 크게), 몸 전체도 살짝 흔들린다
    bend = (0.26 * R * math.sin(ph - 0.9)) if flop else 0.0
    wob = (math.radians(5) * math.sin(ph - 0.4)) if flop else 0.0

    def W(q):
        v = min(1.0, max(0.0, (q[1] - top) / bh)) if bh > 0 else 0.0
        return rot((q[0] + bend * (1 - v) ** 2, q[1]), hip, lean + wob)
    P = []
    r = 0.12 * R
    for (ox, oy, a0) in ((bw / 2 - r, -bh / 2 + r, -90), (bw / 2 - r, bh / 2 - r, 0), (-bw / 2 + r, bh / 2 - r, 90), (-bw / 2 + r, -bh / 2 + r, 180)):
        for k in range(7):
            a = math.radians(a0 + k * 15)
            P.append((body_c[0] + ox + math.cos(a) * r, body_c[1] + oy + math.sin(a) * r))
    P.append(P[0])
    if flop:  # 곧은 변을 잘게 나눠야 휘어 보인다
        Q = [P[0]]
        for a, b in zip(P, P[1:]):
            n = max(1, int(math.hypot(b[0] - a[0], b[1] - a[1]) / (0.08 * R)))
            Q += [(a[0] + (b[0] - a[0]) * k / n, a[1] + (b[1] - a[1]) * k / n) for k in range(1, n + 1)]
        P = Q
    P = [W(p) for p in P]
    L.append((P, c, 2.4 if name == '탱크' else 1.7))
    if wpn in ('split', 'b_split'):  # 가운데 찢어진 금
        zz = [(body_c[0] + (0.08 if k % 2 else -0.08) * bw, top + bh * k / 6) for k in range(7)]
        L.append(([W(p) for p in zz], c, 1.2))
    # 무늬는 카드 귀퉁이에 작게 (왼쪽 위, 오른쪽 아래), 가운데는 얼굴 (머리 없이 카드가 곧 몸이자 얼굴)
    pr = 0.085 * R * min(wk, hk)
    for (ox, oy) in ((-bw / 2 + 0.12 * R * wk, -bh / 2 + 0.14 * R * hk), (bw / 2 - 0.12 * R * wk, bh / 2 - 0.14 * R * hk)): # 귀퉁이 끝으로 (얼굴과 안 겹치게)
        for s in neon.suit(st)(body_c[0] + ox, body_c[1] + oy, pr):
            L.append(([W(p) for p in s], c, 1.1))
    fc = (body_c[0], body_c[1] - 0.04 * R)
    ex, ey, er = 0.16 * R * min(wk, 1.1), 0.1 * R, 0.09 * R # 게임 크기에서도 보이게 눈을 크게
    blink = (t * spd * 1.3) % 1.0 > 0.9  # 가끔 눈 깜빡
    if wpn == 'b_hex':  # 외눈
        L.append(([W(p) for p in neon.circle(fc[0], fc[1] - ey, er * 1.6, 16)], c, 1.5))
        L.append(([W(p) for p in neon.circle(fc[0], fc[1] - ey, er * 0.6, 10)], c, 1.3))
    for s in (-1, 1) if wpn != 'b_hex' else ():
        e_c = (fc[0] + s * ex, fc[1] - ey)
        if ghost:  # 텅 빈 눈
            L.append(([W(p) for p in neon.circle(e_c[0], e_c[1] + 0.02 * R, er * 1.2, 12)], c, 1.4))
            continue
        if blink:
            L.append(([W((e_c[0] - er, e_c[1])), W((e_c[0] + er, e_c[1]))], c, 1.4))
        else:
            L.append(([W(p) for p in neon.circle(e_c[0], e_c[1], er, 12)], c, 1.4))
            pup = (e_c[0] + 0.02 * R, e_c[1] + 0.01 * R)
            L.append(([W(p) for p in neon.circle(pup[0], pup[1], er * 0.35, 8)], c, 1.2))
        if name in ('탱크', '보스: 왕') or (boss and wpn not in ('b_regen',)):  # 짙은 눈썹
            L.append(([W((e_c[0] - s * 0.09 * R, e_c[1] - 0.12 * R)), W((e_c[0] + s * 0.07 * R, e_c[1] - 0.08 * R))], c, 1.4))
    nose = [(fc[0], fc[1] - 0.02 * R), (fc[0] - 0.04 * R, fc[1] + 0.09 * R), (fc[0] + 0.02 * R, fc[1] + 0.09 * R)]
    L.append(([W(p) for p in nose], c, 1.2))
    mw = 0.2 * R
    if wpn == 'b_devour':   # 크게 벌린 둥근 입
        mouth = neon.circle(fc[0], fc[1] + 0.22 * R, 0.13 * R + 0.03 * R * abs(sw), 16)
    elif name == '빠름' or wpn in ('b_rage', 'b_charge', 'b_break'):  # 이를 악문 입
        mouth = [(fc[0] - mw, fc[1] + 0.2 * R), (fc[0] + mw, fc[1] + 0.2 * R)]
    elif name == '재생자' or wpn == 'b_regen':  # 웃는 입
        mouth = [(fc[0] + math.cos(math.radians(a)) * mw, fc[1] + 0.14 * R + math.sin(math.radians(a)) * 0.1 * R) for a in range(20, 161, 20)]
    else:                   # 걸음마다 오물오물 (살짝 열렸다 닫힘)
        op = 0.03 * R + 0.04 * R * abs(sw)
        mouth = [(fc[0] - mw * 0.7, fc[1] + 0.2 * R), (fc[0], fc[1] + 0.2 * R + op), (fc[0] + mw * 0.7, fc[1] + 0.2 * R), (fc[0], fc[1] + 0.2 * R - op * 0.3), (fc[0] - mw * 0.7, fc[1] + 0.2 * R)]
    L.append(([W(p) for p in mouth], c, 1.3))
    if wpn in ('king', 'b_regen', 'b_warlord'):  # 왕관은 카드 위에 바로
        hx, hy = body_c[0], top
        crown = [(hx - 0.3 * R, hy), (hx - 0.3 * R, hy - 0.28 * R), (hx - 0.15 * R, hy - 0.12 * R), (hx, hy - 0.34 * R), (hx + 0.15 * R, hy - 0.12 * R),
                 (hx + 0.3 * R, hy - 0.28 * R), (hx + 0.3 * R, hy)]
        L.append(([W(p) for p in crown], HX('ffd96a'), 1.6))
    # 팔 (어깨에서, 다리와 반대로 흔든다)
    sh_y = top + 0.22 * bh
    for s in (1, -1):
        sh = W((cx + s * bw / 2, sh_y))
        a = -math.radians(45 if flop else 30) * math.sin(ph - (0.6 if flop else 0.0)) * s
        hand = (sh[0] + s * 0.14 * R + math.sin(a) * 0.07 * R, sh[1] + 0.26 * R * math.cos(a * 0.5)) # 짧은 팔
        L.append(([sh, hand], c, 1.5))
        if s == 1:
            rh = hand
        else:
            lh = hand
    # 무기
    if wpn == 'spear':
        tip = (rh[0] + 0.05 * R, rh[1] - 1.25 * R)
        L.append(([(rh[0], rh[1] + 0.35 * R), tip], c, 1.4))
        L.append(([tip, (tip[0] - 0.08 * R, tip[1] + 0.2 * R), (tip[0], tip[1] - 0.12 * R), (tip[0] + 0.08 * R, tip[1] + 0.2 * R), tip], c, 1.4))
    elif wpn == 'axe':
        top_a = (rh[0], rh[1] - 1.0 * R)
        L.append(([(rh[0], rh[1] + 0.25 * R), top_a], c, 1.8))
        L.append(([top_a, (top_a[0] + 0.32 * R, top_a[1] - 0.12 * R), (top_a[0] + 0.36 * R, top_a[1] + 0.22 * R), (top_a[0], top_a[1] + 0.2 * R)], c, 1.8))
    elif wpn == 'shield':
        sc = (cx + 0.72 * bw, body_c[1] + 0.12 * R)
        sp = [(sc[0], sc[1] - 0.42 * R), (sc[0] + 0.32 * R, sc[1] - 0.28 * R), (sc[0] + 0.28 * R, sc[1] + 0.2 * R), (sc[0], sc[1] + 0.45 * R),
              (sc[0] - 0.28 * R, sc[1] + 0.2 * R), (sc[0] - 0.32 * R, sc[1] - 0.28 * R), (sc[0], sc[1] - 0.42 * R)]
        L.append((sp, c, 2.0))
        for s in neon.suit('heart')(sc[0], sc[1], 0.15 * R):
            L.append((s, c, 1.2))
    elif wpn == 'regen':  # 떠오르는 + 둘
        for k in range(2):
            u = (t + k * 0.5) % 1.0
            px, py = cx + (0.45 if k else -0.45) * bw, top - u * 0.6 * R
            a = 0.12 * R
            cc = tuple(int(v * (1 - u)) for v in c)
            L.append(([(px - a, py), (px + a, py)], cc, 1.4))
            L.append(([(px, py - a), (px, py + a)], cc, 1.4))
    elif wpn == 'king':  # 홀 + 망토
        tip = (rh[0], rh[1] - 1.1 * R)
        L.append(([(rh[0], rh[1] + 0.2 * R), tip], c, 1.6))
        L.append((neon.circle(tip[0], tip[1] - 0.1 * R, 0.1 * R, 14), c, 1.6))
        cape = [W((cx - bw / 2 - 0.05 * R, top + 0.1 * R)), (cx - bw / 2 - 0.35 * R - 0.08 * R * sw, hip[1] + 0.15 * R), (cx - bw / 2 - 0.05 * R, hip[1] + 0.05 * R)]
        L.append((cape, c, 1.3))
    elif boss:
        boss_extra(L, wpn, c, cx, top, bw, bh, body_c, hip, lean, rh, lh, R, t, sw, ph)
    elif name == '빠름':  # 뒤로 흩날리는 속도선
        for k in range(3):
            y = top + bh * (0.25 + 0.25 * k)
            x0 = cx - bw / 2 - 0.15 * R
            L.append(([(x0, y), (x0 - (0.3 + 0.15 * ((t * 3 + k) % 1)) * R, y)], tuple(int(v * 0.6) for v in c), 1.2))
    return L


def boss_extra(L, w, c, cx, top, bw, bh, bc, hip, lean, rh, lh, R, t, sw, ph):
    """보스 장식 (D-176)"""
    def ln(pts, wd=1.5): L.append((pts, c, wd))
    def rt(pts): return [rot(p, hip, lean) for p in pts]
    if w == 'b_shield':  # 둥근 투구 + 큰 탑 방패
        ln(rt([(cx - 0.32 * R + 0.64 * R * k / 10, top - math.sin(math.pi * k / 10) * 0.3 * R) for k in range(11)]), 1.6)
        ln(rt([(cx, top - 0.3 * R), (cx, top - 0.42 * R)]))
        sx = cx + 0.62 * bw
        ln([(sx - 0.26 * R, bc[1] - 0.55 * R), (sx + 0.26 * R, bc[1] - 0.55 * R), (sx + 0.26 * R, bc[1] + 0.4 * R), (sx, bc[1] + 0.6 * R), (sx - 0.26 * R, bc[1] + 0.4 * R), (sx - 0.26 * R, bc[1] - 0.55 * R)], 2.0)
        for s in neon.suit('heart')(sx, bc[1] - 0.05 * R, 0.14 * R): ln(s, 1.2)
    elif w == 'b_split':  # 반쪽 왕관 둘 (양쪽으로 기울어짐)
        for s in (-1, 1):
            x0 = cx + s * 0.2 * R
            ln([(x0 - 0.16 * R, top - 0.02 * R + s * 0.04 * R), (x0 - 0.16 * R, top - 0.24 * R), (x0, top - 0.12 * R), (x0 + 0.16 * R, top - 0.26 * R), (x0 + 0.16 * R, top - 0.02 * R - s * 0.04 * R)], 1.4)
    elif w == 'b_regen':  # 홀 + 떠오르는 + 셋
        tip = (rh[0], rh[1] - 1.0 * R); ln([(rh[0], rh[1] + 0.2 * R), tip]); L.append((neon.circle(tip[0], tip[1] - 0.1 * R, 0.1 * R, 14), c, 1.4))
        for k in range(3):
            u = (t + k / 3) % 1.0; px, py = cx + (-0.5 + 0.5 * k) * bw, top - 0.4 * R - u * 0.5 * R; a = 0.12 * R
            cc = tuple(int(v * (1 - u)) for v in c); L.append(([(px - a, py), (px + a, py)], cc, 1.4)); L.append(([(px, py - a), (px, py + a)], cc, 1.4))
    elif w == 'b_charge':  # 깃털 투구 + 앞으로 뻗은 창
        hx, hy = rot((cx, top), hip, lean)
        ln([(hx - 0.3 * R + 0.6 * R * k / 10, hy - math.sin(math.pi * k / 10) * 0.26 * R) for k in range(11)], 1.6)
        ln([(hx, hy - 0.26 * R), (hx - 0.3 * R, hy - 0.5 * R - 0.05 * R * sw), (hx - 0.45 * R, hy - 0.38 * R)], 1.3)
        ln([(rh[0] - 0.2 * R, rh[1]), (rh[0] + 1.1 * R, rh[1] - 0.15 * R)], 1.7)
        ln([(rh[0] + 1.1 * R, rh[1] - 0.15 * R), (rh[0] + 0.9 * R, rh[1] - 0.27 * R), (rh[0] + 0.92 * R, rh[1] - 0.03 * R), (rh[0] + 1.1 * R, rh[1] - 0.15 * R)], 1.5)
    elif w == 'b_summon':  # 뾰족 모자 + 구슬 지팡이 + 주위를 도는 작은 카드 둘
        ln([(cx - 0.4 * R, top), (cx + 0.4 * R, top)], 1.5); ln([(cx - 0.26 * R, top), (cx + 0.05 * R, top - 0.62 * R), (cx + 0.26 * R, top)], 1.5)
        tip = (rh[0], rh[1] - 1.05 * R); ln([(rh[0], rh[1] + 0.25 * R), tip]); L.append((neon.circle(tip[0], tip[1] - 0.12 * R, 0.12 * R, 14), c, 1.6))
        for k in range(2):
            a = ph + k * math.pi; ox, oy = bc[0] + math.cos(a) * 0.95 * R, bc[1] + math.sin(a) * 0.3 * R
            ln([(ox - 0.08 * R, oy - 0.12 * R), (ox + 0.08 * R, oy - 0.12 * R), (ox + 0.08 * R, oy + 0.12 * R), (ox - 0.08 * R, oy + 0.12 * R), (ox - 0.08 * R, oy - 0.12 * R)], 1.1)
    elif w == 'b_ghost':  # 들어 올린 두 팔 (팔은 이미 그림) + 머리 위 물결
        ln([(cx - 0.2 * R + 0.4 * R * k / 6, top - 0.18 * R - 0.05 * R * math.sin(ph * 2 + k)) for k in range(7)], 1.1)
    elif w == 'b_warlord':  # 깃발 창 + 어깨 견장
        tip = (rh[0], rh[1] - 1.3 * R); ln([(rh[0], rh[1] + 0.3 * R), tip], 1.4)
        fl = 0.06 * R * sw; ln([tip, (tip[0] + 0.5 * R, tip[1] + 0.14 * R + fl), (tip[0], tip[1] + 0.3 * R)], 1.4)
        for s in (-1, 1): ln(rt([(cx + s * bw / 2, top + 0.14 * bh), (cx + s * (bw / 2 + 0.16 * R), top + 0.1 * bh)]), 2.0)
    elif w == 'b_rage':  # 삐죽 머리 + 양손 도끼
        ln([(cx - 0.36 * R + 0.72 * R * k / 8, top - (0.24 * R if k % 2 else 0.02 * R)) for k in range(9)], 1.5)
        for hnd, s in ((rh, 1), (lh, -1)):
            a0 = (hnd[0], hnd[1] - 0.75 * R); ln([(hnd[0], hnd[1] + 0.2 * R), a0], 1.6)
            ln([a0, (a0[0] + s * 0.26 * R, a0[1] - 0.1 * R), (a0[0] + s * 0.3 * R, a0[1] + 0.18 * R), (a0[0], a0[1] + 0.16 * R)], 1.5)
    elif w == 'b_blink':  # 뒤에 남는 잔상 카드 둘
        for k, a in ((1, 0.5), (2, 0.25)):
            ox = -k * 0.32 * R
            cc = tuple(int(v * a) for v in c)
            L.append(([(cx - bw / 2 + ox, top), (cx + bw / 2 + ox, top), (cx + bw / 2 + ox, top + bh), (cx - bw / 2 + ox, top + bh), (cx - bw / 2 + ox, top)], cc, 1.1))
    elif w == 'b_devour':  # 입으로 빨려 드는 점 셋
        mc = (bc[0], bc[1] + 0.18 * R)
        for k in range(3):
            u = (t + k / 3) % 1.0; a = k * 2.1 + 1.0
            d = (1 - u) * 0.9 * R; px, py = mc[0] + math.cos(a) * d, mc[1] + math.sin(a) * d * 0.6
            L.append((neon.circle(px, py, 0.05 * R, 8), tuple(int(v * (0.4 + 0.6 * u)) for v in c), 1.2))
    elif w == 'b_warden':  # 머리 위 고리 + 둥근 방패 + 창
        L.append(([(cx + math.cos(a) * 0.3 * R, top - 0.3 * R + math.sin(a) * 0.08 * R) for a in [k / 20 * 2 * math.pi for k in range(21)]], c, 1.4))
        L.append((neon.circle(lh[0] - 0.1 * R, lh[1] - 0.1 * R, 0.3 * R, 20), c, 1.8))
        tip = (rh[0], rh[1] - 1.2 * R); ln([(rh[0], rh[1] + 0.3 * R), tip], 1.4)
        ln([tip, (tip[0] - 0.08 * R, tip[1] + 0.2 * R), (tip[0], tip[1] - 0.12 * R), (tip[0] + 0.08 * R, tip[1] + 0.2 * R), tip], 1.3)
    elif w == 'b_hex':  # 뾰족 두건 + 별 지팡이
        ln([(cx - bw / 2, top + 0.1 * R), (cx, top - 0.5 * R), (cx + bw / 2, top + 0.1 * R)], 1.5)
        tip = (rh[0], rh[1] - 0.95 * R); ln([(rh[0], rh[1] + 0.2 * R), tip])
        for s in neon.suit('star')(tip[0], tip[1] - 0.1 * R, 0.14 * R): ln(s, 1.2)
    elif w == 'b_break':  # 큰 망치 + 카드 금
        hd = (rh[0], rh[1] - 0.9 * R); ln([(rh[0], rh[1] + 0.25 * R), hd], 1.8)
        ln([(hd[0] - 0.3 * R, hd[1] - 0.16 * R), (hd[0] + 0.3 * R, hd[1] - 0.16 * R), (hd[0] + 0.3 * R, hd[1] + 0.12 * R), (hd[0] - 0.3 * R, hd[1] + 0.12 * R), (hd[0] - 0.3 * R, hd[1] - 0.16 * R)], 2.0)
        ln(rt([(cx + 0.2 * bw, top + 0.05 * bh), (cx + 0.32 * bw, top + 0.22 * bh), (cx + 0.18 * bw, top + 0.32 * bh), (cx + 0.36 * bw, top + 0.48 * bh)]), 1.1)


def bosses(out):
    W, H, S = 1280, 1060, 2
    M = neon._Mock(S, W, H)
    R = 38
    for i, k in enumerate(BOSSES):
        row, colx = i // 2, (i % 2) * 640
        y = 40 + row * 150
        M.tx(colx + 24, y, k[0], HX(k[2]), 15, 'lm')
        for f in range(4):
            x = colx + 200 + f * 110
            for pts, c, wd in soldier(k, x, y + 125, R, f / 4):
                M.ln(pts, c, wd)
    img = Image.new('RGB', (W * S, H * S), neon.BG)
    M.draw(img).resize((W, H), Image.LANCZOS).save(out)


FLOP_PICK = [0, 1, 2, 3, 4, 5]


def flop_sheet(out):
    """흐느적 시안: 고른 종류 × 8프레임 (위) + 지금 4프레임 비교 (아래)"""
    W_, H_, S = 1280, 900, 2
    M = neon._Mock(S, W_, H_)
    R = 44
    for row, ki in enumerate(FLOP_PICK):
        k = KINDS[ki]
        y = 30 + row * 140
        M.tx(20, y + 10, k[0], HX(k[2]), 15, 'lm')
        for f in range(8):
            for pts, c, wd in soldier(k, 200 + f * 132, y + 125, R, f / 8, flop=True):
                M.ln(pts, c, wd)
    M.tx(20, H_ - 20, '흐느적 8프레임 (카드 윗부분이 걸음보다 늦게 휘청)', (95, 104, 120), 11, 'lm')
    img = Image.new('RGB', (W_ * S, H_ * S), neon.BG)
    M.draw(img).resize((W_, H_), Image.LANCZOS).save(out)


def flop_gif(out):
    W_, H_, S = 960, 300, 1
    frames = []
    N = 32
    for f in range(N):
        M = neon._Mock(S, W_, H_)
        for x in range(0, W_, 24):
            M.ln([(x, 200), (x + 12, 200)], (60, 110, 140), 1.0)
        for i, ki in enumerate(FLOP_PICK + [7]):
            k = KINDS[ki]
            spd = k[5]
            tt = (f / N * spd * 2) % 1.0
            x = (i * 135 + 60 + f / N * 135 * spd) % (W_ + 80) - 40
            for pts, c, wd in soldier(k, x, 220, 36 if ki != 7 else 40, tt, flop=True):
                M.ln(pts, c, wd)
        img = Image.new('RGB', (W_ * S, H_ * S), neon.BG)
        frames.append(M.draw(img).convert('P', palette=Image.ADAPTIVE))
    frames[0].save(out, save_all=True, append_images=frames[1:], duration=60, loop=0)


def sheet(out):
    W, H, S = 1280, 990, 2
    M = neon._Mock(S, W, H)
    R = 46
    for i, k in enumerate(KINDS):
        row, colx = i // 2, (i % 2) * 640
        y = 70 + row * 220
        M.tx(colx + 30, y - 40, k[0], HX(k[2]), 16, 'lm')
        for f in range(4):
            x = colx + 120 + f * 130
            for pts, c, wd in soldier(k, x, y + 130, R * (1.0 if k[0] != '조각' else 1.0), f / 4):
                M.ln(pts, c, wd)
        M.tx(colx + 30, y + 150, '걷기 4프레임', (95, 104, 120), 10, 'lm')
    # 게임 크기 (판 칸 48 안에서 적 키 약 30)
    M.tx(30, H - 20, '게임 크기', (95, 104, 120), 10, 'lm')
    for i, k in enumerate(KINDS):
        for pts, cc, wd in soldier(k, 140 + i * 60, H - 8, 13 if k[0] != '보스: 왕' else 17, 0.25):
            M.ln(pts, cc, wd * 0.6)
    img = Image.new('RGB', (W * S, H * S), neon.BG)
    M.draw(img).resize((W, H), Image.LANCZOS).save(out)


def gif(out):
    W, H, S = 960, 300, 1
    frames = []
    N = 24
    for f in range(N):
        M = neon._Mock(S, W, H)
        # 길
        M.ln([(0, 230), (W, 230)], (40, 70, 90), 1.0)
        for x in range(0, W, 24):
            M.ln([(x, 200), (x + 12, 200)], (60, 110, 140), 1.0)
        for i, k in enumerate(KINDS):
            spd = k[5]
            t = (f / N * spd * 2) % 1.0
            x = (i * 120 + 60 + f / N * 120 * spd) % (W + 80) - 40
            R = 34 if k[0] != '보스: 왕' else 40
            for pts, c, wd in soldier(k, x, 220, R, t):
                M.ln(pts, c, wd)
        img = Image.new('RGB', (W * S, H * S), neon.BG)
        frames.append(M.draw(img).convert('P', palette=Image.ADAPTIVE))
    frames[0].save(out, save_all=True, append_images=frames[1:], duration=70, loop=0)


if __name__ == '__main__':
    {'sheet': sheet, 'gif': gif, 'bosses': bosses, 'flop': flop_sheet, 'flopgif': flop_gif}[sys.argv[1]](sys.argv[2])
    print('saved', sys.argv[2])
