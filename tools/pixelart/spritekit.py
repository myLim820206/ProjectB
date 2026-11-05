"""스프라이트 생성 공통 도구.

- 재질 영역 맵(문자 그리드)을 받아 광원 방향 기준으로 자동 명암을 넣는다.
- 색 있는 외곽선(sel-out): 외곽선 색을 인접 재질의 어두운 톤으로 칠한다.
- 수동 덧칠(override)로 눈, 하이라이트 같은 세부를 얹는다.
광원은 왼쪽 위를 기본으로 한다.
"""
from PIL import Image

from palette import CLEAR, INK


def parse(rows, width=None):
    width = width or max(len(r) for r in rows)
    return [list(r.ljust(width, ".")) for r in rows]


def _dist(grid, x, y, dx, dy, ch):
    h, w = len(grid), len(grid[0])
    k = 0
    while True:
        k += 1
        nx, ny = x + dx * k, y + dy * k
        if not (0 <= nx < w and 0 <= ny < h) or grid[ny][nx] != ch:
            return k


def shade_grid(grid, ramps, light=(0.65, 0.35), thresholds=(0.22, 0.62, 0.86),
               cast=None, flat=(), edge=()):
    """grid: 문자 2차원 배열. ramps: 문자 -> 4단계 램프.

    light: (가로 가중치, 세로 가중치). 왼쪽 위가 밝다.
    cast: {아래쪽 재질: 위쪽 재질} 위쪽 재질이 바로 위에 있으면 그림자 톤.
    flat: 명암 없이 기본색만 쓰는 재질.
    edge: 기본색 위주로 칠하고 오른쪽, 아래 가장자리만 그림자를 넣는 재질 (얼굴 등).
    """
    h, w = len(grid), len(grid[0])
    img = Image.new("RGBA", (w, h), CLEAR)
    cast = cast or {}
    for y in range(h):
        for x in range(w):
            ch = grid[y][x]
            if ch not in ramps:
                continue
            r = ramps[ch]
            if not isinstance(r, list):
                img.putpixel((x, y), r)
                continue
            if ch in flat:
                img.putpixel((x, y), r[1])
                continue
            dl, dr = _dist(grid, x, y, -1, 0, ch), _dist(grid, x, y, 1, 0, ch)
            du, dd = _dist(grid, x, y, 0, -1, ch), _dist(grid, x, y, 0, 1, ch)
            tx = (dl - 0.5) / (dl + dr - 1)
            ty = (du - 0.5) / (du + dd - 1)
            t = light[0] * tx + light[1] * ty
            idx = 0 if t < thresholds[0] else 1 if t < thresholds[1] else 2 if t < thresholds[2] else 3
            if ch in edge:
                idx = 2 if (dr == 1 or dd == 1) else 1
            if ch in cast and y > 0 and grid[y - 1][x] == cast[ch]:
                idx = max(idx, 2)
            img.putpixel((x, y), r[idx])
    return img


def selout(img, grid, ramps, pad=0):
    """투명 칸 중 재질에 붙은 칸을 외곽선으로 칠한다.

    위/왼쪽 외곽은 재질의 깊은 톤, 아래/오른쪽 외곽은 INK로 칠해 입체감을 준다.
    """
    w, h = img.size
    out = img.copy()
    for y in range(h):
        for x in range(w):
            if img.getpixel((x, y))[3]:
                continue
            best = None
            for dx, dy, side in ((1, 0, "lt"), (0, 1, "lt"), (-1, 0, "rb"), (0, -1, "rb")):
                nx, ny = x + dx, y + dy
                if 0 <= nx < w and 0 <= ny < h and img.getpixel((nx, ny))[3]:
                    ch = grid[ny][nx]
                    r = ramps.get(ch)
                    if side == "rb" or not isinstance(r, list):
                        best = INK
                        break
                    best = r[3]
            if best:
                out.putpixel((x, y), best)
    return out


def apply(img, overrides):
    """overrides: [(x, y, color)]"""
    for x, y, c in overrides:
        img.putpixel((x, y), c)
    return img


def scale(img, s):
    return img.resize((img.width * s, img.height * s), Image.NEAREST)
