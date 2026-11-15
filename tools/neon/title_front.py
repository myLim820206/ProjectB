"""NEON PALLAS 대문(제목 화면) UI 시안 — 확정 로고(D-249/251: 칸 없는 NEON + 필기체 Pallas + 왕관 스페이드) 분위기

    python tools/neon/title_front.py 출력폴더

A 바 입구: 가운데 로고, 버튼은 테두리 없는 네온 글자 메뉴 (고른 줄만 켜지고 양옆 스페이드 커서), 아래 정보 줄
B 벽 간판: 벽돌 벽에 로고를 걸고 바닥에 반사, 오른쪽에 세로 튜브 버튼
C 큰 버튼 + 아이콘: 이어하기 · 게임 시작 큰 마키 두 개, 나머지 넷은 동그란 아이콘 버튼
"""
import os
import sys

from PIL import Image, ImageChops

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, spade_crown, PINK, CYAN, GOLD, WHITE  # noqa: E402
from logo_retro import bulbs, ORANGE, SCRIPT, BLOCK, KO  # noqa: E402
from neon import suit, circle  # noqa: E402

MENU = ['이어하기', '게임 시작', '영구 강화', '잠금 해제', '환경 설정', '게임 종료']
DIMC = (95, 105, 125)
SEL = 1


def logo(M, cx, y, k=1.0):
    """확정 로고: NEON(칸 없음) 위, Pallas 아래, 오른쪽 왕관 스페이드. y = NEON 윗선"""
    size = 66 * k
    w, gap = size * 0.86, 4 * k
    x0 = cx - (4 * w + 3 * gap) / 2
    for i, ch in enumerate('NEON'):
        M.word(x0 + i * (w + gap) + w / 2, y + size * 0.8, ch, CYAN, size * 0.72, font=BLOCK)
    M.word(cx - 20 * k, y + 178 * k, 'Pallas', PINK, 96 * k, None, font=SCRIPT)
    spade_crown(M, cx + 240 * k, y + 145 * k, 20 * k, GOLD)


def chip(M, x, y, txt, c=GOLD):
    w = 40 + len(txt) * 11
    M.rr(x, y, w, 30, 15, c, 1.4)
    for p in suit('diamond')(x + 16, y + 15, 6): M.ln(p, c, 1.2)
    M.word(x + 30, y + 21, txt, c, 14, None, font=KO, anchor_center=False)


def reflect(img, box, floor_y, k=0.16):
    """box 영역을 floor_y 아래로 뒤집어 흐리게 더한다 (젖은 바닥 반사)"""
    x0, y0, x1, y1 = box
    part = img.crop(box).transpose(Image.FLIP_TOP_BOTTOM).point(lambda v: int(v * k))
    h = min(part.height, img.height - floor_y)
    out = img.copy()
    region = out.crop((x0, floor_y, x1, floor_y + h))
    out.paste(ImageChops.add(region, part.crop((0, 0, x1 - x0, h))), (x0, floor_y))
    return out


# ---------------------------------------------------------------- A 바 입구 (글자 메뉴)
def front_A():
    M = Sign()
    logo(M, 640, 40)
    chip(M, 1080, 24, '칩 1,240')
    for i, n in enumerate(MENU):
        y = 300 + i * 54
        on = i == SEL
        M.word(640, y + 30, n, WHITE if on else (150, 160, 180), 24 if on else 20, [1.0 if on else 0.42] * len(n), track=4 if on else 2, font=KO)
        if on:
            for s in (-1, 1):
                for p in suit('spade')(640 + s * 110, y + 21, 9): M.ln(p, PINK, 1.6)
            M.ln([(560, y + 44), (720, y + 44)], ORANGE, 1.6)
            bulbs(M, [(560, y + 52), (720, y + 52)], 16, 0)
    # 아래 정보 줄
    M.ln([(80, 660), (1200, 660)], DIMC, 1.0, 0.35)
    M.word(80, 690, '최고 기록 42웨이브', DIMC, 12, None, font=KO, anchor_center=False)
    M.word(640, 690, '↑↓ 고르기   Enter 결정', DIMC, 12, None, font=KO)
    M.word(1200 - 60, 690, 'v0.9', DIMC, 12, None, font=KO, anchor_center=False)
    return M.draw()


# ---------------------------------------------------------------- B 벽 간판 + 바닥 반사
def front_B():
    M = Sign()
    BR = (60, 48, 58)
    # 벽돌 벽 (흐린 선)
    for r in range(0, 14):
        y = 20 + r * 34
        M.ln([(0, y), (1280, y)], BR, 1.0, 0.3)
        for c in range(0, 22):
            x = c * 64 + (32 if r % 2 else 0)
            M.ln([(x, y), (x, y + 34)], BR, 1.0, 0.3)
    M.ln([(0, 500), (1280, 500)], (120, 110, 140), 1.4, 0.5)  # 벽과 바닥 경계
    logo(M, 400, 110, 1.15)
    chip(M, 1080, 24, '칩 1,240')
    for i, n in enumerate(MENU):
        y = 120 + i * 58
        x0 = 860
        on = i == SEL
        c = PINK if on else CYAN
        M.rr(x0, y, 300, 44, 22, c, 2.2 if on else 1.2, 1.0 if on else 0.3)
        M.word(x0 + 150, y + 30, n, WHITE if on else c, 18, [1.0 if on else 0.5] * len(n), font=KO)
        if on:
            for p in suit('heart')(x0 - 22, y + 22, 8): M.ln(p, PINK, 1.4)
    M.word(860, 498 - 14, '최고 기록 42웨이브', DIMC, 12, None, font=KO, anchor_center=False)
    img = M.draw()
    img = reflect(img, (120, 120, 760, 340), 500 + 4, 0.2)
    img = reflect(img, (840, 110, 1190, 470), 500 + 4, 0.1)
    return img


# ---------------------------------------------------------------- C 큰 버튼 + 아이콘 버튼
def icon(M, kind, x, y, r, c, b):
    if kind == 'up':  # 영구 강화: 위 화살표 둘
        for dy in (-4, 6):
            M.ln([(x - r * 0.5, y + dy + r * 0.2), (x, y + dy - r * 0.3), (x + r * 0.5, y + dy + r * 0.2)], c, 1.6, b)
    elif kind == 'lock':  # 잠금 해제: 열린 자물쇠
        M.rr(x - r * 0.5, y - r * 0.05, r, r * 0.7, 3, c, 1.5, b)
        M.ln([(x - r * 0.3, y - r * 0.05), (x - r * 0.3, y - r * 0.45), (x + r * 0.1, y - r * 0.65), (x + r * 0.3, y - r * 0.5)], c, 1.5, b)
    elif kind == 'gear':  # 환경 설정
        M.ln(circle(x, y, r * 0.32, 16), c, 1.5, b)
        for k in range(8):
            import math
            a = k * math.pi / 4
            M.ln([(x + math.cos(a) * r * 0.48, y + math.sin(a) * r * 0.48), (x + math.cos(a) * r * 0.7, y + math.sin(a) * r * 0.7)], c, 2.0, b)
    elif kind == 'exit':  # 게임 종료: 문과 화살표
        M.ln([(x + r * 0.1, y - r * 0.6), (x - r * 0.5, y - r * 0.6), (x - r * 0.5, y + r * 0.6), (x + r * 0.1, y + r * 0.6)], c, 1.5, b)
        M.ln([(x - r * 0.15, y), (x + r * 0.6, y)], c, 1.5, b)
        M.ln([(x + r * 0.35, y - r * 0.25), (x + r * 0.6, y), (x + r * 0.35, y + r * 0.25)], c, 1.5, b)


def front_C():
    M = Sign()
    logo(M, 640, 36)
    chip(M, 1080, 24, '칩 1,240')
    # 큰 마키 버튼 둘
    for i, (n, sub) in enumerate((('이어하기', '웨이브 17 · 하트 덱'), ('게임 시작', '새 판'))):
        x0, y = 330 + i * 320, 300
        on = i == SEL
        c = ORANGE if on else DIMC
        M.rr(x0, y, 300, 110, 10, c, 2.2 if on else 1.3, 1.0 if on else 0.55)
        if on: bulbs(M, [(x0 + 12, y + 12), (x0 + 288, y + 12), (x0 + 288, y + 98), (x0 + 12, y + 98), (x0 + 12, y + 12)], 22, 0)
        M.word(x0 + 150, y + 62, n, WHITE if on else DIMC, 26, [1.0 if on else 0.6] * len(n), font=KO)
        M.word(x0 + 150, y + 86, sub, GOLD if on else DIMC, 12, [1.0 if on else 0.5] * len(sub), font=KO)
    # 아이콘 버튼 넷
    items = [('up', '영구 강화', GOLD), ('lock', '잠금 해제', PINK), ('gear', '환경 설정', CYAN), ('exit', '게임 종료', (150, 160, 180))]
    for k, (kind, n, c) in enumerate(items):
        x, y = 640 - 285 + k * 190, 500
        M.ln(circle(x, y, 38, 40), c, 1.6, 0.75)
        icon(M, kind, x, y, 30, c, 0.9)
        M.word(x, y + 66, n, c, 14, [0.8] * len(n), font=KO)
    M.word(640, 690, '최고 기록 42웨이브', DIMC, 12, None, font=KO)
    return M.draw()


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    for name, fn in (('front_A_menu', front_A), ('front_B_wall', front_B), ('front_C_icons', front_C)):
        fn().save(os.path.join(outdir, name + '.png'))
        print(name)


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
