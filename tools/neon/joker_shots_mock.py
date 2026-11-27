"""조커 10종 + 상위 조커 30종 발사체 차별화 시안 (D-488, 움직이는 GIF, 네온 화풍 DOC/art/15-art-style.md 10-1절)
언리얼 코드: Source/ProjectB/PDJokerShots.cpp (같은 표 · 모양)

    python3 tools/neon/joker_shots_mock.py 출력폴더

출력: shots_base.gif (조커 10종), shots_upper.gif (상위 조커 30종), 같은 이름의 .png (한 장면).
칸마다 왼쪽 조커 타워가 오른쪽 위 적에게 쏜다. 5초, 20fps, 화면 기준 크기의 Z 배.

원칙
- 조커마다 실루엣(모양)과 색이 다르다. 한 물체 한 색 + 흰 심 (희비극만 분홍·하늘 두 색).
- 상위 조커는 재료 조커(첫 재료) 무리의 '움직임'을 물려받고, 모양 · 색은 자기 인물 상징으로.
- 착탄: 자기 실루엣이 커지며 사라지는 '도장' + 무리마다 다른 파편.
- 발사체 크기: 반지름 약 11px (지금 3~7px) — 판 위에서 한눈에 읽히게.
"""
import math
import os
import random
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

BG = (6, 6, 11)
FPS, SEC = 20, 5.0
CYCLE = 1.25          # 한 발 주기 (초)
TAU = math.tau


def hexc(h):
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


def _kfont(px):
    for p in ['C:/Windows/Fonts/malgunbd.ttf', '/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc', '/usr/share/fonts/opentype/unifont/unifont.otf']:
        if os.path.exists(p):
            return ImageFont.truetype(p, int(px))
    return ImageFont.load_default()


# ---------------------------------------------------------------- 도형 (단위 좌표: 반지름 1 ≈ 11px, x = 앞)
def circ(x, y, r, n=20, sx=1.0, sy=1.0):
    return [(x + math.cos(k / n * TAU) * r * sx, y + math.sin(k / n * TAU) * r * sy) for k in range(n + 1)]


def arcp(x, y, r, a0, a1, n=14, sx=1.0):
    return [(x + math.cos(math.radians(a0 + (a1 - a0) * k / n)) * r * sx, y + math.sin(math.radians(a0 + (a1 - a0) * k / n)) * r) for k in range(n + 1)]


def star(x, y, r, ri=0.45, n=5, rot=-90):
    return [(x + math.cos(math.radians(rot + k * 180 / n)) * (ri * r if k % 2 else r), y + math.sin(math.radians(rot + k * 180 / n)) * (ri * r if k % 2 else r)) for k in range(2 * n + 1)]


def heart(x, y, r):
    P = []
    for i in range(29):
        t = i / 28 * TAU
        hx = 16 * math.sin(t) ** 3
        hy = -(13 * math.cos(t) - 5 * math.cos(2 * t) - 2 * math.cos(3 * t) - math.cos(4 * t))
        P.append((x + hx / 17 * r, y + (hy / 17 - 0.1) * r))
    return P


def bez(p0, p1, p2, n=10):
    return [((1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * p1[0] + t * t * p2[0], (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * p1[1] + t * t * p2[1]) for t in [i / n for i in range(n + 1)]]


def rot(lines, a):
    c, s = math.cos(a), math.sin(a)
    return [[(x * c - y * s, x * s + y * c) for x, y in ln] for ln in lines]


def scl(lines, k, dx=0.0, dy=0.0):
    return [[(x * k + dx, y * k + dy) for x, y in ln] for ln in lines]


# --- 조커 10종
def s_juggle(t):    # 광대: 서로 도는 저글링 공 셋
    return [circ(math.cos(t * 9 + k * TAU / 3) * 0.55, math.sin(t * 9 + k * TAU / 3) * 0.55, 0.32, 12) for k in range(3)]


def s_hat(t):       # 모자 장수: 실크해트
    return [[(-1.1, .55), (1.1, .55)], [(-.65, .55), (-.6, -.9), (.6, -.9), (.65, .55)], [(-.63, .2), (.63, .2)]]


def s_mask(t):      # 웃는 가면: 눈가리개 + 미소
    out = [(-1, -.25), (-.55, -.6), (0, -.35), (.55, -.6), (1, -.25), (.7, .2), (0, .02), (-.7, .2), (-1, -.25)]
    return [out, circ(-.45, -.18, .16, 10, 1.3), circ(.45, -.18, .16, 10, 1.3), arcp(0, .25, .5, 25, 155, 10)]


def s_face_smile(t):
    return [circ(0, 0, .6, 18), arcp(0, .05, .32, 20, 160, 8), [(-.22, -.18), (-.1, -.18)], [(.1, -.18), (.22, -.18)]]


def s_face_frown(t):
    return [circ(0, 0, .6, 18), arcp(0, .42, .3, 200, 340, 8), [(-.22, -.15), (-.1, -.2)], [(.1, -.2), (.22, -.15)]]


def s_diamond(t):   # 할리퀸: 길쭉한 마름모
    return [[(1.1, 0), (0, .55), (-1.1, 0), (0, -.55), (1.1, 0)]]


def s_note(t):      # 지휘봉: 8분음표
    return [circ(-.3, .55, .3, 14, 1.35), [(.08, .5), (.08, -.95), (.6, -.55), (.55, -.25)]]


def s_blade(t):     # 잭: 초승달 칼날 + 자루
    return [arcp(-.7, 0, 1.4, -65, 65, 16) + arcp(-1.0, 0, 1.15, 55, -55, 14) + [arcp(-.7, 0, 1.4, -65, -65, 1)[0]], [(-1.0, 0), (-1.6, 0)], [(-1.1, -.3), (-1.1, .3)]]


def s_grin(t, eyes=True):  # 체셔: 이빨 미소 (+ 눈)
    w, y = .9, 0.0
    g = bez((-w, y), (0, y + .8), (w, y)) + bez((w, y), (0, y + .35), (-w, y))[1:]
    L = [g] + [[(x, y + .12), (x, y + .32 - abs(x) * .3)] for x in (-.4, -.2, 0, .2, .4)]
    if eyes:
        L += [[(-.55, -.45), (-.25, -.35)], [(.55, -.45), (.25, -.35)]]
    return L


def s_bell(t):      # 방울: 종 + 추
    sw = math.sin(t * 14) * 0.25
    body = bez((-.65, .5), (-.7, -.95), (0, -.85)) + bez((0, -.85), (.7, -.95), (.65, .5))[1:] + [(-.65, .5)]
    return rot([body, circ(0, .72, .16, 10), [(-.85, .5), (.85, .5)]], sw)


def s_star(t):      # 스타: 큰 별
    return [star(0, 0, 1.05)]


# --- 상위 조커 30종
def u_rose(t):      # 장미: 원 + 나선 + 잎 둘
    sp = [(math.cos(a) * a / 9.5 * .7, math.sin(a) * a / 9.5 * .7) for a in [k * .35 for k in range(28)]]
    return [circ(0, 0, .75, 20), sp, bez((-.2, .75), (-.8, 1.1), (-.9, .65)) + bez((-.9, .65), (-.5, .55), (-.2, .75))[1:]]


def u_tweedle(t):   # 트위들덤·디: 맞붙은 똑같은 얼굴 둘
    L = []
    for cx in (-.5, .5):
        L += [circ(cx, 0, .48, 16), [(cx - .18, -.1), (cx - .06, -.1)], [(cx + .06, -.1), (cx + .18, -.1)], arcp(cx, .02, .25, 20, 160, 6)]
    L.append([(-.5, -.48), (-.5, -.7)])
    L.append([(.5, -.48), (.5, -.7)])  # 머리 위 꼭지 (프로펠러 모자 막대)
    return L


def u_feather(t):   # 도도새: 깃털
    return [[(1.1, 0), (.35, -.45), (-.55, -.35), (-1.0, 0), (-.55, .35), (.35, .45), (1.1, 0)], [(-1.4, 0), (1.0, 0)],
            [(.2, 0), (-.1, -.38)], [(-.3, 0), (-.6, -.3)], [(.2, 0), (-.1, .38)], [(-.3, 0), (-.6, .3)]]


def u_smoke(t):     # 애벌레: 연기 고리 셋 (앞이 작고 뒤가 큼)
    return [circ(.7, 0, .28, 14, .55), circ(0, 0, .45, 16, .55), circ(-.85, 0, .62, 18, .55)]


def u_pig(t):       # 공작부인: 아기 돼지 얼굴
    return [circ(0, .05, .72, 20), circ(0, .22, .28, 12, 1.35), circ(-.12, .22, .05, 6), circ(.12, .22, .05, 6),
            [(-.55, -.45), (-.6, -.95), (-.2, -.65)], [(.55, -.45), (.6, -.95), (.2, -.65)], [(-.3, -.15), (-.2, -.15)], [(.2, -.15), (.3, -.15)]]


def u_mirror(t):    # 거울 나라: 손거울
    return [circ(0, -.25, .62, 20, .78), circ(0, -.25, .45, 16, .78), [(0, .37), (0, 1.05)], [(-.18, .62), (.18, .62)]]


def u_teacup(t):    # 3월 토끼: 찻잔 + 김
    st = lambda x: [(x + math.sin(k * 1.2 + t * 10) * .1, -.5 - k * .14) for k in range(5)]
    return [[(-.75, -.3), (-.5, .45), (.5, .45), (.75, -.3), (-.75, -.3)], arcp(.72, .02, .24, -80, 100, 8), [(-.6, .62), (.6, .62)], st(-.2), st(.2)]


def u_kingcrown(t):  # 하트 왕: 왕관 + 하트
    cr = [(-.85, .1), (-.95, -.75), (-.45, -.3), (0, -.95), (.45, -.3), (.95, -.75), (.85, .1), (-.85, .1)]
    return [cr, heart(0, .62, .38)]


def u_tart(t):      # 하트 잭: 격자 타르트
    L = [circ(0, 0, .8, 22), circ(0, 0, .58, 18)]
    for k in range(8):
        a = k * TAU / 8
        L.append([(math.cos(a) * .58, math.sin(a) * .58), (math.cos(a) * .8, math.sin(a) * .8)])
    L += [[(-.4, -.4), (.4, .4)], [(-.4, .4), (.4, -.4)]]
    return L


def u_teapot(t):    # 미친 다과회: 찻주전자
    return [circ(0, .1, .6, 20, 1.15), [(.65, 0), (1.15, -.45), (1.25, -.4)], arcp(-.72, .1, .3, 100, 260, 8), arcp(0, -.48, .3, 200, 340, 8), circ(0, -.65, .1, 8)]


def u_clock(t):     # 흰 토끼: 회중시계 (바늘이 돈다)
    L = [circ(0, 0, .82, 24), circ(0, -.98, .14, 10)]
    for k in range(4):
        a = k * TAU / 4
        L.append([(math.cos(a) * .62, math.sin(a) * .62), (math.cos(a) * .74, math.sin(a) * .74)])
    L.append([(0, 0), (math.cos(t * 9 - TAU / 4) * .6, math.sin(t * 9 - TAU / 4) * .6)])
    L.append([(0, 0), (math.cos(t * 1.5) * .38, math.sin(t * 1.5) * .38)])
    return L


def u_egg(t):       # 험프티 덤프티: 금 간 알
    return [circ(0, 0, .62, 22, 1.0, 1.3), [(-.6, -.05), (-.35, .15), (-.15, -.1), (.1, .15), (.32, -.1), (.6, .1)]]


def u_queen(t):     # 붉은 여왕: 체스 퀸 말
    return [[(-.65, .9), (.65, .9)], [(-.48, .9), (-.25, -.2), (.25, -.2), (.48, .9)], [(-.38, -.2), (-.48, -.62), (-.2, -.38), (0, -.78), (.2, -.38), (.48, -.62), (.38, -.2)], circ(0, -.95, .12, 8), [(-.35, .4), (.35, .4)]]


def u_turtle(t):    # 가짜 거북: 거북 등 + 머리 + 지느러미
    hx = [(math.cos(k * TAU / 6) * .32, math.sin(k * TAU / 6) * .32) for k in range(7)]
    fl = math.sin(t * 16) * .15
    return [circ(0, 0, .8, 22, 1.0, .68), hx, circ(1.0, 0, .2, 10), [(.4, -.5), (.6, -.9 - fl)], [(.4, .5), (.6, .9 + fl)], [(-.6, -.42), (-.85, -.7)], [(-.6, .42), (-.85, .7)]]


def u_wings(t):     # 그리핀: 퍼덕이는 날개 + 부리
    f = .55 + .45 * math.sin(t * 18)
    L = []
    for sgn in (-1, 1):
        L.append(bez((0, 0), (sgn * .6, -1.0 * f), (sgn * 1.15, -.55 * f)) + [(sgn * .95, -.15 * f), (sgn * .8, -.05), (sgn * .6, .1), (0, .1)])
    L.append([(-.18, -.05), (0, -.45), (.18, -.05)])
    return L


def u_grinonly(t):  # 웃음만 남은 고양이: 눈 없는 미소
    return s_grin(t, eyes=False)


def u_snow(t):      # 하얀 여왕: 육각 눈송이
    L = []
    for k in range(3):
        a = k * TAU / 6 + math.pi / 2
        L.append([(math.cos(a) * .85, math.sin(a) * .85), (-math.cos(a) * .85, -math.sin(a) * .85)])
    for k in range(6):
        a = k * TAU / 6 + math.pi / 2
        m = (math.cos(a) * .5, math.sin(a) * .5)
        for d in (-.6, .6):
            L.append([m, (m[0] + math.cos(a + d) * .3, m[1] + math.sin(a + d) * .3)])
    return L


def u_oyster(t):    # 바다코끼리와 목수: 굴 껍데기
    L = [arcp(0, .35, .85, 200, 340, 14) + [(0, .55), arcp(0, .35, .85, 200, 200, 1)[0]]]
    for a in (230, 255, 285, 310):
        L.append([(0, .55), (math.cos(math.radians(a)) * .75, .35 + math.sin(math.radians(a)) * .75)])
    return L


def u_lance(t):     # 하얀 기사: 랜스
    return [[(-1.25, 0), (.6, 0)], [(.5, -.2), (1.35, 0), (.5, .2), (.5, -.2)], [(-.55, -.38), (-.15, 0), (-.55, .38)]]


def u_shield(t):    # 하얀 기사: 방패
    return [[(-.55, -.6), (.55, -.6), (.5, .2), (0, .75), (-.5, .2), (-.55, -.6)], [(0, -.6), (0, .75)], [(-.52, -.15), (.52, -.15)]]


def u_daisy(t):     # 말하는 꽃들: 데이지
    L = [circ(0, 0, .22, 10)]
    for k in range(6):
        a = k * TAU / 6 + t * 3
        L += place([circ(0, 0, 1, 12, 1.0, .5)], (math.cos(a) * .58, math.sin(a) * .58), a, .32)
    return L


def u_hearts(t):    # 하트 여왕: 하트
    return [heart(0, 0, .85)]


def u_hedgehog(t):  # 홍학 크로케: 가시 공
    L = [circ(0, 0, .5, 16)]
    for k in range(9):
        a = k * TAU / 9 + t * 8
        L.append([(math.cos(a) * .5, math.sin(a) * .5), (math.cos(a) * .9, math.sin(a) * .9)])
    return L


def u_card(t):      # 트럼프 근위대: 날아가는 카드 (스페이드)
    w, h, r = .55, .8, .15
    box = arcp(w - r, -h + r, r, -90, 0, 3) + arcp(w - r, h - r, r, 0, 90, 3) + arcp(-w + r, h - r, r, 90, 180, 3) + arcp(-w + r, -h + r, r, 180, 270, 3)
    box.append(box[0])
    sp = [(x, -y) for x, y in heart(0, -.02, .3)]
    return [box, sp, [(0, .15), (.12, .32), (-.12, .32), (0, .15)]]


def u_checker(t):   # 체스판: 2×2 체크 (두 칸 빗금)
    L = [[(-.75, -.75), (.75, -.75), (.75, .75), (-.75, .75), (-.75, -.75)], [(0, -.75), (0, .75)], [(-.75, 0), (.75, 0)]]
    for x0, y0 in ((-.75, -.75), (0, 0)):
        for k in (.25, .5):
            L.append([(x0 + k, y0), (x0, y0 + k)])
            L.append([(x0 + .75, y0 + k), (x0 + k, y0 + .75)])
    return L


def u_zzz(t):       # 겨울잠쥐: Z z z
    L = []
    for k, (x, y, s) in enumerate([(-.55, .35, .5), (.15, -.05, .36), (.65, -.5, .26)]):
        yy = y + math.sin(t * 6 + k) * .08
        L.append([(x - s, yy - s), (x + s, yy - s), (x - s, yy + s), (x + s, yy + s)])
    return L


def u_horn(t):      # 유니콘: 나선 뿔
    L = [[(-.9, -.32), (1.2, 0), (-.9, .32), (-.9, -.32)]]
    ph = (t * 4) % 1.0
    for k in range(4):
        x = -.75 + (k + ph) * .45
        h = .32 * (1.2 - x) / 2.1
        if x < 1.05:
            L.append([(x - .12, -h), (x + .12, h)])
    return L


def u_claws(t):     # 재버워키: 발톱 세 줄
    return [arcp(-.9, y, 1.3, -50, 50, 12) for y in (-.5, 0, .5)]


def u_lion(t):      # 사자: 갈기 + 얼굴
    mane = [((.95 if k % 2 else .68) * math.cos(k * TAU / 22), (.95 if k % 2 else .68) * math.sin(k * TAU / 22)) for k in range(23)]
    return [mane, circ(0, 0, .45, 16), circ(-.16, -.08, .05, 6), circ(.16, -.08, .05, 6), [(-.12, .18), (0, .26), (.12, .18)]]


def u_cloud(t):     # 꿈꾸는 붉은 왕: 꿈 구름 + 작은 왕관
    c = arcp(-.55, .1, .38, 90, 250, 8) + arcp(-.1, -.15, .45, 200, 330, 8) + arcp(.5, .05, .4, 250, 450, 10) + [(-.55, .48)]
    return [c, [(-.3, -.6), (-.35, -.95), (-.15, -.75), (0, -1.0), (.15, -.75), (.35, -.95), (.3, -.6), (-.3, -.6)]]


def u_bat(t):       # 반짝반짝 작은 박쥐: 퍼덕이는 박쥐
    f = .6 + .4 * math.sin(t * 20)
    L = [circ(0, 0, .22, 10, 1.0, 1.4), [(-.12, -.28), (-.18, -.48)], [(.12, -.28), (.18, -.48)]]
    for s in (-1, 1):
        L.append([(s * .2, -.1), (s * .65, -.55 * f), (s * 1.2, -.35 * f), (s * 1.0, .1 * f), (s * .8, -.02), (s * .62, .22 * f), (s * .42, .02), (s * .2, .2)])
    return L


# ---------------------------------------------------------------- 무리(움직임 · 꼬리 · 착탄)
# 움직임: lob 포물선, boomerang 휘며 돌기, spin 직선 회전, twin 두 줄 감기, fan 부채꼴 셋, wave 물결, slash 느린 큰 베기, instant 즉시 광선, ring 소리 고리, comet 혜성
FAMILY = {
    'lob':       dict(dur=.8, orient='up'),
    'boomerang': dict(dur=.75, orient='spin'),
    'spin':      dict(dur=.6, orient='spin'),
    'twin':      dict(dur=.7, orient='up'),
    'fan':       dict(dur=.5, orient='dir'),
    'wave':      dict(dur=.8, orient='up'),
    'slash':     dict(dur=.9, orient='dir'),
    'instant':   dict(dur=0., orient='up'),
    'ring':      dict(dur=.7, orient='up'),
    'comet':     dict(dur=.55, orient='spinslow'),
}

# (이름, 색, 무리, 모양, 착탄 파편, 설명 한 줄, [보조 모양, 보조 색])
BASE = [
    ('광대', 'ffa45a', 'lob', s_juggle, 'confetti', '서로 도는 저글링 공 셋, 포물선 + 통통 그림자'),
    ('모자 장수', '4ff0c8', 'boomerang', s_hat, 'poof', '빙글 도는 실크해트, 옆으로 휘어 날아감'),
    ('웃는 가면', 'f2ff6a', 'spin', s_mask, 'crescent', '도는 눈가리개 가면, 착탄에 웃는 호'),
    ('희비극', 'ff9ad5', 'twin', s_face_smile, 'cross', '웃는 얼굴(분홍) · 우는 얼굴(하늘)이 감기며', s_face_frown, '7fd6ff'),
    ('할리퀸', 'ff5ad2', 'fan', s_diamond, 'shards', '마름모 셋이 부채꼴로 벌어졌다 모임'),
    ('지휘봉', 'a6ff7a', 'wave', s_note, 'sparkle', '물결치며 나는 8분음표, 떠오르는 반짝이'),
    ('잭', '6a8cff', 'slash', s_blade, 'xcut', '큰 초승달 칼날이 느리게, X 자 베기'),
    ('체셔', 'b77aff', 'instant', s_grin, 'spiral', '즉시 보라 광선, 적 위에 이빨 미소가 남음'),
    ('방울', 'ffd25a', 'ring', s_bell, 'rings', '흔들리는 종이 소리 고리를 뿜으며'),
    ('스타', 'ffffff', 'comet', s_star, 'rays', '천천히 도는 큰 별 + 긴 꼬리 셋'),
]
# 상위 조커 (DOC/design/18 4절 순서). 무리 = 지금 레시피의 첫 재료 조커
UPPER = [
    ('장미 칠하는 정원사', 'ff6a5a', 'lob', u_rose, 'paint', '붉은 장미, 페인트 방울을 흘리며'),
    ('트위들덤과 트위들디', 'ffe45a', 'lob', u_tweedle, 'confetti', '맞붙은 똑같은 얼굴 둘이 통통'),
    ('도도새', 'e6b27a', 'lob', u_feather, 'feathers', '깃털, 착탄에 깃털이 흩날림'),
    ('애벌레', '5ab4ff', 'lob', u_smoke, 'smoke', '연기 고리 셋, 커지며 흩어짐'),
    ('공작부인', 'ffb4c8', 'lob', u_pig, 'poof', '아기 돼지 얼굴, 후추 재채기 연기'),
    ('거울 나라', 'dfe6ff', 'lob', u_mirror, 'mirror', '손거울 + 길 건너편에 거울상'),
    ('3월 토끼', 'c8ff5a', 'boomerang', u_teacup, 'splash', '김 나는 찻잔이 빙글'),
    ('하트 왕', 'ffbe5a', 'boomerang', u_kingcrown, 'rays', '하트 단 왕관이 빙글'),
    ('하트 잭', 'ff9a7a', 'boomerang', u_tart, 'crumbs', '훔친 격자 타르트, 부스러기'),
    ('미친 다과회', '6affb0', 'boomerang', u_teapot, 'splash', '찻주전자가 빙글, 찻물 튐'),
    ('흰 토끼', 'ffffff', 'boomerang', u_clock, 'tick', '바늘 도는 회중시계, 착탄에 째깍 멈춤'),
    ('험프티 덤프티', 'fff0b0', 'spin', u_egg, 'crack', '금 간 알, 착탄에 깨짐'),
    ('붉은 여왕', 'ff3a4a', 'spin', u_queen, 'xcut', '체스 퀸 말 + 달리는 잔상', None, None, 'ghost'),
    ('가짜 거북', 'b8d870', 'spin', u_turtle, 'tears', '헤엄치는 거북, 눈물 방울'),
    ('그리핀', 'e0b860', 'spin', u_wings, 'feathers', '퍼덕이는 날개 + 부리'),
    ('웃음만 남은 고양이', 'e070ff', 'spin', u_grinonly, 'spiral', '눈 없는 미소가 깜빡이며', None, None, 'blink'),
    ('하얀 여왕', 'cfeaff', 'twin', u_snow, 'frost', '눈송이 결정 둘이 감기며'),
    ('바다코끼리와 목수', '4ab8e8', 'twin', u_oyster, 'bubbles', '굴 껍데기 둘, 거품'),
    ('하얀 기사', 'a8b8ff', 'twin', u_lance, 'xcut', '랜스와 방패가 나란히', u_shield, 'a8b8ff'),
    ('말하는 꽃들', 'd8a0ff', 'twin', u_daisy, 'petals', '도는 데이지 둘, 꽃잎'),
    ('하트 여왕', 'ff4a7a', 'fan', u_hearts, 'hearts', '하트 셋 부채꼴, 착탄에 X'),
    ('홍학 크로케', 'ff6aa0', 'fan', u_hedgehog, 'shards', '도는 가시 공 셋'),
    ('트럼프 근위대', 'c8d0e0', 'fan', u_card, 'cards', '스페이드 카드 셋이 줄지어'),
    ('체스판', '9fb0d0', 'wave', u_checker, 'squares', '체크무늬 칸이 물결치며'),
    ('겨울잠쥐', 'b0a0ff', 'wave', u_zzz, 'bubbles', 'Z z z 가 둥실둥실'),
    ('유니콘', 'ffc0f0', 'wave', u_horn, 'sparkle', '나선 뿔, 반짝이 꼬리'),
    ('재버워키', '6aff5a', 'slash', u_claws, 'xcut', '발톱 세 줄 베기'),
    ('사자', 'ffc83a', 'slash', u_lion, 'rays', '갈기 사자 얼굴이 덮침'),
    ('꿈꾸는 붉은 왕', 'ff7a9a', 'instant', u_cloud, 'zzz', '즉시 광선, 적 위에 꿈 구름'),
    ('반짝반짝 작은 박쥐', '8a6aff', 'ring', u_bat, 'sparkle', '퍼덕이는 박쥐가 초음파 고리'),
]


def spec(row):
    name, col, fam, shape, imp, desc = row[:6]
    shape2 = row[6] if len(row) > 6 else None
    col2 = row[7] if len(row) > 7 else None
    extra = row[8] if len(row) > 8 else None
    return dict(name=name, col=hexc(col), fam=fam, shape=shape, imp=imp, desc=desc, shape2=shape2, col2=hexc(col2) if col2 else hexc(col), extra=extra)


# ---------------------------------------------------------------- 그리기
class Canvas:
    """한 프레임: 번짐 층 + 심 층 (화풍: 넓고 옅은 번짐 + 가는 심)"""

    def __init__(self, W, H, S):
        self.S = S
        self.glow = Image.new('RGB', (W * S, H * S), (0, 0, 0))
        self.core = Image.new('RGB', (W * S, H * S), (0, 0, 0))
        self.gd = ImageDraw.Draw(self.glow)
        self.cd = ImageDraw.Draw(self.core)

    def lines(self, lines, col, a=1.0, w=1.6, white=False):
        S = self.S
        c = tuple(int(v * a) for v in col)
        for ln in lines:
            if len(ln) < 2:
                continue
            pts = [(x * S, y * S) for x, y in ln]
            self.gd.line(pts, fill=c, width=max(1, int(w * 3.2 * S)), joint='curve')
            self.cd.line(pts, fill=c, width=max(1, int(round(w * S))), joint='curve')
            if white:
                self.cd.line(pts, fill=tuple(int(255 * a * .85) for _ in range(3)), width=max(1, int(round(w * .45 * S))))

    def box(self, x, y, s, col, a=1.0):
        S = self.S
        c = tuple(int(v * a) for v in col)
        self.cd.rectangle([(x - s / 2) * S, (y - s / 2) * S, (x + s / 2) * S, (y + s / 2) * S], fill=c)
        self.gd.rectangle([(x - s * 2) * S, (y - s * 2) * S, (x + s * 2) * S, (y + s * 2) * S], fill=tuple(int(v * .15) for v in c))

    def finish(self):
        S = self.S
        g = self.glow
        b1 = g.filter(ImageFilter.GaussianBlur(6 * S)).point(lambda v: int(v * .75))
        b2 = g.filter(ImageFilter.GaussianBlur(2 * S)).point(lambda v: int(v * .3))
        img = Image.new('RGB', g.size, BG)
        img = ImageChops.add(img, b1)
        img = ImageChops.add(img, b2)
        img = ImageChops.add(img, self.core)
        return img


def place(lines, P, ang, R):
    return [[(P[0] + (x * math.cos(ang) - y * math.sin(ang)) * R, P[1] + (x * math.sin(ang) + y * math.cos(ang)) * R) for x, y in ln] for ln in lines]


def ease_out(u):
    return 1 - (1 - u) ** 3


def draw_cell(cv, sp, ox, oy, cw, ch, t, Z, label_font, desc_font, idx):
    """칸 하나 그리기. 화면 기준 좌표 × Z"""
    col = sp['col']
    R = 11 * Z
    A = (ox + 34 * Z, oy + ch - 40 * Z)
    B = (ox + cw - 40 * Z, oy + 52 * Z)
    d = ((B[0] - A[0]), (B[1] - A[1]))
    L = math.hypot(*d)
    dn = (d[0] / L, d[1] / L)
    nrm = (-dn[1], dn[0])
    dang = math.atan2(dn[1], dn[0])
    # 타워 (분홍 조커 틀) · 적 (흰 원)
    tw = 13 * Z
    fr = [(A[0] - tw, A[1] - tw), (A[0] + tw, A[1] - tw), (A[0] + tw, A[1] + tw), (A[0] - tw, A[1] + tw), (A[0] - tw, A[1] - tw)]
    cv.lines([fr], (255, 154, 213), .55, 1.2 * Z)
    cv.lines(scl(s_hat(0), tw * .45, A[0], A[1] + tw * .1), (255, 154, 213), .45, 1.0 * Z)
    fam = FAMILY[sp['fam']]
    cyc = t % CYCLE
    k_shot = int(t // CYCLE)
    dur = fam['dur']
    hit = cyc >= dur
    hit_t = cyc - dur
    # 적: 맞으면 흔들림
    shake = (math.sin(hit_t * 80) * 2 * Z * max(0, 1 - hit_t / .2)) if hit else 0
    cv.lines([circ(B[0] + shake, B[1], 8 * Z, 18)], (232, 240, 255), .8, 1.3 * Z)
    shape = sp['shape']
    fname = sp['fam']

    def P_at(u, tt, j=0):
        x = A[0] + d[0] * u
        y = A[1] + d[1] * u
        if fname == 'lob':
            h = math.sin(math.pi * u) * 30 * Z
            y -= h
            if sp['shape'] is u_tweedle:  # 통통 튐
                y -= abs(math.sin(u * math.pi * 3)) * 6 * Z
        elif fname == 'boomerang':
            off = math.sin(math.pi * u) * 26 * Z
            x += nrm[0] * off
            y += nrm[1] * off
        elif fname == 'twin':
            off = math.sin(tt * 16) * 7 * Z * (1 if j == 0 else -1) * math.sin(math.pi * min(1, u * 1.2 + .1))
            x += nrm[0] * off
            y += nrm[1] * off
        elif fname == 'fan':
            off = (j - 1) * math.sin(math.pi * u) * 16 * Z
            x += nrm[0] * off - dn[0] * abs(j - 1) * 6 * Z * (1 - u)
            y += nrm[1] * off - dn[1] * abs(j - 1) * 6 * Z * (1 - u)
        elif fname == 'wave':
            off = math.sin(tt * 14) * 7 * Z
            x += nrm[0] * off
            y += nrm[1] * off
        return (x, y)

    def ang_at(u, tt, j=0):
        o = fam['orient']
        if o == 'dir':
            if fname == 'fan':
                return dang + (j - 1) * .35 * math.cos(math.pi * u * .9)
            return dang
        if o == 'spin':
            return tt * (14 if fname == 'boomerang' else 11)
        if o == 'spinslow':
            return tt * 3
        return 0.0

    alpha_blink = 1.0
    if sp['extra'] == 'blink' and not hit:
        alpha_blink = .35 + .65 * (0.5 + 0.5 * math.cos(cyc * 28))

    if not hit and fname != 'instant':
        u = cyc / dur
        ue = u if fname not in ('slash',) else u ** 1.4
        n_copies = 3 if fname == 'fan' else 2 if fname == 'twin' else 1
        # 꼬리
        for j in range(n_copies):
            if fname == 'comet':
                Pp = P_at(ue, cyc)
                for s in (-1, 0, 1):
                    cv.lines([[(Pp[0] - dn[0] * 4 * Z + nrm[0] * s * 1.5 * Z, Pp[1] - dn[1] * 4 * Z + nrm[1] * s * 1.5 * Z),
                               (Pp[0] - dn[0] * 34 * Z + nrm[0] * s * 7 * Z, Pp[1] - dn[1] * 34 * Z + nrm[1] * s * 7 * Z)]], col, .55, 1.2 * Z)
            elif fname == 'ring':
                for k in range(3):
                    uu = max(0, ue - .1 * (k + 1))
                    Pp = P_at(uu, cyc)
                    rr = (6 + k * 4) * Z
                    cv.lines(place([arcp(0, 0, 1, -55, 55, 10)], (Pp[0] + dn[0] * 2 * Z, Pp[1] + dn[1] * 2 * Z), dang, rr), col, .9 - k * .25, 1.2 * Z)
            elif sp['extra'] == 'ghost':
                for k in (1, 2):
                    uu = max(0, ue - .09 * k)
                    cv.lines(place(shape(cyc), P_at(uu, cyc), ang_at(uu, cyc - .05 * k), R), col, .4 - .15 * k, 1.4 * Z)
            else:
                for k in range(1, 6):  # 점 꼬리
                    uu = ue - .045 * k
                    if uu <= 0:
                        break
                    Pp = P_at(uu, cyc - .03 * k, j)
                    cv.box(Pp[0], Pp[1], max(1.5, 2.6 - .3 * k) * Z, sp['col2'] if j == 1 else col, .8 - .14 * k)
        # 몸통
        for j in range(n_copies):
            shp = sp['shape2'] if (j == 1 and sp['shape2']) else shape
            c = sp['col2'] if j == 1 else col
            Pp = P_at(ue, cyc, j)
            cv.lines(place(shp(cyc), Pp, ang_at(ue, cyc, j), R), c, alpha_blink, 1.5 * Z, white=(fname == 'comet'))
            if sp['shape'] is u_mirror:  # 거울상: 직선 길 건너편에 반전
                Q = (A[0] + d[0] * ue, A[1] + d[1] * ue)
                M = (2 * Q[0] - Pp[0], 2 * Q[1] - Pp[1])
                cv.lines(place([[(-x, y) for x, y in ln] for ln in shp(cyc)], M, 0, R), c, .4, 1.3 * Z)
            if sp['shape'] is u_rose:  # 페인트 방울
                for k in range(3):
                    tt = (cyc * 3 + k / 3) % 1
                    cv.box(Pp[0] - 2 * Z, Pp[1] + tt * 14 * Z, 2.2 * Z, col, .8 * (1 - tt))
    if fname == 'instant' and cyc < .35:
        a = 1 - cyc / .35
        pts = [A]
        for k in range(1, 6):
            uu = k / 6
            pts.append((A[0] + d[0] * uu + nrm[0] * math.sin(uu * 9 + cyc * 40) * 3 * Z, A[1] + d[1] * uu + nrm[1] * math.sin(uu * 9 + cyc * 40) * 3 * Z))
        pts.append(B)
        cv.lines([pts], col, a, 1.6 * Z, white=True)
    # 착탄
    if hit or fname == 'instant':
        ht = hit_t if fname != 'instant' else cyc
        life = .5
        if ht < life:
            u = ht / life
            a = 1 - u
            # 도장: 자기 실루엣이 커지며 사라짐
            k = 1.0 + 1.3 * ease_out(u)
            if fname == 'instant':
                k = 1.6 - .2 * u
                a = 1 - u ** 2
            cv.lines(place(shape(ht), B, ang_at(1, ht) if fam['orient'] == 'dir' else 0, R * k), col, a, 1.5 * Z)
            if sp['shape2']:
                cv.lines(place(sp['shape2'](ht), (B[0] + 10 * Z * u, B[1]), 0, R * k * .8), sp['col2'], a * .8, 1.3 * Z)
            impact(cv, sp, B, dn, ht, u, a, Z, k_shot)
    # 이름
    cv.cd.text(((ox + 8 * Z) * cv.S, (oy + 6 * Z) * cv.S), f'{idx}. {sp["name"]}', fill=col, font=label_font)
    cv.cd.text(((ox + 8 * Z) * cv.S, (oy + ch - 12 * Z) * cv.S), sp['desc'], fill=(138, 148, 166), font=desc_font)


def impact(cv, sp, B, dn, ht, u, a, Z, seed):
    col = sp['col']
    imp = sp['imp']
    rnd = random.Random(hash((sp['name'], seed)) & 0xffff)
    nrm = (-dn[1], dn[0])
    eo = ease_out(u)

    def parts(n, spd, grav=0.0, size=2.2, c=None, cone=None, lines=False):
        for i in range(n):
            ang = rnd.uniform(0, TAU) if cone is None else math.atan2(cone[1], cone[0]) + rnd.uniform(-.7, .7)
            v = rnd.uniform(.5, 1) * spd * Z
            x = B[0] + math.cos(ang) * v * eo * .5
            y = B[1] + math.sin(ang) * v * eo * .5 + grav * Z * ht * ht
            if lines:
                cv.lines([[(x, y), (x - math.cos(ang) * 5 * Z, y - math.sin(ang) * 5 * Z)]], c or col, a, 1.1 * Z)
            else:
                cv.box(x, y, size * Z, c or col, a)

    def ring(r0, r1, c=None, w=1.3, n=24, broken=0):
        r = (r0 + (r1 - r0) * eo) * Z
        if broken:
            for k in range(broken):
                a0 = k * 360 / broken + u * 90
                cv.lines([arcp(B[0], B[1], r, a0, a0 + 360 / broken * .55, 6)], c or col, a, w * Z)
        else:
            cv.lines([circ(B[0], B[1], r, n)], c or col, a, w * Z)

    if imp == 'confetti':
        for i in range(10):
            ang = rnd.uniform(-math.pi, 0)
            v = rnd.uniform(40, 90) * Z
            x, y = B[0] + math.cos(ang) * v * ht, B[1] + math.sin(ang) * v * ht + 160 * Z * ht * ht
            cv.box(x, y, 2.6 * Z, col if i % 2 else (255, 255, 255), a)
    elif imp == 'poof':
        ring(4, 18, broken=0, n=4 if False else 24)
        parts(8, 40, -20, 3.2, (138, 128, 144))
    elif imp == 'crescent':
        for s in (-1, 1):
            cv.lines([arcp(B[0], B[1], (6 + 16 * eo) * Z, 90 * s + u * 200, 90 * s + 70 + u * 200, 8)], col, a, 1.4 * Z)
    elif imp == 'cross':
        cv.lines([place([[(0, -1), (.87, .5), (-.87, .5), (0, -1)]], B, u * 2, (5 + 14 * eo) * Z)[0]], col, a, 1.4 * Z)
        cv.lines([place([[(0, -1), (.87, .5), (-.87, .5), (0, -1)]], B, math.pi - u * 2, (5 + 14 * eo) * Z)[0]], sp['col2'], a, 1.4 * Z)
    elif imp == 'shards':
        for i in range(8):
            ang = rnd.uniform(0, TAU)
            v = rnd.uniform(60, 120) * Z * eo * .5
            P = (B[0] + math.cos(ang) * v, B[1] + math.sin(ang) * v)
            cv.lines(place([[(1, 0), (0, .5), (-1, 0), (0, -.5), (1, 0)]], P, ang + u * 6, 3 * Z), col, a, 1.1 * Z)
    elif imp == 'sparkle':
        for i in range(8):
            x = B[0] + rnd.uniform(-16, 16) * Z
            y = B[1] + rnd.uniform(-6, 6) * Z - 40 * Z * ht
            s = rnd.uniform(2, 4) * Z
            cv.lines([[(x - s, y), (x + s, y)], [(x, y - s), (x, y + s)]], col, a * (0.5 + 0.5 * math.sin(ht * 30 + i)), 1.0 * Z)
    elif imp == 'xcut':
        L = 18 * Z * min(1, u * 4)
        for s in (-1, 1):
            cv.lines([[(B[0] - L, B[1] - L * .75 * s), (B[0] + L, B[1] + L * .75 * s)]], col, a, 2.0 * Z, white=True)
        parts(10, 120, 0, 0, (255, 255, 255), lines=True)
    elif imp == 'spiral':
        for k in range(3):
            a0 = k * 120 + u * 400
            cv.lines([arcp(B[0], B[1], (26 - 20 * eo) * Z, a0, a0 + 60, 8)], col, a, 1.4 * Z)
    elif imp == 'rings':
        for k in range(4):
            uu = max(0, u - k * .12)
            r = (4 + 26 * ease_out(uu) + k * 6) * Z
            for s in (-1, 1):
                pass
            cv.lines([arcp(B[0], B[1], r, -60 + 180 * (k % 2), 60 + 180 * (k % 2), 10)], col, a * (1 - k * .2), (1.8 - k * .3) * Z)
    elif imp == 'rays':
        for k in range(8):
            ang = k * TAU / 8 + seed
            r0, r1 = (6 + 10 * eo) * Z, (14 + 22 * eo) * Z
            cv.lines([[(B[0] + math.cos(ang) * r0, B[1] + math.sin(ang) * r0), (B[0] + math.cos(ang) * r1, B[1] + math.sin(ang) * r1)]], col, a, 1.3 * Z)
    elif imp == 'paint':
        for i in range(9):
            ang = rnd.uniform(0, TAU)
            v = rnd.uniform(14, 26) * Z * eo
            cv.lines([circ(B[0] + math.cos(ang) * v, B[1] + math.sin(ang) * v + 30 * Z * ht * ht, rnd.uniform(1.5, 3) * Z, 8)], col, a, 1.1 * Z)
    elif imp == 'feathers':
        for i in range(5):
            ang = rnd.uniform(-math.pi, 0)
            v = rnd.uniform(20, 40) * Z
            P = (B[0] + math.cos(ang) * v * eo + math.sin(ht * 12 + i) * 4 * Z, B[1] + math.sin(ang) * v * eo + 40 * Z * ht * ht)
            cv.lines(place(u_feather(0)[:2], P, ang + math.sin(ht * 10 + i), 3.5 * Z), col, a, 1.0 * Z)
    elif imp == 'smoke':
        for k in range(3):
            cv.lines([circ(B[0] + (k - 1) * 10 * Z * eo, B[1] - 16 * Z * ht - k * 3 * Z, (4 + 8 * eo + k * 2) * Z, 18, 1.0, .55)], col, a * (1 - k * .2), 1.2 * Z)
    elif imp == 'mirror':
        ring(4, 20, w=1.4)
        for s in (-1, 1):
            cv.lines([[(B[0] + s * 8 * Z * eo, B[1] - 12 * Z), (B[0] + s * 8 * Z * eo, B[1] + 12 * Z)]], col, a, 1.2 * Z)
        parts(6, 80, 0, 0, (255, 255, 255), lines=True)
    elif imp == 'splash':
        parts(12, 70, 140, 2.2)
        cv.lines([circ(B[0], B[1] + 4 * Z, (4 + 14 * eo) * Z, 18, 1.0, .4)], col, a, 1.2 * Z)
    elif imp == 'crumbs':
        parts(12, 70, 160, 2.4)
    elif imp == 'tick':
        for k in range(12):
            ang = k * TAU / 12
            r0, r1 = (12 + 8 * eo) * Z, (16 + 8 * eo) * Z
            cv.lines([[(B[0] + math.cos(ang) * r0, B[1] + math.sin(ang) * r0), (B[0] + math.cos(ang) * r1, B[1] + math.sin(ang) * r1)]], col, a, 1.1 * Z)
    elif imp == 'crack':
        for s in (-1, 1):
            cv.lines(place([arcp(0, 0, .62, 180 if s < 0 else 0, 360 if s < 0 else 180, 10)], (B[0] + s * 8 * Z * eo, B[1] + 6 * Z * eo * (1 if s > 0 else -1) + 30 * Z * ht * ht), s * u * .8, 9 * Z * 1.3), col, a, 1.3 * Z)
    elif imp == 'tears':
        for i in range(6):
            x = B[0] + (i - 2.5) * 5 * Z
            y = B[1] + 50 * Z * ht * ht * (1 + i % 3 * .4) + 4 * Z
            cv.lines([[(x, y - 3 * Z), (x + 1.5 * Z, y), (x, y + 1.5 * Z), (x - 1.5 * Z, y), (x, y - 3 * Z)]], col, a, 1.0 * Z)
    elif imp == 'frost':
        cv.lines(place(u_snow(0), B, u * 1.5, (8 + 14 * eo) * Z), col, a, 1.2 * Z)
        parts(10, 50, 30, 2.0, (255, 255, 255))
    elif imp == 'bubbles':
        for i in range(7):
            x = B[0] + rnd.uniform(-14, 14) * Z + math.sin(ht * 14 + i) * 2 * Z
            y = B[1] - rnd.uniform(10, 50) * Z * ht * 2
            cv.lines([circ(x, y, rnd.uniform(1.5, 3.5) * Z, 10)], col, a, 1.0 * Z)
    elif imp == 'petals':
        for i in range(8):
            ang = i * TAU / 8 + u * 2
            v = (6 + 22 * eo) * Z
            cv.lines(place([circ(0, 0, 1, 10, 1.0, .5)], (B[0] + math.cos(ang) * v, B[1] + math.sin(ang) * v + 20 * Z * ht * ht), ang, 3 * Z), col, a, 1.0 * Z)
    elif imp == 'hearts':
        L = 16 * Z * min(1, u * 4)
        for s in (-1, 1):
            cv.lines([[(B[0] - L, B[1] - L * s), (B[0] + L, B[1] + L * s)]], col, a, 1.8 * Z, white=True)
        for i in range(5):
            ang = rnd.uniform(0, TAU)
            v = rnd.uniform(14, 28) * Z * eo
            cv.lines([heart(B[0] + math.cos(ang) * v, B[1] + math.sin(ang) * v, 3 * Z)], col, a, 1.0 * Z)
    elif imp == 'cards':
        for i in range(5):
            ang = rnd.uniform(0, TAU)
            v = rnd.uniform(18, 34) * Z * eo
            cv.lines(place([[(-.55, -.8), (.55, -.8), (.55, .8), (-.55, .8), (-.55, -.8)]], (B[0] + math.cos(ang) * v, B[1] + math.sin(ang) * v + 30 * Z * ht * ht), ang + u * 5, 4 * Z), col, a, 1.0 * Z)
    elif imp == 'squares':
        for i in range(9):
            gx, gy = i % 3 - 1, i // 3 - 1
            if (gx + gy) % 2:
                continue
            s = 7 * Z * (1 + eo * .6)
            x, y = B[0] + gx * s, B[1] + gy * s
            h = s * .45 * (1 - u * .5)
            cv.lines([[(x - h, y - h), (x + h, y - h), (x + h, y + h), (x - h, y + h), (x - h, y - h)]], col, a, 1.1 * Z)
    elif imp == 'zzz':
        for k in range(3):
            x = B[0] + 8 * Z + k * 7 * Z
            y = B[1] - 10 * Z - k * 8 * Z - 20 * Z * ht
            s = (3 - k * .6) * Z
            cv.lines([[(x - s, y - s), (x + s, y - s), (x - s, y + s), (x + s, y + s)]], col, a, 1.1 * Z)
    if imp not in ('confetti',):
        ring(3, 10, w=1.0) if u < .5 else None


def render_sheet(specs, out_base, cols, Z, start_idx=1, S=1):
    cw, ch = int(210 * Z), int(150 * Z)
    rows = (len(specs) + cols - 1) // cols
    W, H = cw * cols, ch * rows
    lf = _kfont(11 * Z * S)
    df = _kfont(8 * Z * S)
    frames = []
    n = int(FPS * SEC)
    for f in range(n):
        t = f / FPS
        cv = Canvas(W, H, S)
        for i, sp in enumerate(specs):
            ox, oy = (i % cols) * cw, (i // cols) * ch
            cv.lines([[(ox, oy), (ox + cw, oy)], [(ox, oy), (ox, oy + ch)]], (120, 140, 180), .12, .6)
            draw_cell(cv, sp, ox, oy, cw, ch, t + (i % 3) * .21, Z, lf, df, start_idx + i)
        img = cv.finish()
        if S != 1:
            img = img.resize((W, H), Image.LANCZOS)
        frames.append(img)
        if f == int(n * .37):
            img.save(out_base + '.png')
    pal = [fr.convert('P', palette=Image.ADAPTIVE, colors=255) for fr in frames]
    pal[0].save(out_base + '.gif', save_all=True, append_images=pal[1:], duration=int(1000 / FPS), loop=0, optimize=False, disposal=1)


if __name__ == '__main__':
    out = sys.argv[1] if len(sys.argv) > 1 else '.'
    os.makedirs(out, exist_ok=True)
    which = sys.argv[2] if len(sys.argv) > 2 else 'both'
    if which in ('both', 'base'):
        render_sheet([spec(r) for r in BASE], os.path.join(out, 'shots_base'), 5, 1.6)
    if which in ('both', 'upper'):
        render_sheet([spec(r) for r in UPPER], os.path.join(out, 'shots_upper'), 6, 1.3)
