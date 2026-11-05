"""소품 스프라이트: 나무, 덤불, 횃불."""
from palette import INK, ramp
from spritekit import apply, parse, selout, shade_grid

LEAF = ramp("#b4e06a", "#64a64a", "#3c7046", "#25473e")
BARK = ramp("#b88a5a", "#7e5440", "#553638", "#352232")
FIRE = ramp("#fffbe0", "#ffd75a", "#f58a3a", "#c8463a")
POST = ramp("#9a6a44", "#6e4636", "#4a2e30", "#2e1c28")


def blobs(w, h, circles, trunk=None):
    """circles: [(cx, cy, r, 문자)] 뒤에서 앞 순서. 덩어리마다 다른 문자로 칠해 개별 명암."""
    g = [["."] * w for _ in range(h)]
    if trunk:
        for x0, y0, x1, y1 in trunk:
            for y in range(y0, y1 + 1):
                for x in range(x0, x1 + 1):
                    g[y][x] = "T"
    for cx, cy, r, ch in circles:
        for y in range(h):
            for x in range(w):
                if (x + 0.5 - cx) ** 2 + (y + 0.5 - cy) ** 2 <= r * r:
                    g[y][x] = ch
    return g


def tree(k=1):
    """k: 크기 배율. 형태는 같고 픽셀 해상도만 커진다."""
    w, h = 32 * k, 44 * k
    circles = [(16, 14, 9, "a"), (9, 19, 7, "b"), (23, 19, 7, "c"),
               (12, 24, 6.5, "d"), (20, 24, 6.5, "e"), (16, 9, 6, "f")]
    g = blobs(w, h, [(cx * k, cy * k, r * k, ch) for cx, cy, r, ch in circles],
              trunk=[(14 * k, 26 * k, 18 * k - 1, 42 * k - 1), (12 * k, 41 * k, 20 * k - 1, 42 * k - 1)])
    ramps = {k: LEAF for k in "abcdef"}
    ramps["T"] = BARK
    img = shade_grid(g, ramps, light=(0.5, 0.5), thresholds=(0.2, 0.55, 0.85),
                     cast={"T": "d"})
    img = selout(img, g, ramps)
    px = img.load()
    # 잎 질감: 밝은 면 위에 짧은 잎 하이라이트를 무작위로 흩뿌림
    import random
    rnd = random.Random(5)
    for y in range(h - 1):
        for x in range(w - 1):
            if g[y][x] in "abcdef" and px[x, y] == LEAF[1] and rnd.random() < 0.05:
                px[x, y] = LEAF[0]
                if px[x + 1, y] == LEAF[1]:
                    px[x + 1, y] = LEAF[0]
            elif g[y][x] in "abcdef" and px[x, y] == LEAF[2] and rnd.random() < 0.04:
                px[x, y] = LEAF[3]
            if g[y][x] == "T" and (y % 4 == 0) and x == 15 * k:
                px[x, y] = BARK[2]
    return img


def bush():
    g = blobs(24, 16, [(7, 10, 5.5, "a"), (17, 10, 5.5, "b"), (12, 7, 6, "c")])
    ramps = {k: LEAF for k in "abc"}
    img = shade_grid(g, ramps, light=(0.5, 0.5), thresholds=(0.2, 0.55, 0.85))
    img = selout(img, g, ramps)
    return apply(img, [(10, 4, (255, 236, 240, 255)), (16, 7, (255, 200, 220, 255)),
                       (6, 8, (255, 236, 240, 255))])


TORCH = [
    "...ff...",
    "..fFFf..",
    "..FYYF..",
    ".fFYYFf.",
    ".FYWWYF.",
    ".FYWWYF.",
    "..FYYF..",
    "..IIII..",
    "...PP...",
    "...PP...",
    "...PP...",
    "...PP...",
    "...PP...",
    "...PP...",
    "...PP...",
    "...PP...",
    "..PPPP..",
    "",
]


def torch():
    g = parse(TORCH, 8)
    ramps = {"f": FIRE[3], "F": FIRE[2], "Y": FIRE[1], "W": FIRE[0], "I": INK, "P": POST}
    img = shade_grid(g, ramps)
    return selout(img, g, {"P": POST})


ALL = {"tree": tree, "tree_large": lambda: tree(2), "bush": bush, "torch": torch}

if __name__ == "__main__":
    import sys
    from PIL import Image
    from spritekit import scale
    sheet = Image.new("RGBA", (150, 90), (24, 22, 34, 255))
    x = 1
    for n, fn in ALL.items():
        im = fn()
        sheet.paste(im, (x, 89 - im.height), im)
        x += im.width + 4
    scale(sheet, 6).save(sys.argv[1])
