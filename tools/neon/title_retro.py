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


def neon_blocks(M, cx, y, size, c, gap=12, boxes=True, bright=None):
    w = size * 0.86
    x0 = cx - (4 * w + 3 * gap) / 2
    for i, ch in enumerate('NEON'):
        x = x0 + i * (w + gap)
        if boxes: M.rr(x, y, w, size, 7, c, 1.7)
        M.word(x + w / 2, y + size * 0.8, ch, c, size * 0.72, [bright[i]] if bright else None, font=BLOCK)


# ---------------------------------------------------------------- 1 극장 마키
def title_1(phase=0, pal=None, gk=1.0, frame=True, nb=None, pb=None, crown=1.0, spark=None, fx=None):
    P_ = pal or {}
    CYAN, PINK, GOLD, ORANGE, WHITE, DIMC, BULB = (P_.get(k, d) for k, d in (('cyan', globals()['CYAN']), ('pink', globals()['PINK']), ('gold', globals()['GOLD']), ('orange', globals()['ORANGE']), ('white', globals()['WHITE']), ('dim', globals()['DIMC']), ('bulb', (255, 225, 150))))
    M = Sign(gk)
    if frame:
        neon_blocks(M, 640, 40, 62, CYAN)
        M.rr(330, 118, 620, 150, 14, ORANGE, 2.4)
        bulbs(M, [(344, 132), (936, 132), (936, 254), (344, 254), (344, 132)], 24, phase, BULB)
    else:  # 간판 없이 글자만 (D-249): 칸 없는 NEON + 필기체 Pallas + 왕관 스페이드
        neon_blocks(M, 640, 50, 66, CYAN, gap=4, boxes=False, bright=nb)
    M.word(620, 228, 'Pallas', PINK, 96, pb, font=SCRIPT)
    spade_crown(M, 880, 195, 20, GOLD, crown)
    if spark: M.spark(*spark)
    if fx: fx(M)
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
    letters_anim(outdir)


def crackle(M, x, y, k, seed):
    """글자 옆 '빠직': k=0 작은 섬광 + 짧은 번개, 그 뒤 불티 몇 개가 튀어 떨어지며 꺼진다 (D-251)"""
    import random
    rnd = random.Random(seed)
    if k == 0:
        M.dots.append((x, y, 3.4))
        for _ in range(2):
            a = rnd.uniform(0, math.tau)
            p1 = (x + math.cos(a) * 7, y + math.sin(a) * 7)
            p2 = (p1[0] + math.cos(a + rnd.uniform(-1, 1)) * 8, p1[1] + math.sin(a + rnd.uniform(-1, 1)) * 8)
            M.ln([(x, y), p1, p2], (255, 245, 210), 1.4)
    if k == 1: M.dots.append((x, y, 2.0))
    for _ in range(8):
        a = rnd.uniform(-math.pi * 0.95, -math.pi * 0.05) if rnd.random() < 0.7 else rnd.uniform(0, math.pi)
        sp = rnd.uniform(2.4, 5.0)
        vx, vy = math.cos(a) * sp, math.sin(a) * sp
        life = rnd.randint(6, 11)
        if k < 1 or k > life: continue
        def at(t): return (x + vx * t, y + vy * t + 0.35 * t * t)
        M.ln([at(k - 1), at(k)], (255, 225, 150) if rnd.random() < 0.7 else (255, 255, 235), 1.5, 1.0 - (k / life) * 0.8)


def letters_anim(outdir):
    """글자만 시안의 움직임 (D-250, D-251): 켜지기 → NEON 한 글자 옆 빠직 + 그 글자 깜빡 → Pallas 한 글자 빠직 → 버튼 전구 돎. 25fps"""
    from PIL import Image, ImageFont
    # 글자 위치
    w = 66 * 0.86
    x0 = 640 - (4 * w + 3 * 4) / 2
    neon_x = [x0 + i * (w + 4) for i in range(4)]
    f = ImageFont.truetype(SCRIPT, 96)
    pw = [f.getlength(ch) for ch in 'Pallas']
    px0 = 620 - sum(pw) / 2
    pal_x = [px0 + sum(pw[:i]) for i in range(6)]
    T = []  # 프레임마다 (nb, pb, crown, fx)
    def add(n, nb=None, pb=None, crown=1.0, fx=None):
        for _ in range(n): T.append((nb, pb, crown, fx))
    OFF = 0.08
    add(10, [OFF] * 4, [OFF] * 6, 0.1)
    # 켜지기: 글자마다 지직거리며 들어온다
    flick = [0.5, OFF, 0.9, 0.3, 1.0]
    nb = [OFF] * 4
    for i in range(4):
        for v in flick:
            nb = nb[:]; nb[i] = v; add(1, nb, [OFF] * 6, 0.1)
        add(2, nb, [OFF] * 6, 0.1)
    for v in [0.4, OFF, OFF, 0.8, 0.5, 1.0]: add(1, [1] * 4, [v] * 6, 0.1)
    for v in [0.3, 0.6, 1.0]: add(1, [1] * 4, [1] * 6, v)
    add(38)
    # 빠직 1: NEON 의 E 오른쪽 위
    ex, ey = neon_x[1] + w * 0.86, 66
    pat = [0.1, 1, OFF, OFF, 0.7, OFF, 1, 1, 0.5, 1, 1, 1]
    for k, v in enumerate(pat):
        add(1, [1, v, 1, 1], None, 1.0, (lambda M, k=k: crackle(M, ex, ey, k, 3)))
    add(40)
    # 빠직 2: Pallas 의 두 번째 l 옆
    lx, ly = pal_x[3] + pw[3] * 0.9, 168
    pat = [0.1, 1, OFF, 0.6, OFF, OFF, 1, 0.4, 1, 1, 1, 1]
    for k, v in enumerate(pat):
        pb = [1] * 6; pb[3] = v
        add(1, None, pb, 1.0, (lambda M, k=k: crackle(M, lx, ly, k, 11)))
    add(34)
    frames = []
    for k, (nb, pb, crown, fx) in enumerate(T):
        frames.append(title_1((k // 5) % 3, frame=False, nb=nb, pb=pb, crown=crown, fx=fx).resize((960, 540), Image.LANCZOS).convert('P', palette=Image.ADAPTIVE, colors=128))
    frames[0].save(os.path.join(outdir, 'title_1_letters_anim.gif'), save_all=True, append_images=frames[1:], duration=40, loop=0)
    print('anim', len(frames))

if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
