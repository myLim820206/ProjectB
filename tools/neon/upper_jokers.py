"""상위 조커 30종 미리보기 (DOC/design/18-upper-jokers.md, 네온 화풍 DOC/art/15-art-style.md)

    python tools/neon/upper_jokers.py 출력.png

기본 조커와 같은 둥근 사각 틀 + 분홍 기호. 상위 조커 표시는 금색 이중 테두리 + 틀 위 작은 왕관.
기호는 이상한 나라·거울 나라 인물을 상징하는 단순한 선 그림 (재료 조커의 부분을 섞기도 한다).
"""
import math
import sys

import neon
from neon import BG, COL, KFONT, arc, bez, circle, jester_hat, rrect, suit
from PIL import Image, ImageChops, ImageDraw, ImageFilter

PINK, GOLD = COL['pink'], COL['gold']


def _P(cx, cy, R):
    return lambda x, y: (cx + x * R, cy + y * R)


def crown(P, x, y, w, h):
    return [P(x - w, y), P(x - w, y - h), P(x - w * 0.5, y - h * 0.5), P(x, y - h * 1.1), P(x + w * 0.5, y - h * 0.5), P(x + w, y - h), P(x + w, y), P(x - w, y)]


def ellipse(P, x, y, rx, ry, n=32, a0=0.0, a1=360.0):
    return [P(x + math.cos(math.radians(a0 + (a1 - a0) * k / n)) * rx, y + math.sin(math.radians(a0 + (a1 - a0) * k / n)) * ry) for k in range(n + 1)]


def arrow_arc(P, x, y, rx, ry, a0, a1, head=0.16, n=36):
    """a0 → a1 방향으로 도는 고리 + 끝 화살촉 (접선 방향)"""
    ring = ellipse(P, x, y, rx, ry, n, a0, a1)
    t = math.radians(a1)
    sgn = 1 if a1 > a0 else -1
    tx, ty = -math.sin(t) * rx * sgn, math.cos(t) * ry * sgn  # 진행 방향
    ln = math.hypot(tx, ty) or 1.0
    tx, ty = tx / ln, ty / ln
    ex, ey = x + math.cos(t) * rx, y + math.sin(t) * ry
    wing = lambda s2: P(ex - tx * head + (-ty) * head * 0.7 * s2, ey - ty * head + tx * head * 0.7 * s2)
    return [ring, [wing(1), P(ex, ey), wing(-1)]]


def zz(P, x0, x1, y, h, n):  # 지그재그
    return [P(x0 + (x1 - x0) * k / n, y + (h if k % 2 else 0)) for k in range(n + 1)]


def grin(P, y=0.0, w=0.6):
    g = bez(P(-w, y), P(0, y + 0.56 * w / 0.6), P(w, y)) + bez(P(w, y), P(0, y + 0.28 * w / 0.6), P(-w, y))[1:]
    teeth = [[P(x * w / 0.6, y + 0.11), P(x * w / 0.6, y + 0.25 - abs(x) * 0.22)] for x in (-0.3, -0.15, 0, 0.15, 0.3)]
    return [g] + teeth


def heart(P, x, y, r):
    return suit('heart')(*P(x, y), r)


def star(P, x, y, r, rot=-90):
    pts = []
    for k in range(11):
        a = math.radians(rot + k * 36)
        rr = r if k % 2 == 0 else r * 0.45
        pts.append(P(x + math.cos(a) * rr, y + math.sin(a) * rr))
    return [pts]


# ---------------------------------------------------------------- 30종 기호 (중심 cx, cy, 크기 R. 틀 안쪽은 대략 ±0.75)
def m1(cx, cy, R):  # 장미 칠하는 정원사: 장미 + 붓
    P = _P(cx, cy, R)
    spiral = [P(-0.1 + math.cos(t) * 0.04 * t, -0.15 + math.sin(t) * 0.04 * t) for t in [k * 0.35 for k in range(36)]]
    L = [spiral, ellipse(P, -0.1, -0.15, 0.5, 0.42, 30, 200, 340), ellipse(P, -0.1, -0.12, 0.5, 0.44, 30, 20, 160)]
    L += [[P(-0.1, 0.3), P(-0.1, 0.7)], bez(P(-0.1, 0.5), P(-0.35, 0.4), P(-0.4, 0.55), 8)]
    L += [[P(0.25, 0.65), P(0.62, 0.15)], [P(0.58, 0.1), P(0.7, 0.0), P(0.72, 0.18), P(0.62, 0.22), P(0.58, 0.1)]]
    return L


def m2(cx, cy, R):  # 트위들덤과 트위들디: 모자 쓴 둥근 얼굴 둘
    P = _P(cx, cy, R)
    L = []
    for sx in (-0.36, 0.36):
        L.append(circle(*P(sx, 0.22), 0.3 * R, 30))
        L += [circle(*P(sx - 0.1, 0.16), 0.03 * R, 8), circle(*P(sx + 0.1, 0.16), 0.03 * R, 8), bez(P(sx - 0.12, 0.3), P(sx, 0.4), P(sx + 0.12, 0.3), 8)]
        L += jester_hat(*P(sx, -0.12), 0.28 * R)
    return L


def m3(cx, cy, R):  # 도도새: 둥근 경주 화살 + 지휘봉
    P = _P(cx, cy, R)
    L = arrow_arc(P, 0, 0, 0.58, 0.5, -40, 250)
    L += [[P(-0.2, 0.32), P(0.2, -0.32)], circle(*P(0.22, -0.35), 0.07 * R, 14)]
    return L


def m4(cx, cy, R):  # 애벌레: 버섯 + 연기
    P = _P(cx, cy, R)
    L = [ellipse(P, 0, 0.05, 0.62, 0.42, 30, 180, 360) + [P(-0.62, 0.05)], [P(-0.18, 0.05), P(-0.2, 0.62), P(0.2, 0.62), P(0.18, 0.05)]]
    L += [circle(*P(-0.3, -0.12), 0.07 * R, 12), circle(*P(0.12, -0.24), 0.08 * R, 12), circle(*P(0.38, -0.06), 0.06 * R, 12)]
    L += [bez(P(0.25, -0.45), P(0.45, -0.6), P(0.3, -0.72), 10), bez(P(0.05, -0.5), P(-0.1, -0.62), P(0.05, -0.74), 10)]
    return L


def m5(cx, cy, R):  # 공작부인: 큰 모자 + 방울
    P = _P(cx, cy, R)
    L = [ellipse(P, 0, -0.32, 0.66, 0.16, 30), ellipse(P, 0, -0.38, 0.32, 0.3, 24, 180, 360)]
    bell = bez(P(-0.3, 0.5), P(-0.28, -0.05), P(0, -0.05)) + bez(P(0, -0.05), P(0.28, -0.05), P(0.3, 0.5))[1:]
    L += [bell, [P(-0.38, 0.5), P(0.38, 0.5)], circle(*P(0, 0.62), 0.06 * R, 12)]
    return L


def m6(cx, cy, R):  # 거울 나라: 손거울 + 별과 비친 별
    P = _P(cx, cy, R)
    L = [ellipse(P, 0, -0.12, 0.44, 0.56, 36), [P(0, 0.44), P(0, 0.74)], [P(-0.1, 0.74), P(0.1, 0.74)]]
    L += star(P, -0.12, -0.24, 0.2) + star(P, 0.12, 0.08, 0.14, 90)
    return L


def m7(cx, cy, R):  # 3월 토끼: 토끼 귀 + 눈가리개 가면
    P = _P(cx, cy, R)
    L = [ellipse(P, -0.2, -0.4, 0.11, 0.34, 24), ellipse(P, 0.2, -0.4, 0.11, 0.34, 24)]
    mask = bez(P(-0.6, 0.18), P(0, -0.12), P(0.6, 0.18)) + bez(P(0.6, 0.18), P(0.3, 0.46), P(0, 0.3))[1:] + bez(P(0, 0.3), P(-0.3, 0.46), P(-0.6, 0.18))[1:]
    L += [mask, ellipse(P, -0.24, 0.2, 0.1, 0.06, 12), ellipse(P, 0.24, 0.2, 0.1, 0.06, 12), bez(P(-0.18, 0.56), P(0, 0.7), P(0.18, 0.56), 8)]
    return L


def m8(cx, cy, R):  # 하트 왕: 왕관 + 하트 + 두루마리(재판)
    P = _P(cx, cy, R)
    L = [crown(P, 0, -0.22, 0.42, 0.36)] + heart(P, 0, 0.2, 0.24 * R)
    L += [[P(-0.5, 0.6), P(0.5, 0.6)], ellipse(P, -0.5, 0.6, 0.06, 0.08, 12), ellipse(P, 0.5, 0.6, 0.06, 0.08, 12)]
    return L


def m9(cx, cy, R):  # 하트 잭: J + 타르트
    P = _P(cx, cy, R)
    L = [('text', *P(-0.22, -0.12), 'J', 0.7 * R)]
    L += [ellipse(P, 0.3, 0.42, 0.34, 0.14, 24), ellipse(P, 0.3, 0.32, 0.34, 0.14, 24, 180, 360)]
    L += [[P(0.08, 0.36), P(0.3, 0.46)], [P(0.3, 0.3), P(0.52, 0.4)], [P(0.12, 0.44), P(0.34, 0.3)]]
    return L


def m10(cx, cy, R):  # 미친 다과회: 찻잔 + 김 + 모자
    P = _P(cx, cy, R)
    cup = [P(-0.42, 0.0)] + bez(P(-0.42, 0.0), P(-0.38, 0.5), P(0, 0.5), 10)[1:] + bez(P(0, 0.5), P(0.38, 0.5), P(0.42, 0.0), 10)[1:] + [P(-0.42, 0.0)]
    L = [cup, ellipse(P, 0.5, 0.2, 0.12, 0.14, 16, -90, 90), ellipse(P, 0, 0.62, 0.6, 0.08, 24)]
    L += [bez(P(-0.15, -0.08), P(-0.25, -0.25), P(-0.12, -0.4), 8), bez(P(0.12, -0.08), P(0.02, -0.25), P(0.15, -0.4), 8)]
    L += [[P(-0.3, -0.5), P(0.3, -0.5)], [P(-0.18, -0.5), P(-0.15, -0.78), P(0.15, -0.78), P(0.18, -0.5)]]
    return L


def m11(cx, cy, R):  # 흰 토끼: 회중시계 + 귀
    P = _P(cx, cy, R)
    L = [circle(*P(0, 0.12), 0.46 * R, 40), circle(*P(0, 0.12), 0.38 * R, 36), [P(0, 0.12), P(0, -0.14)], [P(0, 0.12), P(0.18, 0.22)]]
    L += [[P(-0.06, -0.34), P(-0.06, -0.42), P(0.06, -0.42), P(0.06, -0.34)], circle(*P(0, -0.48), 0.06 * R, 12)]
    L += [ellipse(P, -0.42, -0.42, 0.08, 0.26, 18), ellipse(P, 0.42, -0.42, 0.08, 0.26, 18)]
    return L


def m12(cx, cy, R):  # 험프티 덤프티: 달걀 + 가면 띠 + 담장
    P = _P(cx, cy, R)
    L = [ellipse(P, 0, -0.08, 0.36, 0.5, 36)]
    L += [bez(P(-0.36, -0.12), P(0, -0.26), P(0.36, -0.12), 10), bez(P(-0.36, -0.02), P(0, -0.16), P(0.36, -0.02), 10)]
    L += [bez(P(-0.14, 0.18), P(0, 0.28), P(0.14, 0.18), 8)]
    L += [[P(-0.66, 0.42), P(0.66, 0.42)], [P(-0.66, 0.68), P(0.66, 0.68)], [P(-0.3, 0.42), P(-0.3, 0.68)], [P(0.2, 0.42), P(0.2, 0.68)]]
    return L


def m13(cx, cy, R):  # 붉은 여왕: 큰 왕관 + 할리퀸 마름모
    P = _P(cx, cy, R)
    L = [crown(P, 0, -0.16, 0.5, 0.42), [P(0, 0.0), P(0.28, 0.34), P(0, 0.7), P(-0.28, 0.34), P(0, 0.0)]]
    L += [circle(*P(-0.5, -0.66), 0.05 * R, 10), circle(*P(0, -0.7), 0.05 * R, 10), circle(*P(0.5, -0.66), 0.05 * R, 10)]
    return L


def m14(cx, cy, R):  # 가짜 거북: 등딱지 + 눈물
    P = _P(cx, cy, R)
    L = [ellipse(P, 0, 0.3, 0.56, 0.5, 30, 180, 360) + [P(-0.56, 0.3)]]
    hexs = [P(0.16 * math.cos(math.radians(60 * k)), 0.06 + 0.14 * math.sin(math.radians(60 * k))) for k in range(7)]
    L += [hexs, [P(-0.16, 0.06), P(-0.42, 0.06)], [P(0.16, 0.06), P(0.42, 0.06)], [P(0, -0.08), P(0, -0.18)]]
    L += [circle(*P(0.7, 0.18), 0.12 * R, 16), [P(-0.36, 0.3), P(-0.42, 0.5)], [P(0.36, 0.3), P(0.42, 0.5)]]
    L += [[P(0.72, 0.36), P(0.66, 0.5), P(0.72, 0.56), P(0.78, 0.5), P(0.72, 0.36)]]
    return L


def m15(cx, cy, R):  # 그리핀: 펼친 날개 둘 + 부리
    P = _P(cx, cy, R)
    L = []
    for s in (-1, 1):
        for k in range(4):
            L.append(bez(P(s * 0.12, -0.1 + k * 0.1), P(s * (0.4 + k * 0.05), -0.55 + k * 0.12), P(s * (0.72 - k * 0.08), -0.2 + k * 0.18), 10))
    L += [circle(*P(0, 0.12), 0.16 * R, 20), [P(0.1, 0.2), P(0.0, 0.42), P(-0.1, 0.2)]]
    return L


def m16(cx, cy, R):  # 웃음만 남은 고양이: 미소 + 눈 + 사라지는 머리(점선)
    P = _P(cx, cy, R)
    head = ellipse(P, 0, 0.0, 0.66, 0.58, 48)
    L = [head[k:k + 3] for k in range(0, 48, 6)]
    L += grin(P, 0.06, 0.52)
    L += [bez(P(-0.4, -0.24), P(-0.27, -0.38), P(-0.14, -0.24), 8), bez(P(0.14, -0.24), P(0.27, -0.38), P(0.4, -0.24), 8)]
    return L


def m17(cx, cy, R):  # 하얀 여왕: 왕관 + 거꾸로 도는 화살 (거꾸로 사는 여왕)
    P = _P(cx, cy, R)
    L = [crown(P, 0, -0.24, 0.42, 0.36)]
    L += arrow_arc(P, 0, 0.32, 0.36, 0.3, 330, 30, 0.14)  # 거꾸로(시계 반대) 도는 화살
    return L


def m18(cx, cy, R):  # 바다코끼리와 목수: 수염 + 엄니 + 목수 모자
    P = _P(cx, cy, R)
    L = [bez(P(0, 0.08), P(-0.3, -0.04), P(-0.56, 0.18), 10), bez(P(0, 0.08), P(0.3, -0.04), P(0.56, 0.18), 10)]
    L += [[P(-0.14, 0.16), P(-0.18, 0.62)], [P(0.14, 0.16), P(0.18, 0.62)], circle(*P(-0.22, -0.22), 0.05 * R, 10), circle(*P(0.22, -0.22), 0.05 * R, 10)]
    L += [[P(-0.36, -0.42), P(0.36, -0.42)], [P(-0.22, -0.42), P(-0.22, -0.66), P(0.22, -0.66), P(0.22, -0.42)]]
    return L


def m19(cx, cy, R):  # 하얀 기사: 투구 + 깃털
    P = _P(cx, cy, R)
    helm = [P(-0.36, 0.6), P(-0.36, 0.0)] + bez(P(-0.36, 0.0), P(-0.36, -0.42), P(0, -0.42), 10)[1:] + bez(P(0, -0.42), P(0.36, -0.42), P(0.36, 0.0), 10)[1:] + [P(0.36, 0.6), P(-0.36, 0.6)]
    L = [helm, [P(-0.26, 0.1), P(0.26, 0.1)], [P(0, 0.1), P(0, 0.5)]]
    L += [bez(P(0, -0.42), P(0.3, -0.7), P(0.6, -0.5), 10), bez(P(0.05, -0.42), P(0.3, -0.58), P(0.55, -0.38), 10)]
    return L


def m20(cx, cy, R):  # 말하는 꽃들: 꽃잎 여섯 + 웃는 얼굴
    P = _P(cx, cy, R)
    L = [circle(*P(0.38 * math.cos(math.radians(60 * k - 90)), -0.08 + 0.38 * math.sin(math.radians(60 * k - 90))), 0.17 * R, 18) for k in range(6)]
    L += [circle(*P(0, -0.08), 0.2 * R, 20), bez(P(-0.1, -0.04), P(0, 0.06), P(0.1, -0.04), 6)]
    L += [[P(0, 0.32), P(0, 0.72)], bez(P(0, 0.56), P(0.24, 0.42), P(0.32, 0.56), 8)]
    return L


def m21(cx, cy, R):  # 하트 여왕: 왕관 쓴 하트 + 도끼 ("목을 쳐라")
    P = _P(cx, cy, R)
    L = heart(P, -0.12, 0.18, 0.34 * R) + [crown(P, -0.12, -0.26, 0.26, 0.26)]
    L += [[P(0.46, -0.5), P(0.46, 0.7)], bez(P(0.46, -0.42), P(0.78, -0.4), P(0.74, -0.08), 10) + [P(0.46, -0.1)]]
    return L


def m22(cx, cy, R):  # 홍학 크로케: 홍학 목 + 공
    P = _P(cx, cy, R)
    neck = bez(P(0.0, 0.66), P(-0.5, 0.2), P(0.0, -0.1), 14) + bez(P(0.0, -0.1), P(0.4, -0.38), P(0.0, -0.58), 14)[1:]
    L = [neck, circle(*P(-0.06, -0.6), 0.08 * R, 14), [P(-0.14, -0.6), P(-0.32, -0.5), P(-0.16, -0.54)]]
    L += [circle(*P(0.42, 0.52), 0.16 * R, 18), [P(-0.6, 0.7), P(0.66, 0.7)]]
    return L


def m23(cx, cy, R):  # 트럼프 근위대: 창 + 카드 + 별
    P = _P(cx, cy, R)
    L = [[P(-0.42, 0.72), P(-0.42, -0.5)], [P(-0.52, -0.42), P(-0.42, -0.72), P(-0.32, -0.42), P(-0.52, -0.42)]]
    L += [rrect(*P(0.14, 0.14), 0.36 * R, 0.08 * R)] + star(P, 0.14, 0.14, 0.2)
    return L


def m24(cx, cy, R):  # 체스판: 3×3 칸 + 졸
    P = _P(cx, cy, R)
    L = []
    for k in range(4):
        t = -0.5 + k / 3
        L += [[P(-0.5, t + 0.12), P(0.5, t + 0.12)], [P(t, -0.38), P(t, 0.62)]]
    L += [circle(*P(1 / 6, -1 / 6 + 0.04), 0.08 * R, 12), [P(0.07, 0.1), P(1 / 6, -0.08), P(0.26, 0.1), P(0.07, 0.1)]]
    return L


def m25(cx, cy, R):  # 겨울잠쥐: 웅크린 쥐 + z
    P = _P(cx, cy, R)
    L = [ellipse(P, -0.05, 0.28, 0.48, 0.32, 32), circle(*P(0.3, 0.02), 0.12 * R, 14), circle(*P(0.36, 0.22), 0.03 * R, 6)]
    L += [bez(P(-0.52, 0.32), P(-0.75, 0.62), P(-0.4, 0.62), 10)]
    L += [('text', *P(0.3, -0.36), 'z', 0.3 * R), ('text', *P(0.5, -0.56), 'z', 0.22 * R)]
    return L


def m26(cx, cy, R):  # 유니콘: 꼬인 뿔 + 별
    P = _P(cx, cy, R)
    L = [[P(-0.3, 0.62), P(0.0, -0.68), P(0.3, 0.62), P(-0.3, 0.62)]]
    L += [[P(-0.22 + k * 0.03, 0.34 - k * 0.24), P(0.2 - k * 0.04, 0.24 - k * 0.24)] for k in range(4)]
    L += star(P, 0.5, -0.4, 0.14)
    return L


def m27(cx, cy, R):  # 재버워키: 빛나는 두 눈 + 톱니 이빨 턱
    P = _P(cx, cy, R)
    L = [[P(-0.5, -0.4), P(-0.12, -0.24)], [P(0.5, -0.4), P(0.12, -0.24)], circle(*P(-0.28, -0.14), 0.08 * R, 14), circle(*P(0.28, -0.14), 0.08 * R, 14)]
    L += [zz(P, -0.56, 0.56, 0.18, 0.16, 8), zz(P, -0.5, 0.5, 0.5, -0.16, 8)]
    L += [bez(P(-0.56, 0.18), P(-0.7, 0.4), P(-0.5, 0.5), 8), bez(P(0.56, 0.18), P(0.7, 0.4), P(0.5, 0.5), 8)]
    return L


def m28(cx, cy, R):  # 사자: 갈기 + 얼굴
    P = _P(cx, cy, R)
    mane = [P(0.62 * math.cos(math.radians(k * 15)) * (1 if k % 2 == 0 else 0.82), 0.04 + 0.62 * math.sin(math.radians(k * 15)) * (1 if k % 2 == 0 else 0.82)) for k in range(25)]
    L = [mane, circle(*P(0, 0.04), 0.34 * R, 30), circle(*P(-0.12, -0.04), 0.04 * R, 8), circle(*P(0.12, -0.04), 0.04 * R, 8)]
    L += [[P(-0.06, 0.08), P(0.06, 0.08), P(0, 0.16), P(-0.06, 0.08)], bez(P(-0.12, 0.22), P(0, 0.3), P(0.12, 0.22), 8)]
    return L


def m29(cx, cy, R):  # 꿈꾸는 붉은 왕: 초승달 + 기운 왕관 + 별
    P = _P(cx, cy, R)
    moon = ellipse(P, -0.1, 0.12, 0.46, 0.46, 30, 60, 300) + list(reversed(ellipse(P, 0.06, 0.06, 0.36, 0.38, 30, 70, 290)))
    L = [moon + [moon[0]]]
    c = crown(P, 0.3, -0.36, 0.2, 0.2)
    ang = math.radians(18)
    ox, oy = P(0.3, -0.46)
    L.append([(ox + (x - ox) * math.cos(ang) - (y - oy) * math.sin(ang), oy + (x - ox) * math.sin(ang) + (y - oy) * math.cos(ang)) for x, y in c])
    L += star(P, 0.5, 0.3, 0.1) + star(P, 0.28, 0.56, 0.07)
    return L


def m30(cx, cy, R):  # 반짝반짝 작은 박쥐: 박쥐 날개 + 별
    P = _P(cx, cy, R)
    wing = [P(0, 0.12), P(-0.2, -0.12), P(-0.66, -0.2), P(-0.56, 0.06), P(-0.42, 0.0), P(-0.36, 0.2), P(-0.2, 0.12), P(0, 0.32),
            P(0.2, 0.12), P(0.36, 0.2), P(0.42, 0.0), P(0.56, 0.06), P(0.66, -0.2), P(0.2, -0.12), P(0, 0.12)]
    L = [wing, [P(-0.06, -0.04), P(-0.1, -0.16)], [P(0.06, -0.04), P(0.1, -0.16)]]
    L += star(P, 0, -0.5, 0.18) + star(P, -0.46, 0.5, 0.08) + star(P, 0.46, 0.52, 0.08)
    return L


UPPER = [  # (기호, 이름, 재료) DOC/design/18 4절 순서
    (m1, '장미 칠하는 정원사', '광대 + 모자 장수'), (m2, '★ 트위들덤과 트위들디', '광대 + 희비극'), (m3, '도도새', '광대 + 지휘봉'),
    (m4, '애벌레', '광대 + 체셔'), (m5, '공작부인', '광대 + 방울'), (m6, '★ 거울 나라', '광대 + 스타'),
    (m7, '3월 토끼', '모자 장수 + 웃는 가면'), (m8, '하트 왕', '모자 장수 + 할리퀸'), (m9, '하트 잭', '모자 장수 + 잭'),
    (m10, '★ 미친 다과회', '모자 장수 + 체셔'), (m11, '★ 흰 토끼', '모자 장수 + 방울'), (m12, '험프티 덤프티', '웃는 가면 + 희비극'),
    (m13, '★ 붉은 여왕', '웃는 가면 + 할리퀸'), (m14, '★ 가짜 거북', '웃는 가면 + 지휘봉'), (m15, '그리핀', '웃는 가면 + 잭'),
    (m16, '★ 웃음만 남은 고양이', '웃는 가면 + 체셔'), (m17, '하얀 여왕', '희비극 + 할리퀸'), (m18, '★ 바다코끼리와 목수', '희비극 + 지휘봉'),
    (m19, '하얀 기사', '희비극 + 잭'), (m20, '말하는 꽃들', '희비극 + 스타'), (m21, '★ 하트 여왕', '할리퀸 + 잭'),
    (m22, '홍학 크로케', '할리퀸 + 방울'), (m23, '트럼프 근위대', '할리퀸 + 스타'), (m24, '체스판', '지휘봉 + 체셔'),
    (m25, '★ 겨울잠쥐', '지휘봉 + 방울'), (m26, '유니콘', '지휘봉 + 스타'), (m27, '★ 재버워키', '잭 + 체셔'),
    (m28, '사자', '잭 + 방울'), (m29, '꿈꾸는 붉은 왕', '체셔 + 스타'), (m30, '반짝반짝 작은 박쥐', '방울 + 스타'),
]


def upper_frame(cx, cy, R):
    """상위 조커 표시: 이중 테두리 + 틀 위 작은 왕관 (금색)"""
    P = _P(cx, cy, R)
    return [rrect(cx, cy, R * 0.98, R * 0.22), rrect(cx, cy, R * 0.88, R * 0.17), crown(P, 0, -0.9, 0.2, 0.18)]


def _draw(img, lines, c, core, S, blur=7):
    glow = Image.new('RGB', img.size, (0, 0, 0))
    gd = ImageDraw.Draw(glow)
    for ln in lines:
        if ln and ln[0] == 'text':
            gd.text((ln[1], ln[2]), ln[3], fill=c, font=KFONT(ln[4]), anchor='mm')
        else:
            gd.line(ln, fill=c, width=core * 4, joint='curve')
    img = ImageChops.add(img, glow.filter(ImageFilter.GaussianBlur(blur * S)).point(lambda v: int(v * 0.9)))
    d = ImageDraw.Draw(img)
    for ln in lines:
        if ln and ln[0] == 'text':
            d.text((ln[1], ln[2]), ln[3], fill=c, font=KFONT(ln[4]), anchor='mm')
        else:
            d.line(ln, fill=c, width=core, joint='curve')
    return img


def render(out, S=3):
    cols, cw, ch, R = 6, 200, 236, 58
    rows = (len(UPPER) + cols - 1) // cols
    W, H = cols * cw, rows * ch + 40
    img = Image.new('RGB', (W * S, H * S), BG)
    core = max(2, int(round(1.6 * R / 60 * S)))
    for k, (fn, name, parts) in enumerate(UPPER):
        cx, cy = (k % cols) * cw + cw / 2, 40 + (k // cols) * ch + 84
        img = _draw(img, upper_frame(cx * S, cy * S, R * S), GOLD, core, S, blur=5)
        img = _draw(img, fn(cx * S, cy * S, R * S), PINK, core, S)
        d = ImageDraw.Draw(img)
        d.text((cx * S, (cy + R + 22) * S), f'{k + 1}. {name}', fill=GOLD if name.startswith('★') else (207, 216, 230), font=KFONT(11 * S), anchor='mm')
        d.text((cx * S, (cy + R + 40) * S), parts, fill=(138, 148, 166), font=KFONT(9 * S), anchor='mm')
    d = ImageDraw.Draw(img)
    d.text((W / 2 * S, 20 * S), '상위 조커 30종 (금색 이중 테두리 + 왕관 = 상위, ★ = 대표 조커)', fill=(138, 148, 166), font=KFONT(11 * S), anchor='mm')
    img.resize((W, H), Image.LANCZOS).save(out)


if __name__ == '__main__':
    render(sys.argv[1] if len(sys.argv) > 1 else 'upper_jokers.png')
    print('saved', sys.argv[1] if len(sys.argv) > 1 else 'upper_jokers.png')
