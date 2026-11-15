"""NEON PALLAS 제목 화면 시안 — A 폰트(블록 NEON + 필기체 Pallas) + 다른 간판 + 아래 버튼 6개

    python tools/neon/title_retro.py 출력폴더

1 극장 마키: 전구 두른 사각 간판, 위에 NEON 칸 글자 → 버튼은 작은 마키 판 (고른 버튼만 전구가 켜짐)
2 둥근 바 간판: 전구 두른 원 + 위 호를 따라 NEON, 가운데 필기체 → 버튼은 꺼진 네온 튜브 (고른 버튼만 켜짐)
3 스페이드 간판: 스페이드 모양 틀 안 필기체, 위 NEON → 버튼은 카드 모양 (고른 버튼은 앞으로 기울어 켜짐)
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, spade_crown, PINK, CYAN, GOLD, WHITE  # noqa: E402
from logo_retro import bulbs, ORANGE, RED, SCRIPT, BLOCK, KO  # noqa: E402
from neon import suit, circle, arc  # noqa: E402

MENU = ['이어하기', '게임 시작', '영구 강화', '잠금 해제', '환경 설정', '게임 종료']
DIMC = (95, 105, 125)
SEL = 1


def chip(M, x, y, txt, c):
    w = 40 + len(txt) * 11
    M.rr(x, y, w, 30, 15, c, 1.4)
    for p in suit('diamond')(x + 16, y + 15, 6): M.ln(p, c, 1.2)
    M.word(x + 30, y + 21, txt, c, 14, None, font=KO, anchor_center=False)


def neon_blocks(M, cx, y, size, c, gap=12, boxes=True):
    w = size * 0.86
    x0 = cx - (4 * w + 3 * gap) / 2
    for i, ch in enumerate('NEON'):
        x = x0 + i * (w + gap)
        if boxes: M.rr(x, y, w, size, 7, c, 1.7)
        M.word(x + w / 2, y + size * 0.8, ch, c, size * 0.72, font=BLOCK)


# ---------------------------------------------------------------- 1 극장 마키
def title_1(phase=0, pal=None, gk=1.0, frame=True):
    P_ = pal or {}
    CYAN, PINK, GOLD, ORANGE, WHITE, DIMC, BULB = (P_.get(k, d) for k, d in (('cyan', globals()['CYAN']), ('pink', globals()['PINK']), ('gold', globals()['GOLD']), ('orange', globals()['ORANGE']), ('white', globals()['WHITE']), ('dim', globals()['DIMC']), ('bulb', (255, 225, 150))))
    M = Sign(gk)
    if frame:
        neon_blocks(M, 640, 40, 62, CYAN)
        M.rr(330, 118, 620, 150, 14, ORANGE, 2.4)
        bulbs(M, [(344, 132), (936, 132), (936, 254), (344, 254), (344, 132)], 24, phase, BULB)
    else:  # 간판 없이 글자만 (D-249): 칸 없는 NEON + 필기체 Pallas + 왕관 스페이드
        neon_blocks(M, 640, 50, 66, CYAN, gap=4, boxes=False)
    M.word(620, 228, 'Pallas', PINK, 96, None, font=SCRIPT)
    spade_crown(M, 880, 195, 20, GOLD)
    chip(M, 1080, 24, '칩 1,240', GOLD)
    for i, n in enumerate(MENU):
        y = 330 + i * 52
        x0 = 640 - 150
        on = i == SEL
        c = ORANGE if on else DIMC
        M.rr(x0, y, 300, 42, 6, c, 2.0 if on else 1.2, 1.0 if on else 0.55)
        if on:
            for bx in (x0 - 14, x0 + 314): bulbs(M, [(bx, y + 4), (bx, y + 40)], 12, phase, BULB)  # 고른 버튼 양옆 전구 기둥
        M.word(640, y + 29, n, WHITE if on else DIMC, 18, [1.0 if on else 0.6] * len(n), font=KO)
    M.word(640, 690, '최고 기록 42웨이브', DIMC, 12, None, font=KO)
    return M.draw()


# ---------------------------------------------------------------- 2 둥근 바 간판
def title_2(phase=0):
    M = Sign()
    cx, cy, R = 640, 170, 140
    M.ln(circle(cx, cy, R, 72), ORANGE, 2.4)
    bulbs(M, circle(cx, cy, R - 14, 72), 22, phase)
    M.ln(circle(cx, cy, R - 30, 72), RED, 1.0, 0.6)
    # NEON 을 위 호를 따라 (글자마다 위치)
    for i, ch in enumerate('NEON'):
        a = math.radians(-128 + i * 25.5)
        M.word(cx + math.cos(a) * (R - 58), cy + math.sin(a) * (R - 58) + 14, ch, CYAN, 34, font=BLOCK)
    M.word(cx, cy + 34, 'Pallas', PINK, 78, None, font=SCRIPT)
    M.ln([(cx - 90, cy + 56), (cx + 90, cy + 56)], CYAN, 1.6)
    spade_crown(M, cx, cy + 92, 14, GOLD)
    chip(M, 1080, 24, '칩 1,240', GOLD)
    for i, n in enumerate(MENU):
        y = 362 + i * 50
        x0 = 640 - 160
        on = i == SEL
        c = PINK if on else CYAN
        b = 1.0 if on else 0.22  # 꺼진 튜브: 흐린 선만
        M.rr(x0, y, 320, 40, 20, c, 2.2 if on else 1.2, b)
        M.word(640, y + 28, n, WHITE if on else c, 18, [1.0 if on else 0.45] * len(n), font=KO)
        if on:
            for s in (-1, 1):
                for p in suit('heart')(640 + s * 190, y + 20, 8): M.ln(p, PINK, 1.4)
    M.word(640, 690, '최고 기록 42웨이브', DIMC, 12, None, font=KO)
    return M.draw()


# ---------------------------------------------------------------- 3 스페이드 간판
def title_3(phase=0):
    M = Sign()
    neon_blocks(M, 640, 26, 52, CYAN, gap=10)
    # 스페이드 틀 (크게, 납작하게)
    sp = suit('spade')(640, 190, 170)
    for p in sp:
        q = [(640 + (x - 640) * 1.9, 190 + (y - 190) * 0.62) for x, y in p]
        M.ln(q, ORANGE, 2.4)
    M.word(640, 214, 'Pallas', PINK, 96, None, font=SCRIPT)
    chip(M, 1080, 24, '칩 1,240', GOLD)
    suits = ['spade', 'heart', 'diamond', 'club', 'spade', 'heart']
    cols = [CYAN, PINK, GOLD, (138, 255, 196), CYAN, PINK]
    for i, n in enumerate(MENU):
        row, col = i // 2, i % 2
        x0 = 640 - 290 + col * 300
        y = 348 + row * 104
        on = i == SEL
        c = cols[i]
        b = 1.0 if on else 0.4
        M.rr(x0, y, 280, 88, 12, c, 2.2 if on else 1.3, b)
        M.rr(x0 + 10, y + 10, 30, 40, 5, c, 1.2, b)
        for p in suit(suits[i])(x0 + 25, y + 30, 8): M.ln(p, c, 1.2, b)
        M.word(x0 + 160, y + 54, n, WHITE if on else c, 22, [1.0 if on else 0.55] * len(n), font=KO)
    M.word(640, 696, '최고 기록 42웨이브', DIMC, 12, None, font=KO)
    return M.draw()


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    for name, fn in (('title_1_marquee', title_1), ('title_2_round', title_2), ('title_3_spade', title_3)):
        fn().save(os.path.join(outdir, name + '.png'))
        print(name)
    # 1번 색감 죽이기 (D-248): 조금 / 많이
    soft = {'cyan': (125, 185, 205), 'pink': (225, 125, 165), 'gold': (225, 195, 130), 'orange': (215, 150, 95), 'white': (230, 225, 215), 'dim': (85, 90, 105), 'bulb': (230, 205, 150)}
    vint = {'cyan': (110, 150, 160), 'pink': (190, 115, 135), 'gold': (195, 170, 120), 'orange': (185, 130, 90), 'white': (210, 200, 185), 'dim': (75, 78, 88), 'bulb': (200, 175, 130)}
    title_1(0, soft, 0.6).save(os.path.join(outdir, 'title_1_soft.png'))
    title_1(0, vint, 0.38).save(os.path.join(outdir, 'title_1_vintage.png'))
    print('muted')
    title_1(0, frame=False).save(os.path.join(outdir, 'title_1_letters.png'))
    print('letters')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
