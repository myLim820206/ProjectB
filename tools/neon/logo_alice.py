"""BAR ALICE 로고 선 정의 (D-309, D-311) — 글꼴 없이 손으로 그린 네온 튜브. 언리얼 PDLogo.h 와 같은 점

    python tools/neon/logo_alice.py 출력폴더   → logo_alice_A.png · logo_alice_B.png · logo_alice_badge.png

- BAR: 블록 글자를 속이 빈 외곽선으로 (점 목록 = 꺾은선), 예전 NEON 과 같은 크기 (너비 44, 높이 60)
- Alice: 필기체 한 줄 튜브. 점은 Catmull-Rom 으로 이어 부드럽게 (예전 Pallas 와 같은 방식)
- 부제 LATE NIGHT POKER: 작은 글자는 언리얼에서 Slate 글꼴 (튜브 아님)
- 마티니 잔 (올리브 대신 스페이드): 예전 스페이드 왕관 자리
좌표: 글자 기준선 y=0, 위가 음수. 글자마다 따로 (깜빡임을 글자별로)
"""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, PINK, CYAN, GOLD  # noqa: E402
from logo_strokes import catmull  # noqa: E402
from neon import suit, circle  # noqa: E402

GRY = (120, 125, 140)

BAR = {
    'B': [[(0, 0), (0, -60), (31, -60), (41, -52), (41, -39), (35, -31), (44, -23), (44, -9), (35, 0), (0, 0)],
          [(12, -49), (27, -49), (29, -46), (29, -40), (27, -37), (12, -37), (12, -49)],
          [(12, -25), (30, -25), (32, -22), (32, -14), (30, -11), (12, -11), (12, -25)]],
    'A': [[(0, 0), (16, -60), (28, -60), (44, 0), (32, 0), (29, -12), (15, -12), (12, 0), (0, 0)],
          [(18, -23), (26, -23), (22, -42), (18, -23)]],
    'R': [[(0, 0), (0, -60), (31, -60), (41, -51), (41, -35), (33, -27), (44, 0), (31, 0), (22, -24), (12, -24), (12, 0), (0, 0)],
          [(12, -49), (27, -49), (29, -46), (29, -38), (27, -35), (12, -35), (12, -49)]],
}
BAR_WORD = 'BAR'
BAR_ADV = 56

# Alice 필기체 (대문자 ~86, 소문자 ~40)
ALICE = [
    # A: 왼쪽 다리 올라가 꼭대기에서 오른쪽 다리로 내려와 l 로 이음 + 가로 획(살짝 휘게)
    [[(0, 4), (12, -24), (27, -58), (40, -82), (46, -82), (50, -60), (53, -30), (57, -8), (64, 0), (74, -6)],
     [(8, -30), (26, -38), (48, -38), (62, -32)]],
    # l
    [[(74, -6), (88, -34), (99, -68), (101, -86), (92, -90), (84, -70), (81, -32), (85, -5), (95, 0), (106, -8)]],
    # i (점은 따로)
    [[(106, -8), (112, -24), (117, -40), (118, -24), (118, -7), (124, 0), (132, -5)],
     [(116, -55), (119, -58), (122, -55), (119, -52), (116, -55)]],
    # c (위 고리에서 시작)
    [[(164, -33), (160, -40), (149, -42), (139, -29), (139, -9), (149, 0), (165, -8)]],
    # e
    [[(165, -8), (180, -19), (192, -31), (190, -41), (180, -42), (172, -30), (172, -10), (182, 0), (198, -3), (210, -12)]],
]
ALICE_W = 210


def martini(M, cx, cy, r, c, b=1.0):
    """마티니 잔: 역삼각 잔 + 다리 + 받침, 안에 스페이드 (올리브 자리)"""
    M.ln([(cx - r, cy - r * 0.9), (cx + r, cy - r * 0.9), (cx, cy + r * 0.15), (cx - r, cy - r * 0.9)], c, 2.2, b)
    M.ln([(cx, cy + r * 0.15), (cx, cy + r * 0.95)], c, 2.0, b)
    M.ln([(cx - r * 0.5, cy + r * 0.95), (cx + r * 0.5, cy + r * 0.95)], c, 2.0, b)
    for p in suit('spade')(cx + r * 0.1, cy - r * 0.48, r * 0.26): M.ln(p, PINK, 1.6, b)
    M.ln([(cx + r * 0.1, cy - r * 0.3), (cx + r * 0.62, cy - r * 1.25)], GRY, 1.2, b)  # 꼬치


def draw_bar(M, x0, y, k, nb=None, col=CYAN):
    for i, ch in enumerate(BAR_WORD):
        b = 1.0 if nb is None else nb[i]
        for poly in BAR[ch]:
            M.ln([(x0 + (i * BAR_ADV + px * 0.8) * k, y + (60 + py) * k) for px, py in poly], col, 1.8 * k, b)


def draw_alice(M, px0, by, k, pb=None, col=PINK):
    for i, letter in enumerate(ALICE):
        b = 1.0 if pb is None else pb[i]
        for st in letter:
            M.ln([(px0 + px * k, by + py * k) for px, py in catmull(st)], col, 3.0 * k, b)


def logo_A(M, cx, y, k=1.0):
    """A: 예전 배치 그대로 — 위 BAR(하늘 블록), 아래 Alice(분홍 필기체), 오른쪽 마티니 잔(금), 맨 아래 부제"""
    w = (len(BAR_WORD) - 1) * BAR_ADV + 44 * 0.8
    draw_bar(M, cx - w * k / 2, y, k)
    draw_alice(M, cx - ALICE_W * k / 2, y + 170 * k, k)
    martini(M, cx + 150 * k, y + 120 * k, 26 * k, GOLD)
    M.ln([(cx - 120 * k, y + 196 * k), (cx + 120 * k, y + 196 * k)], GOLD, 1.0, 0.5)
    M.word(cx, y + 222 * k, 'LATE NIGHT POKER', GOLD, 15 * k, None, track=6 * k, font=FONT)


def logo_B(M, cx, y, k=1.0):
    """B: Alice 를 크게 가운데, BAR 는 왼쪽 위 작은 알약 간판 안 (바 문 위 간판처럼), 부제는 아래 두 줄 사이"""
    draw_alice(M, cx - ALICE_W * 1.25 * k / 2, y + 150 * k, 1.25 * k)
    bx, byy, kb = cx - 190 * k, y - 6 * k, 0.55 * k
    M.rr(bx - 14 * kb, byy - 10 * kb, (2 * BAR_ADV + 44 * 0.8) * kb + 28 * kb, 80 * kb, 40 * kb, GOLD, 1.4, 1.0)
    draw_bar(M, bx, byy, kb, col=CYAN)
    martini(M, cx + 168 * k, y + 70 * k, 24 * k, GOLD)
    for dy in (180, 214):
        M.ln([(cx - 150 * k, y + dy * k), (cx + 150 * k, y + dy * k)], PINK, 1.0, 0.45)
    M.word(cx, y + 203 * k, 'LATE NIGHT POKER', CYAN, 15 * k, None, track=7 * k, font=FONT)


FONT = '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    for name, fn in (('A', logo_A), ('B', logo_B)):
        M = Sign()
        fn(M, 640, 160, 1.5)
        M.draw().save(os.path.join(outdir, 'logo_alice_%s.png' % name))
    # 인게임 왼쪽 위 배지 크기 (A 배치, 작게)
    M = Sign()
    logo_A(M, 640, 300, 0.5)
    M.draw().crop((440, 260, 840, 460)).resize((800, 400)).save(os.path.join(outdir, 'logo_alice_badge.png'))
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
