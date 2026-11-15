"""레벨업 이펙트 시안 (D-271) — 위쪽 띠의 레벨 글자에서 터지는 움직임 GIF 3종 (25fps, 화면 오른쪽 위 640×450 잘라냄)

    python tools/neon/levelup_mock.py 출력폴더

A 네온 간판 점등: 경험치 막대가 차서 하얗게 번쩍 → 'LEVEL UP' 금색 간판이 지직거리며 켜지고 빠직 불티 → 숫자 7 → 8 → '포인트 +1' 분홍 꼬리표 → 꺼짐
B 잭팟: 레벨 글자 둘레에 전구가 돌고 숫자가 슬롯처럼 굴러 8 에 멈춤 → 금빛 칩이 쏟아짐 → 'LEVEL 8' 금 글자
C 파동 + 안내: 레벨 글자에서 두 겹 고리가 퍼지고 화면 테두리가 금빛으로 한 번 숨쉼 → '+1' 이 스킬 트리 버튼으로 날아가 꽂히고 버튼이 켜짐 (포인트를 쓰러 가게)
"""
import math
import os
import random
import sys

from PIL import Image

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, PINK, CYAN, GOLD, WHITE  # noqa: E402
from logo_retro import KO, BLOCK, bulbs  # noqa: E402
from neon import circle  # noqa: E402

LAV = (168, 150, 225)
FG = (207, 216, 230)
DIM = (95, 104, 120)
CROP = (640, 0, 1280, 450)
FPS = 25


def ease_out(u): return 1 - (1 - min(1, max(0, u))) ** 3


def hud(M, lv, xp_fill, xp_flash=0.0, tree_b=0.8, tree_pt=0):
    """위쪽 띠 + 오른쪽 열 일부 (지금 게임 자리)"""
    M.rr(16, 12, 1248, 50, 12, LAV, 1.4, 0.55)
    M.word(860, 44, '목숨 18', FG, 17, None, font=KO, anchor_center=False)
    M.word(980, 36, f'레벨 {lv}', FG, 17, None, font=KO, anchor_center=False)
    # 경험치 막대
    M.ln([(980, 47), (1120, 47)], CYAN, 4.0, 0.15)
    if xp_fill > 0: M.ln([(980, 47), (980 + 140 * xp_fill, 47)], CYAN if xp_flash <= 0 else WHITE, 4.0, 0.6 + 0.4 * xp_flash)
    M.word(980, 59, f'{int(xp_fill * 230)} / 230', DIM, 9, None, font=KO, anchor_center=False)
    for i in range(3):  # 오른쪽 위 버튼 셋
        M.rr(1138 + i * 40, 22, 30, 30, 8, LAV, 1.2, 0.6)
    # 오른쪽 열: 판 위력 카드 머리, 웨이브 버튼, 스킬 트리 버튼
    M.rr(1044, 74, 220, 228, 12, LAV, 1.4, 0.45)
    M.word(1060, 100, '판 위력', FG, 13, None, font=KO, anchor_center=False)
    M.word(1060, 136, '3,412', WHITE, 26, None, font=BLOCK, anchor_center=False)
    M.rr(1044, 314, 220, 52, 14, LAV, 1.6, 0.7)
    M.word(1154, 346, '웨이브 12 진행 중', FG, 14, None, font=KO)
    c = PINK if tree_b > 0.85 else LAV
    M.rr(1044, 378, 220, 44, 22, c, 2.2 if tree_b > 0.85 else 1.5, tree_b)
    M.rr(1050, 384, 208, 32, 16, c, 1.0, tree_b * 0.75)
    M.word(1154, 406, '스킬 트리', WHITE if tree_b > 0.85 else FG, 15, None, font=KO)
    if tree_pt:
        M.rr(1058, 392, 54, 18, 9, GOLD, 1.0, 1.0)
        M.word(1085, 405, f'포인트 {tree_pt}', GOLD, 9, None, font=KO)
    # 판 윗부분 (흐리게)
    M.rr(262, 74, 772, 490, 10, LAV, 1.2, 0.4)


def sparks(M, x, y, k, seed, n=8, spd=4.0, col=(255, 225, 150)):
    rnd = random.Random(seed)
    for _ in range(n):
        a = rnd.uniform(-math.pi * 0.95, -math.pi * 0.05) if rnd.random() < 0.7 else rnd.uniform(0, math.pi)
        sp = rnd.uniform(spd * 0.6, spd * 1.3)
        life = rnd.randint(7, 12)
        if k < 1 or k > life: continue
        def at(t): return (x + math.cos(a) * sp * t, y + math.sin(a) * sp * t + 0.35 * t * t)
        M.ln([at(k - 1), at(k)], col, 1.4, 1.0 - k / life * 0.8)
    if k == 0: M.dots.append((x, y, 3.2))


def block_word(M, cx, y, s, c, px, bright):
    """속 빈 블록 글자 (간판)"""
    M.word(cx, y, s, c, px, bright, track=px * 0.12, font=BLOCK)


# ---------------------------------------------------------------- A 네온 간판 점등
def frame_A(f):
    t = f / FPS
    M = Sign(0.85); M.small_glow = 0.0
    fill = min(1.0, 0.82 + t * 0.6) if t < 0.3 else 0.0 + max(0, (t - 0.3)) * 0.05
    flash = max(0.0, 1 - abs(t - 0.3) / 0.12) if t < 0.45 else 0
    lv = 7 if t < 0.55 else 8
    hud(M, lv, fill if t < 0.3 else min(0.08, (t - 0.3) * 0.05), flash, tree_pt=1 if t > 0.9 else 0)
    if 0.25 < t < 0.6: M.dots.append((1120, 47, 2.6 * flash + 0.5))
    # 간판: 레벨 글자 아래에 매달림 (줄 두 개)
    if 0.35 < t < 2.0:
        u = t - 0.35
        fade = 1.0 if t < 1.6 else max(0.0, 1 - (t - 1.6) / 0.4)
        seq = [0.4, 0.08, 0.9, 0.2, 1.0]
        k = int(u * FPS)
        letters = 'LEVEL UP'
        br = []
        for i, ch in enumerate(letters):
            j = k - i
            v = 0.08 if j < 0 else (seq[j] if j < len(seq) else 1.0)
            if ch == ' ': v = 1.0
            br.append(v * fade)
        for x in (850, 950): M.ln([(x, 62), (x, 92)], DIM, 1.0, 0.5 * fade)
        M.rr(796, 92, 208, 58, 10, GOLD, 2.0, 0.9 * fade)
        block_word(M, 900, 134, letters, GOLD, 30, br)
        # 빠직: 마지막 글자 켜질 때 P 옆
        kk = k - len(letters) - 1
        if 0 <= kk < 12: sparks(M, 978, 104, kk, 7)
        # 숫자 바뀜 강조
        if 0.55 < t < 1.1:
            g = 1 - (t - 0.55) / 0.55
            M.rr(1028, 16, 56, 26, 8, GOLD, 1.6, g)
        if t > 0.9:
            pu = ease_out((t - 0.9) / 0.25)
            M.rr(840, 158 + (1 - pu) * 10, 120, 24, 12, PINK, 1.6, pu * fade)
            M.word(900, 175 + (1 - pu) * 10, '포인트 +1', WHITE, 13, [pu * fade] * 6, font=KO)
    return M.draw().crop(CROP)


# ---------------------------------------------------------------- B 잭팟
def frame_B(f):
    t = f / FPS
    M = Sign(0.85); M.small_glow = 0.0
    hud(M, 8 if t > 0.9 else 7, 0.0 if t > 0.2 else 0.9 + t, tree_pt=1 if t > 1.0 else 0)
    if t < 1.9:
        # 전구 테두리 (레벨 글자 둘레)
        fade = 1.0 if t < 1.5 else max(0.0, 1 - (t - 1.5) / 0.4)
        if fade > 0:
            M.rr(968, 14, 128, 42, 10, GOLD, 1.8, fade)
            bulbs(M, [(976, 20), (1088, 20), (1088, 50), (976, 50), (976, 20)], 13, int(t * 12) % 3)
        # 슬롯 숫자: 레벨 숫자 자리를 위로 굴림 (0.2 ~ 0.9초)
        if 0.2 < t < 0.9:
            roll = (t - 0.2) / 0.7
            offs = ease_out(roll) * 10  # 10칸 굴러 8 에서 멈춤
            for d in range(-1, 2):
                v = (int(offs) + d + 8 - 10) % 10
                yy = 36 + (d - (offs % 1)) * 22
                if 18 < yy < 54: M.word(1032, yy, str(v), GOLD, 17, None, font=KO, anchor_center=False)
        # 칩 쏟아짐
        if t > 0.9:
            rnd = random.Random(3)
            for i in range(26):
                x0 = 1032 + rnd.uniform(-40, 40)
                vx, vy = rnd.uniform(-90, 90), rnd.uniform(-160, -40)
                u = t - 0.9 - rnd.uniform(0, 0.15)
                if u <= 0 or u > 0.9: continue
                x, y = x0 + vx * u, 36 + vy * u + 420 * u * u
                M.ln(circle(x, y, 3.2, 10), GOLD, 1.2, 1 - u / 0.9)
                M.ln([(x - 1.5, y), (x + 1.5, y)], GOLD, 0.8, 1 - u / 0.9)
            if t < 1.7:
                g = ease_out((t - 0.9) / 0.2) * fade
                M.word(900, 110, 'LEVEL 8', GOLD, 30, [g] * 7, track=3, font=BLOCK)
                M.word(900, 132, '트리 포인트 +1', FG, 12, [g] * 9, font=KO)
    return M.draw().crop(CROP)


# ---------------------------------------------------------------- C 파동 + 안내
def frame_C(f):
    t = f / FPS
    M = Sign(0.85); M.small_glow = 0.0
    arrive = t > 1.15
    tb = 0.8
    if arrive: tb = 1.0 if t < 1.9 else 0.8 + 0.2 * (0.5 + 0.5 * math.sin(t * 8))
    hud(M, 8 if t > 0.15 else 7, 0.95 if t < 0.15 else 0.02, max(0, 1 - t / 0.25) if t < 0.25 else 0, tree_b=tb, tree_pt=1 if arrive else 0)
    O = (1012, 30)
    # 두 겹 고리
    for d, col in ((0.0, GOLD), (0.18, PINK)):
        u = (t - d) / 0.8
        if 0 < u < 1:
            r = 10 + ease_out(u) * 120
            M.ln(circle(O[0], O[1], r, 64), col, 2.0, (1 - u) * 0.9)
    # 화면 테두리 금빛 숨
    if t < 0.9:
        g = math.sin(min(1, t / 0.9) * math.pi) * 0.6
        M.rr(4, 4, 1272, 712, 16, GOLD, 2.0, g)
    # '+1' 이 스킬 트리 버튼으로 날아감 (0.35 ~ 1.15초), 꼬리 남김
    if 0.35 < t < 1.25:
        def pos(u):
            u = ease_out(u)
            x = O[0] + (1085 - O[0]) * u + math.sin(u * math.pi) * 60
            y = O[1] + (400 - O[1]) * u
            return x, y
        u = min(1, (t - 0.35) / 0.8)
        trail = [pos(max(0, u - k * 0.04)) for k in range(8)]
        M.ln(trail, GOLD, 2.0, 0.8)
        x, y = trail[0]
        if u < 1: M.word(x, y + 6, '+1', GOLD, 20, None, font=BLOCK); M.dots.append((x, y - 6, 1.6))
        else: sparks(M, 1085, 400, int((t - 1.15) * FPS), 11, n=10)
    if arrive and t < 1.6:
        k = int((t - 1.15) * FPS)
        sparks(M, 1085, 400, k, 11, n=10)
    return M.draw().crop(CROP)


def gif(fn, n, path):
    frames = [fn(f).convert('P', palette=Image.ADAPTIVE, colors=128) for f in range(n)]
    frames[0].save(path, save_all=True, append_images=frames[1:], duration=1000 // FPS, loop=0)


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    for name, fn, n in (('levelup_A_sign', frame_A, 60), ('levelup_B_jackpot', frame_B, 55), ('levelup_C_wave', frame_C, 55)):
        gif(fn, n, os.path.join(outdir, name + '.gif'))
        fn(int(n * 0.45)).save(os.path.join(outdir, name + '.png'))
        print(name)


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
