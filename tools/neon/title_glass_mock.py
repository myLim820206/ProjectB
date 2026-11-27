"""BAR Alice 간판 마티니 잔 미리보기 (D-481): main 의 잔 | 작업 중인 잔 (기울기를 인자로 주면 여러 개)

    python tools/neon/title_glass_mock.py [출력.png] [기울기...]

게임의 PaintAliceSign 그리기를 흉내 낸다: 관 바깥 테 색(Edge) 굵기 Th × 1.5 + 파스텔 가운데(Core) Th × 0.9,
번짐 두 겹(Th × 15 · 0.16, Th × 6.5 · 0.55). Th = 관 굵기(원본 화소) × K × 0.8, K = 간판 너비 / 1139.
윗줄 = 시작 화면 크기(너비 580), 아랫줄 = 잔 확대, 맨 아래 = 인게임 족보 메뉴판 크기(높이 40).
BAR 는 게임에서 블록 글자로 그리지만 여기서는 따라 그린 하늘 선으로 대신한다 (잔 미리보기용).
"""
import math
import os
import re
import subprocess
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFilter

ROOT = os.path.join(os.path.dirname(__file__), '..', '..')
sys.path.insert(0, os.path.join(ROOT, 'tools', 'ui_art'))
import gen_title_inl as G  # noqa: E402

EDGE = [(0x14, 0xd2, 0xeb), (0xff, 0x2d, 0xcd), (0xff, 0xd7, 0x14), (0x14, 0xd2, 0xeb), (0xff, 0x2d, 0xcd)]
CORE = [(0xaf, 0xff, 0xff), (0xff, 0xaf, 0xf2), (0xff, 0xfa, 0xa5), (0xaf, 0xff, 0xff), (0xff, 0xaf, 0xf2)]
TUBE = [16.62, 20.11, 14.86, 14.86, G.STRAW_TUBE]
SRC_W, SRC_H = 1139.0, 546.0
BG = (6, 6, 11)


def old_strokes():
    """main 에 있는 예전 PDTitleLogo.inl (git show) 을 읽는다"""
    txt = subprocess.run(['git', '-C', ROOT, 'show', 'origin/main:Source/ProjectB/PDTitleLogo.inl'], capture_output=True, text=True, encoding='utf-8').stdout
    arr = {int(m.group(1)): [float(v.rstrip('f')) for v in m.group(2).split(',')] for m in re.finditer(r'const float S(\d+)\[\] = \{([^}]*)\}', txt)}
    out = []
    for m in re.finditer(r'\{(\d), (\d), \d+, \w+, \w+, S(\d+)\}', txt):
        v = arr[int(m.group(3))]
        out.append((int(m.group(1)), list(zip(v[0::2], v[1::2]))))
    return out


def new_strokes(tilt):
    return [(ci, P) for ci, _, P, _, _ in G.strokes(tilt)]


def sign(strokes, tw, crop=None, S=2):
    """간판 한 장. crop = (x0, y0, x1, y1) 원본 화소 → 그 부분만 tw 너비로"""
    x0, y0, x1, y1 = crop or (0, 0, SRC_W, SRC_H)
    K = tw / (x1 - x0)
    Kt = K if crop is None else tw / SRC_W * (SRC_W / (x1 - x0))
    W, H = int(tw * S), int((y1 - y0) * K * S)
    img = Image.new('RGB', (W, H), BG)
    P = lambda pts: [((x - x0) * K * S, (y - y0) * K * S) for x, y in pts]
    for wide, a in ((15.0, 0.16), (6.5, 0.55)):
        layer = Image.new('RGB', img.size, (0, 0, 0))
        d = ImageDraw.Draw(layer)
        for ci, pts in strokes:
            th = TUBE[ci] * Kt * 0.8 * S
            d.line(P(pts), fill=tuple(int(c * a) for c in EDGE[ci]), width=max(1, int(th * wide * 0.35)), joint='curve')
        img = ImageChops.add(img, layer.filter(ImageFilter.GaussianBlur(max(1, th * wide * 0.18))))
    d = ImageDraw.Draw(img)
    for ci, pts in strokes:
        th = TUBE[ci] * Kt * 0.8 * S
        for col, w in ((EDGE[ci], th * 1.5), (CORE[ci], th * 0.9)):
            d.line(P(pts), fill=col, width=max(1, int(w)), joint='curve')
            r = w / 2
            for q in (P(pts)[0], P(pts)[-1]):
                d.ellipse([q[0] - r, q[1] - r, q[0] + r, q[1] + r], fill=col)
    return img


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'DOC', 'assets', 'images', 'title-wall', 'glass_redraw.png')
    tilts = [float(t) for t in sys.argv[2:]] or [G.TILT]
    sets = [old_strokes()] + [new_strokes(t) for t in tilts]
    GLASS_BOX = (770, 30, 1139, 450)
    cols = []
    for st in sets:
        full = sign(st, 580)
        zoom = sign(st, 340, GLASS_BOX)
        small = sign(st, 40 * SRC_W / SRC_H)
        col = Image.new('RGB', (full.width, full.height + zoom.height + small.height + 40), BG)
        col.paste(full, (0, 0))
        col.paste(zoom, ((full.width - zoom.width) // 2, full.height + 10))
        col.paste(small, ((full.width - small.width) // 2, full.height + zoom.height + 30))
        cols.append(col)
    img = Image.new('RGB', (sum(c.width for c in cols) + 20 * (len(cols) - 1), cols[0].height), (20, 20, 30))
    x = 0
    for c in cols:
        img.paste(c, (x, 0)); x += c.width + 20
    img.save(out)
    print('ok', out, img.size)


if __name__ == '__main__':
    main()
