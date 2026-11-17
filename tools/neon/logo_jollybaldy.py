"""JOLLY BALDY (유쾌한 대머리) 회사 로고 시안 — 네온 선 (docs/15)

    python tools/neon/logo_jollybaldy.py 출력폴더

A 기본형: 둥근 머리 + 웃는 눈 + 활짝 웃는 입 + 정수리 반짝 별
B 한 가닥: A + 정수리에 꼬불 머리카락 한 가닥
C 틀: 둥근 사각 틀 안에 A (판 위 물체처럼)
anim.gif: 머리가 켜지고 → 정수리에서 '띵' 하고 광이 번쩍
"""
import math
import os
import sys

from PIL import Image

sys.path.insert(0, os.path.dirname(__file__))
from neon import bez, circle  # noqa: E402
from logo_mock import Sign, GOLD, CYAN, WHITE, PINK  # noqa: E402

TEXT_COL = CYAN


def arc(cx, cy, r, a0, a1, n=20, sx=1.0):
    return [(cx + math.cos(math.radians(a0 + (a1 - a0) * k / n)) * r * sx,
             cy + math.sin(math.radians(a0 + (a1 - a0) * k / n)) * r) for k in range(n + 1)]


def sparkle(cx, cy, r, inner=0.22):
    """네 갈래 반짝 별"""
    pts = []
    for k in range(9):
        a = math.radians(-90 + k * 45)
        rr = r if k % 2 == 0 else r * inner
        pts.append((cx + math.cos(a) * rr, cy + math.sin(a) * rr))
    return pts


def head_lines(cx, cy, R, hair=False):
    """(머리 선들, 광 선들) — 머리는 금빛, 광은 흰빛"""
    P = lambda x, y: (cx + x * R, cy + y * R)
    # 머리: 위는 둥근 돔, 아래 턱은 조금 좁고 둥글게
    head = []
    for k in range(73):
        t = k / 72 * 2 * math.pi
        s = math.sin(t)
        head.append(P(math.cos(t) * (0.84 if s < 0 else 0.84 - 0.08 * s), s * (1.0 if s < 0 else 0.9)))
    L = [head]
    # 귀 (양옆 반달)
    for sg in (-1, 1):
        L.append([P(sg * 0.83, -0.02)] + bez(P(sg * 0.83, -0.02), P(sg * 1.08, 0.05), P(sg * 0.98, 0.22), 10)[1:]
                 + bez(P(sg * 0.98, 0.22), P(sg * 0.92, 0.34), P(sg * 0.8, 0.3), 8)[1:])
        L.append(bez(P(sg * 0.88, 0.06), P(sg * 0.97, 0.12), P(sg * 0.89, 0.22), 6))
    # 눈썹 (치켜 올라감) + 웃는 눈 (∩)
    for sg in (-1, 1):
        L.append(bez(P(sg * 0.18, -0.3), P(sg * 0.32, -0.42), P(sg * 0.47, -0.32), 8))
        L.append(bez(P(sg * 0.2, -0.08), P(sg * 0.32, -0.24), P(sg * 0.44, -0.08), 8))
    # 코
    L.append(bez(P(0.0, -0.02), P(0.12, 0.14), P(-0.03, 0.17), 8))
    # 활짝 웃는 입 (D 모양) + 볼
    mouth = bez(P(-0.44, 0.27), P(0, 0.37), P(0.44, 0.27), 14) + bez(P(0.44, 0.27), P(0, 0.85), P(-0.44, 0.27), 18)[1:]
    L.append(mouth)
    L.append(bez(P(-0.2, 0.55), P(0, 0.5), P(0.2, 0.55), 8))  # 혀 윗선
    for sg in (-1, 1):
        L.append(arc(*P(sg * 0.5, 0.2), 0.1 * R, 200 if sg < 0 else -20, 260 if sg < 0 else 40, 6))  # 볼 주름
    if hair:  # 정수리 꼬불 한 가닥
        L.append(bez(P(0.06, -1.0), P(0.0, -1.22), P(0.18, -1.3), 10) + bez(P(0.18, -1.3), P(0.36, -1.32), P(0.32, -1.16), 8)[1:]
                 + bez(P(0.32, -1.16), P(0.28, -1.06), P(0.2, -1.12), 6)[1:])
    # 광: 돔 위 반짝 별 + 옆 작은 별 + 돔 따라 흐르는 빛줄
    G = [sparkle(*P(-0.4, -0.62), 0.2 * R), sparkle(*P(-0.16, -0.8), 0.07 * R, 0.3), arc(cx, cy, 0.86 * R, 228, 252, 10, 0.84)]
    return L, G


def logo(variant='A', head_b=1.0, glint_b=1.0, flash=0.0, text_b=1.0):
    M = Sign()
    cx, cy, R = 640, 300, 150 if variant != 'C' else 130
    if variant == 'B':
        cy += 20
    L, G = head_lines(cx, cy, R, hair=(variant == 'B'))
    if variant == 'C':
        M.rr(cx - 200, cy - 190, 400, 380, 48, PINK, 2.2, head_b)
    for p in L:
        M.ln(p, GOLD, 2.2, head_b)
    for p in G:
        M.ln(p, WHITE, 1.8, glint_b)
    if flash > 0:  # '띵': 큰 반짝 별 + 흰 섬광
        gx, gy = cx - 0.4 * R, cy - 0.62 * R
        M.ln(sparkle(gx, gy, R * (0.2 + 0.5 * flash), 0.12), WHITE, 1.6, 1.0)
        M.dots.append((gx, gy, 3 + 6 * flash))
    ty = 610 if variant != 'C' else 600
    M.word(640, ty, 'JOLLY BALDY', TEXT_COL, 74, [text_b] * 11, track=10)
    return M.draw()


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    imgs = []
    for v in 'ABC':
        im = logo(v)
        im.save(os.path.join(outdir, 'jollybaldy_%s.png' % v))
        imgs.append(im)
    sheet = Image.new('RGB', (1920, 360))
    for i, im in enumerate(imgs):
        sheet.paste(im.resize((640, 360), Image.LANCZOS), (i * 640, 0))
    sheet.save(os.path.join(outdir, 'jollybaldy_sheet.png'))
    # 켜짐 연출: 꺼짐 → 머리 깜빡 켜짐 → 글자 → 정수리 '띵' → 유지
    seq = [(0, 0, 0, 0)] * 5 + [(0.5, 0, 0, 0), (0.1, 0, 0, 0), (1, 0, 0, 0), (0.3, 0, 0, 0)] + [(1, 0, 0, 0)] * 3
    seq += [(1, 0, 0, t) for t in (0.2, 0.45, 0.7, 0.9, 1.0)]
    seq += [(1, 0, f, 1) for f in (1.0, 0.7, 0.4, 0.2)] + [(1, 1, 0, 1)] * 20
    frames = [logo('A', h, g, f, t).resize((640, 360), Image.LANCZOS).convert('P', palette=Image.ADAPTIVE, colors=128) for (h, g, f, t) in seq]
    frames[0].save(os.path.join(outdir, 'jollybaldy_anim.gif'), save_all=True, append_images=frames[1:], duration=70, loop=0)
    print('ok', len(frames), 'frames')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
