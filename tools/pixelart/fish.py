"""물고기 32x16. 몸통 윤곽을 수식으로 만들고, 재질 영역 + 자동 명암 + 무늬로 변형을 만든다."""
import math

from palette import INK, WHITE, ramp
from spritekit import apply, selout, shade_grid

W, H, CY = 32, 16, 8

SPECIES = {
    "carp": dict(body=ramp("#ffc27a", "#f07a3a", "#b84a34", "#6e2a38"),
                 belly=ramp("#fff4d8", "#ffd9a8", "#e0a07a", "#a06058"),
                 fin=ramp("#ffd08a", "#f09a4a", "#c0603a", "#7a3438"), pattern=None, depth=5.2),
    "mackerel": dict(body=ramp("#a8e4f0", "#4a9cc8", "#2e6498", "#22386a"),
                     belly=ramp("#ffffff", "#dceef4", "#a8c4d4", "#6a849c"),
                     fin=ramp("#8fd0e0", "#3a80b0", "#2a5488", "#1e3060"), pattern="stripe", depth=4.2),
    "amethyst": dict(body=ramp("#e0c4ff", "#9a6ad8", "#6a42a8", "#3e2a72"),
                     belly=ramp("#fff0ff", "#ecd4f8", "#c0a0d8", "#806098"),
                     fin=ramp("#f0a8e0", "#c860b8", "#8a3a90", "#522460"), pattern="spot", depth=5.8),
    "pike": dict(body=ramp("#d8f08a", "#86b84a", "#4e8040", "#2e5038"),
                 belly=ramp("#fffce0", "#f0f0b8", "#c8c890", "#8a8c68"),
                 fin=ramp("#e8c860", "#c08a3a", "#8a5a30", "#523428"), pattern="band", depth=3.6),
}


def profile(t, depth):
    """t: 0(주둥이) ~ 1(꼬리자루). 앞은 둥근 타원, 뒤는 가늘어진다."""
    if t < 0.35:
        return depth * math.sqrt(max(0.0, 1 - ((0.35 - t) / 0.37) ** 2))
    return depth * (1 - ((t - 0.35) / 0.65) ** 1.4 * 0.7)


def build_grid(depth):
    g = [["."] * W for _ in range(H)]
    x0, x1 = 2, 24
    tops, bots = {}, {}
    for x in range(x0, x1 + 1):
        hh = max(profile((x - x0) / (x1 - x0), depth), 1.0)
        for y in range(H):
            dy = y + 0.5 - CY
            if -hh <= dy <= hh * 0.8:
                g[y][x] = "L" if dy > 1.0 else "B"
                tops.setdefault(x, y)
                bots[x] = y
    # 등지느러미: 몸통 윗선 위로 솟는 삼각형
    for x in range(9, 18):
        hgt = [1, 2, 3, 3, 3, 2, 2, 1, 1][x - 9]
        for k in range(1, hgt + 1):
            g[tops[x] - k][x] = "F"
    # 배지느러미
    for x, hgt in ((13, 2), (14, 2), (15, 1)):
        for k in range(1, hgt + 1):
            if bots[x] + k < H:
                g[bots[x] + k][x] = "F"
    # 꼬리 (갈라진 형태)
    for x in range(24, 31):
        k = x - 23
        for y in range(H):
            dy = abs(y + 0.5 - CY)
            if dy <= 1.0 + k * 0.95 and not (x >= 28 and dy < (x - 27) * 0.9):
                if g[y][x] == ".":
                    g[y][x] = "T"
    return g


def fish(name):
    sp = SPECIES[name]
    g = build_grid(sp["depth"])
    ramps = {"B": sp["body"], "L": sp["belly"], "F": sp["fin"], "T": sp["fin"]}
    img = shade_grid(g, ramps, light=(0.25, 0.75), thresholds=(0.2, 0.55, 0.85))
    px = img.load()
    body = sp["body"]
    for y in range(H):
        for x in range(W):
            if g[y][x] != "B":
                continue
            if sp["pattern"] == "stripe" and (x + y // 2) % 4 == 0 and 6 < x < 23:
                px[x, y] = body[3]
            if sp["pattern"] == "spot" and (x * 5 + y * 3) % 9 == 0 and 5 < x < 23:
                px[x, y] = body[0]
            if sp["pattern"] == "band" and x in (9, 10, 15, 16, 20):
                px[x, y] = body[2]
    # 꼬리와 지느러미 결
    for y in range(H):
        for x in range(W):
            if g[y][x] in "TF" and (x + y) % 3 == 0:
                px[x, y] = sp["fin"][2]
    img = selout(img, g, ramps)
    ey = CY - 2
    return apply(img, [
        (6, ey, INK), (6, ey + 1, INK), (7, ey, WHITE),   # 눈
        (9, CY - 1, body[3]), (9, CY, body[3]), (10, CY + 1, body[3]),  # 아가미
        (3, CY + 1, body[3]),  # 입
    ])


if __name__ == "__main__":
    import sys
    from PIL import Image
    from spritekit import scale
    sheet = Image.new("RGBA", (W * 4 + 12, H), (24, 22, 34, 255))
    for i, n in enumerate(SPECIES):
        f = fish(n)
        sheet.paste(f, (i * (W + 4), 0), f)
    scale(sheet, 6).save(sys.argv[1])
