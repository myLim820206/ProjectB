"""16x16 타일. 가로세로 이음매 없이 반복되도록 좌표를 감싸서(wrap) 그린다."""
import random

from PIL import Image

from palette import (DIRT, PATH, GEM_BLUE, GEM_GREEN, GEM_RED, GRASS, IRON, MORTAR, MOSS,
                     STONE, WOOD)

N = 16


def _new(c):
    return Image.new("RGBA", (N, N), c)


def _put(img, x, y, c):
    img.putpixel((x % N, y % N), c)


def pebble(img, x, y, ramp, w=2, h=2):
    """작은 돌 덩어리: 왼쪽 위 밝음, 오른쪽 아래 어두움."""
    for dy in range(h):
        for dx in range(w):
            c = ramp[1]
            if dx == 0 and dy == 0:
                c = ramp[0]
            elif dx == w - 1 or dy == h - 1:
                c = ramp[2]
            _put(img, x + dx, y + dy, c)
    _put(img, x + w, y + h - 1, ramp[3])


def stone_brick(seed=1, moss=False):
    rnd = random.Random(seed)
    img = _new(MORTAR[1])
    for row in range(4):
        y0 = row * 4
        off = 0 if row % 2 == 0 else 4
        for col in range(2):
            x0 = col * 8 + off
            tone = rnd.choice([0, 0, 1])  # 일부 벽돌은 살짝 어둡게
            base = STONE[1 + tone]
            for dx in range(1, 8):
                _put(img, x0 + dx, y0 + 1, STONE[0] if tone == 0 else STONE[1])
                _put(img, x0 + dx, y0 + 2, base)
                _put(img, x0 + dx, y0 + 3, STONE[2 + tone] if tone == 0 else STONE[3])
            _put(img, x0 + 1, y0 + 2, STONE[0] if tone == 0 else STONE[1])
            _put(img, x0 + 7, y0 + 2, STONE[2 + tone])
            if rnd.random() < 0.3:  # 균열
                cx = x0 + rnd.randint(3, 5)
                _put(img, cx, y0 + 2, STONE[3])
                _put(img, cx + 1, y0 + 3, STONE[3])
            if rnd.random() < 0.25:  # 흠집 점
                _put(img, x0 + rnd.randint(2, 6), y0 + 2, STONE[2])
        # 줄눈 아래 그림자
        for x in range(N):
            if img.getpixel((x, (y0 + 4) % N)) == MORTAR[1]:
                _put(img, x, y0 + 4, MORTAR[2])
    if moss:
        for _ in range(3):
            mx, my = rnd.randint(0, 15), rnd.choice([0, 4, 8, 12])
            for dx in range(rnd.randint(3, 5)):
                _put(img, mx + dx, my, MOSS[1])
                _put(img, mx + dx, my + 1, MOSS[0] if dx % 2 == 0 else MOSS[1])
                if rnd.random() < 0.5:
                    _put(img, mx + dx, my + 2, MOSS[2])
    return img


def dirt(seed=2):
    rnd = random.Random(seed)
    img = _new(DIRT[1])
    # 큰 얼룩으로 질감
    for _ in range(10):
        x, y = rnd.randint(0, 15), rnd.randint(0, 15)
        for dx in range(rnd.randint(2, 4)):
            _put(img, x + dx, y, DIRT[2])
    for _ in range(4):
        pebble(img, rnd.randint(0, 15), rnd.randint(0, 15), STONE, rnd.choice([2, 3]), 2)
    for _ in range(3):
        x, y = rnd.randint(0, 15), rnd.randint(0, 15)
        _put(img, x, y, DIRT[0])
    return img


def path(seed=12):
    """흙길: 밝고 채도가 낮은 모래흙에 자갈."""
    rnd = random.Random(seed)
    img = _new(PATH[1])
    for _ in range(12):
        x, y = rnd.randint(0, 15), rnd.randint(0, 15)
        for dx in range(rnd.randint(2, 4)):
            _put(img, x + dx, y, PATH[0] if rnd.random() < 0.5 else PATH[2])
    for _ in range(3):
        pebble(img, rnd.randint(0, 15), rnd.randint(0, 15), STONE, 2, 2)
    return img


def grass_side(seed=3):
    """지면 블록 옆면: 위는 풀, 아래는 흙."""
    rnd = random.Random(seed)
    img = dirt(seed + 10)
    depth = [rnd.choice([3, 4, 4, 5]) for _ in range(N)]
    for x in range(N):
        for y in range(depth[x]):
            c = GRASS[1]
            if y == 0:
                c = GRASS[0] if x % 3 else GRASS[1]
            elif y == depth[x] - 1:
                c = GRASS[2]
            _put(img, x, y, c)
        _put(img, x, depth[x], GRASS[3])
        _put(img, x, depth[x] + 1, DIRT[3])  # 풀 아래 그림자
    # 늘어진 풀 가닥
    for _ in range(3):
        x = rnd.randint(0, 15)
        _put(img, x, depth[x], GRASS[2])
        _put(img, x, depth[x] + 1, GRASS[3])
    return img


def grass_top(seed=4):
    """지면 블록 윗면: 풀 덤불 패턴과 작은 꽃."""
    rnd = random.Random(seed)
    img = _new(GRASS[1])
    for _ in range(18):
        x, y = rnd.randint(0, 15), rnd.randint(0, 15)
        _put(img, x, y, GRASS[2])
        _put(img, x - 1, y - 1, GRASS[0])
        _put(img, x + 1, y - 1, GRASS[0])
    for _ in range(6):
        x, y = rnd.randint(0, 15), rnd.randint(0, 15)
        _put(img, x, y, GRASS[3])
    for c in ((255, 240, 200, 255), (255, 180, 200, 255)):
        x, y = rnd.randint(0, 15), rnd.randint(0, 15)
        _put(img, x, y, c)
        _put(img, x, y + 1, GRASS[3])
    return img


def wood_plank(seed=5):
    rnd = random.Random(seed)
    img = _new(WOOD[1])
    for row in range(4):
        y0 = row * 4
        seam = rnd.randint(4, 12) + (row % 2) * 4
        for x in range(N):
            _put(img, x, y0, WOOD[0])
            _put(img, x, y0 + 3, WOOD[2])
        _put(img, seam, y0, WOOD[3])
        _put(img, seam, y0 + 1, WOOD[3])
        _put(img, seam, y0 + 2, WOOD[3])
        _put(img, seam, y0 + 3, WOOD[3])
        _put(img, seam + 1, y0 + 1, WOOD[0])
        for _ in range(2):  # 나뭇결
            gx = rnd.randint(0, 15)
            for dx in range(rnd.randint(2, 4)):
                _put(img, gx + dx, y0 + rnd.choice([1, 2]), WOOD[2])
        _put(img, seam - 2, y0 + 1, IRON[1])  # 못
        _put(img, seam + 3, y0 + 2, IRON[2])
    return img


def crate(seed=6):
    img = wood_plank(seed)
    d = img.load()
    for i in range(N):  # 테두리 각목
        for t in range(2):
            d[i, t] = WOOD[0] if t == 0 else WOOD[1]
            d[i, N - 1 - t] = WOOD[2] if t == 1 else WOOD[3]
            d[t, i] = WOOD[0] if t == 0 else WOOD[1]
            d[N - 1 - t, i] = WOOD[2] if t == 1 else WOOD[3]
    for i in range(2, N - 2):  # 대각 보강재
        d[i, N - 1 - i] = WOOD[0]
        d[i, N - i] = WOOD[1] if i < N - 2 else d[i, N - i]
        if N - 2 - i >= 2:
            d[i, N - 2 - i] = WOOD[3]
    for cx, cy in ((0, 0), (N - 3, 0), (0, N - 3), (N - 3, N - 3)):  # 쇠 모서리
        for dx in range(3):
            for dy in range(3):
                if dx == 0 or dy == 0 or (dx, dy) == (1, 1):
                    d[cx + dx, cy + dy] = IRON[0] if dx + dy < 2 else IRON[2]
    return img


def _cells(rnd, n):
    pts = [(rnd.uniform(0, N), rnd.uniform(0, N)) for _ in range(n)]

    def nearest(x, y):
        best = []
        for i, (px, py) in enumerate(pts):
            dx = min(abs(x - px), N - abs(x - px))
            dy = min(abs(y - py), N - abs(y - py))
            sx = (x - px + N / 2) % N - N / 2
            sy = (y - py + N / 2) % N - N / 2
            best.append((dx * dx + dy * dy, i, sx, sy))
        best.sort()
        return best[0], best[1]
    return nearest


def stone_rough(seed=7):
    """보로노이 셀로 만든 자갈 벽. 셀 경계는 틈, 셀 안은 왼쪽 위가 밝다."""
    rnd = random.Random(seed)
    nearest = _cells(rnd, 6)
    img = _new(STONE[2])
    for y in range(N):
        for x in range(N):
            (d1, i, sx, sy), (d2, *_rest) = nearest(x + 0.5, y + 0.5)
            if d2 ** 0.5 - d1 ** 0.5 < 1.0:
                c = STONE[3]
            else:
                t = (sx + sy) / 6.0
                c = STONE[0] if t < -0.75 else STONE[1] if t < 0.35 else STONE[2]
            img.putpixel((x, y), c)
    return img


def ore(gem, seed=8):
    rnd = random.Random(seed)
    img = stone_rough(seed)
    crystal = [
        ".0.",
        "012",
        "112",
        ".23",
    ]
    for _ in range(2):
        x, y = rnd.randint(0, 15), rnd.randint(0, 15)
        for dy, row in enumerate(crystal):
            for dx, ch in enumerate(row):
                if ch != ".":
                    _put(img, x + dx, y + dy, gem[int(ch)])
        _put(img, x + 1, y + 1, (255, 255, 255, 255))
        _put(img, x + 3, y + 3, STONE[3])
    for _ in range(2):  # 작은 조각
        x, y = rnd.randint(0, 15), rnd.randint(0, 15)
        _put(img, x, y, gem[1])
        _put(img, x + 1, y, gem[2])
    return img

ALL = {
    "stone_brick": lambda: stone_brick(1),
    "stone_brick_moss": lambda: stone_brick(11, moss=True),
    "dirt": dirt,
    "path": path,
    "grass_side": grass_side,
    "grass_top": grass_top,
    "wood_plank": wood_plank,
    "crate": crate,
    "stone_rough": stone_rough,
    "ore_blue": lambda: ore(GEM_BLUE, 8),
    "ore_red": lambda: ore(GEM_RED, 9),
    "ore_green": lambda: ore(GEM_GREEN, 10),
}

if __name__ == "__main__":
    import sys
    from spritekit import scale
    names = list(ALL)
    sheet = Image.new("RGBA", (len(names) * 34, 32), (24, 22, 34, 255))
    for i, n in enumerate(names):
        t = ALL[n]()
        # 2x2 반복으로 이음매 확인
        for ox in range(2):
            for oy in range(2):
                sheet.paste(t, (i * 34 + ox * 16, oy * 16))
    scale(sheet, 5).save(sys.argv[1])
