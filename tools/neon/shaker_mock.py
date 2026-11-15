"""칵테일 보상 화면: 잔 위에서 따르는 그림을 코블러 셰이커로 (시안). 지금은 네모 병 + 목이라 셰이커로 안 보인다

    python tools/neon/shaker_mock.py 출력폴더   → shaker_mock.png

코블러 셰이커 = 둥근 뚜껑 + 거름망 어깨(둥글게 좁아짐) + 아래로 살짝 좁아지는 몸통 + 이음 띠
A 따르기: 뚜껑을 열어 선반에 두고, 셰이커를 기울여 거름망으로 술을 따른다
B 흔들기: 잔 위에서 셰이커를 세워 흔든다 (흔들림 선 + 물방울), 고르면 따르기로
C 작은 아이콘: 뚜껑 닫은 채 기울인 단순한 모양 (지금 병 자리 크기 그대로)
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, PINK, GOLD, WHITE  # noqa: E402
from logo_retro import KO  # noqa: E402
from reward_mock import glass, LAV, FG, DIM  # noqa: E402

STEEL = (200, 214, 235)


def arc(cx, cy, rx, ry, a0, a1, n=10):
    return [(cx + math.cos(math.radians(a0 + (a1 - a0) * k / n)) * rx, cy + math.sin(math.radians(a0 + (a1 - a0) * k / n)) * ry) for k in range(n + 1)]


def shaker_parts(cap=True, s=1.0):
    """셰이커 선 (바닥 가운데 원점, 위 = 음수). 반환: [(점들, 굵기)]"""
    body = [(-21, -62), (-15, 0), (15, 0), (21, -62)]
    shoulder = arc(0, -62, 21, 20, 180, 270, 6)[:-1] + [(-12, -81)]
    shoulder = [(-21, -62)] + [(-21 + (9) * (1 - math.cos(math.radians(k * 15))), -62 - 19 * math.sin(math.radians(k * 15))) for k in range(1, 7)]
    sh_r = [(-x, y) for x, y in shoulder]
    neck = [(-12, -81), (-11, -87), (11, -87), (12, -81)]
    parts = [(body + [(-21, -62)], 1.8), (shoulder, 1.8), (sh_r, 1.8), (neck, 1.6),
             ([(-21, -58), (21, -58)], 1.0)]  # 이음 띠
    if cap:
        parts.append(([(-10, -87)] + arc(0, -96, 10, 10, 180, 360, 10) + [(10, -87)], 1.6))
        parts.append(([(-3, -106), (3, -106)], 1.2))
    parts = [([(x * s, y * s) for x, y in P], w * s ** 0.5) for P, w in parts]
    return parts


def put(M, parts, x, y, ang, c, b=1.0):
    ca, sa = math.cos(math.radians(ang)), math.sin(math.radians(ang))
    for P, w in parts:
        M.ln([(x + px * ca - py * sa, y + px * sa + py * ca) for px, py in P], c, w, b)


def rot(px, py, x, y, ang):
    ca, sa = math.cos(math.radians(ang)), math.sin(math.radians(ang))
    return (x + px * ca - py * sa, y + px * sa + py * ca)


def panel(M, cx, title, sub):
    M.rr(cx - 190, 110, 380, 470, 14, LAV, 1.4, 0.55)
    M.word(cx, 150, title, GOLD, 22, font=KO)
    M.word(cx, 176, sub, FG, 13, font=KO)
    M.ln([(cx - 160, 470), (cx + 160, 470)], LAV, 2.0, 0.6)  # 선반


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    M = Sign()
    M.small_glow = 0.3
    by = 468
    # A 따르기
    cx = 220
    panel(M, cx, 'A 따르기', '뚜껑 열고 거름망으로')
    glass(M, 'highball', cx, by, PINK, 1.0, 1.0)
    sx, sy, ang = cx + 110, by - 170, -128
    put(M, shaker_parts(cap=False), sx, sy, ang, STEEL)
    lip = rot(0, -87, sx, sy, ang)
    M.ln([lip, (lip[0] - 6, lip[1] + 20), (cx + 1, by - 70)], PINK, 1.8)  # 술줄기
    put(M, shaker_parts(cap=True)[5:], cx + 120, by + 87, 0, STEEL, 0.8)  # 선반 위 뚜껑
    # B 흔들기
    cx = 640
    panel(M, cx, 'B 흔들기', '마우스 올리면 흔들고, 고르면 따른다')
    glass(M, 'martini', cx, by, (232, 238, 255), 1.0, 0.0)
    sx, sy = cx, by - 120
    put(M, shaker_parts(), sx + 2, sy, -14, STEEL)
    for k, (dx, dy, l) in enumerate(((-40, -70, 14), (-44, -50, 18), (36, -96, 14), (42, -76, 18))):
        M.ln([(sx + dx, sy + dy), (sx + dx + (-8 if dx < 0 else 8), sy + dy + l)], STEEL, 1.0, 0.6)  # 흔들림 선
    for (dx, dy) in ((-30, -112), (26, -118), (-18, -122)):
        M.ln([(sx + dx, sy + dy), (sx + dx + 1, sy + dy - 3)], WHITE, 1.4, 0.7)  # 물방울
    # C 작은 아이콘
    cx = 1060
    panel(M, cx, 'C 작은 셰이커', '지금 병 자리 크기, 뚜껑 닫힘')
    glass(M, 'rocks', cx, by, GOLD, 1.0, 1.0)
    sx, sy, ang = cx + 70, by - 125, -122
    put(M, shaker_parts(cap=True, s=0.62), sx, sy, ang, STEEL)
    lip = rot(0, -87 * 0.62, sx, sy, ang)
    M.ln([lip, (cx + 1, by - 40)], GOLD, 1.6)
    M.word(640, 640, '지금 = 기운 네모 병 + 목 (셰이커로 안 보임) → 코블러 셰이커: 둥근 뚜껑 · 거름망 어깨 · 아래로 좁아지는 몸통 · 이음 띠', DIM, 12, font=KO)
    M.draw().save(os.path.join(outdir, 'shaker_mock.png'))
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
