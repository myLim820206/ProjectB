"""NEON PALLAS 로고 선 정의 (D-253) — 글꼴 없이 손으로 그린 네온 튜브 선. 언리얼 PDLogo.h 와 같은 점

    python tools/neon/logo_strokes.py 출력폴더   → logo_strokes.png 미리보기

- NEON: 블록 글자를 속이 빈 외곽선으로 (점 목록 = 꺾은선)
- Pallas: 필기체 한 줄 튜브. 점은 Catmull-Rom 으로 이어 부드럽게
- 좌표: 글자 기준선 y=0, 위가 음수. 글자마다 따로 (깜빡임을 글자별로)
Segoe Script 같은 시스템 글꼴의 외곽선은 배포 권리가 없어서 쓰지 않는다.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, spade_crown, PINK, CYAN, GOLD  # noqa: E402

# ---- NEON 블록 글자 (너비 44, 높이 60, 둥근 모서리는 언리얼에서 그대로 꺾은선)
NEON = {
    'N': [[(0, 0), (0, -60), (12, -60), (32, -22), (32, -60), (44, -60), (44, 0), (32, 0), (12, -38), (12, 0), (0, 0)]],
    'E': [[(0, 0), (0, -60), (42, -60), (42, -49), (12, -49), (12, -36), (36, -36), (36, -25), (12, -25), (12, -11), (42, -11), (42, 0), (0, 0)]],
    'O': [[(10, 0), (0, -10), (0, -50), (10, -60), (34, -60), (44, -50), (44, -10), (34, 0), (10, 0)],
          [(15, -12), (12, -15), (12, -45), (15, -48), (29, -48), (32, -45), (32, -15), (29, -12), (15, -12)]],
}
NEON_WORD = 'NEON'
NEON_ADV = 56  # 글자 간격

# ---- Pallas 필기체 (높이: 대문자 ~84, 소문자 ~40). 글자마다 선 목록, 선 = 조절점 (Catmull-Rom)
PALLAS = [
    # P: 기둥 + 고리
    [[(4, 8), (10, -14), (18, -46), (26, -78)],
     [(16, -62), (30, -80), (56, -84), (72, -70), (68, -50), (46, -40), (24, -42)]],
    # a (오른쪽으로 이음)
    [[(101, -41), (90, -45), (74, -40), (64, -20), (68, -2), (82, 0), (98, -16), (103, -33), (106, -38), (108, -31), (104, -15), (104, -4), (108, 0), (116, -4)]],
    # l
    [[(116, -4), (128, -26), (140, -62), (142, -84), (132, -88), (124, -68), (122, -30), (126, -4), (136, 0), (148, -6)]],
    # l
    [[(148, -6), (160, -28), (172, -64), (174, -84), (164, -88), (156, -68), (154, -30), (158, -4), (166, -1)]],
    # a
    [[(209, -41), (198, -45), (182, -40), (172, -20), (176, -2), (190, 0), (206, -16), (211, -33), (214, -38), (216, -31), (212, -15), (212, -4), (216, 0), (226, -4)]],
    # s
    [[(226, -4), (236, -18), (242, -35), (247, -41), (252, -33), (258, -15), (252, -2), (236, 2), (224, -6)]],
]


def catmull(pts, seg=8):
    """조절점을 지나는 부드러운 곡선 (끝점은 복제)"""
    if len(pts) < 3: return list(pts)
    P = [pts[0]] + list(pts) + [pts[-1]]
    out = []
    for i in range(1, len(P) - 2):
        p0, p1, p2, p3 = P[i - 1], P[i], P[i + 1], P[i + 2]
        for k in range(seg):
            t = k / seg
            t2, t3 = t * t, t * t * t
            out.append(tuple(0.5 * ((2 * p1[j]) + (-p0[j] + p2[j]) * t + (2 * p0[j] - 5 * p1[j] + 4 * p2[j] - p3[j]) * t2 + (-p0[j] + 3 * p1[j] - 3 * p2[j] + p3[j]) * t3) for j in (0, 1)))
    out.append(pts[-1])
    return out


def draw_logo(M, cx, y, k=1.0, nb=None, pb=None, crown=1.0):
    """y = NEON 윗선. 로고 전체 폭 ≈ 300k"""
    w = (len(NEON_WORD) - 1) * NEON_ADV + 44
    x0 = cx - w * k * 0.5
    for i, ch in enumerate(NEON_WORD):
        b = 1.0 if nb is None else nb[i]
        for poly in NEON[ch]:
            M.ln([(x0 + (i * NEON_ADV + px) * 0.8 * k + i * NEON_ADV * 0.2 * k, y + (60 + py) * k) for px, py in poly], CYAN, 1.8 * k, b)
    px0, by = cx - 140 * k, y + 170 * k
    for i, letter in enumerate(PALLAS):
        b = 1.0 if pb is None else pb[i]
        for st in letter:
            M.ln([(px0 + px * k, by + py * k) for px, py in catmull(st)], PINK, 3.0 * k, b)
    spade_crown(M, cx + 170 * k, y + 135 * k, 20 * k, GOLD, crown)


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    M = Sign()
    draw_logo(M, 640, 140, 1.4)
    M.draw().save(os.path.join(outdir, 'logo_strokes.png'))
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
