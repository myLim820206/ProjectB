"""프로젝트 공용 팔레트.

명암 단계마다 색조를 튼다(hue shifting). 어두운 쪽은 푸른 보라로,
밝은 쪽은 노란 쪽으로 이동해야 픽셀 아트가 탁하지 않고 생기 있게 보인다.
각 재질은 4단계 램프: [밝음, 기본, 그림자, 깊은 그림자].
"""


def hexc(h, a=255):
    h = h.lstrip("#")
    return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16), a)


def ramp(*hexes):
    return [hexc(h) for h in hexes]


# 외곽과 공통
INK = hexc("#21182b")          # 가장 어두운 색. 순수 검정 대신 사용
WHITE = hexc("#fff8e8")
CLEAR = (0, 0, 0, 0)

# 재질 램프 [밝음, 기본, 그림자, 깊음]
SKIN = ramp("#ffe2c4", "#f4b48c", "#c97f6a", "#8c4c55")
HAIR = ramp("#f7a35c", "#d0603e", "#963a3c", "#5c2440")
TUNIC = ramp("#8fd3d6", "#4a97b4", "#2f6390", "#233c66")
SCARF = ramp("#ffe27a", "#f0a83c", "#c06a30", "#7c3a2e")
LEATHER = ramp("#d39a5e", "#a0643e", "#6e3e30", "#43252c")
PANTS = ramp("#8a86a8", "#5e5a80", "#3f3b5e", "#2a2642")
GOLD = ramp("#fff3a0", "#f2c64a", "#b88330", "#7a5028")
BLUSH = hexc("#e88a80")

STONE = ramp("#c9c6c2", "#9a96a0", "#6c6880", "#454260")
MORTAR = ramp("#5a5468", "#433e55", "#302b42", "#211c30")
MOSS = ramp("#b7d66a", "#7caa48", "#4e7c3e", "#2e5034")
DIRT = ramp("#b8906a", "#8a6650", "#5e4444", "#3c2a38")
PATH = ramp("#ecd29a", "#c8a674", "#94765c", "#5e4a48")
GRASS = ramp("#c4e070", "#82bc4c", "#4f8a40", "#2f5c3a")
WOOD = ramp("#e0a868", "#b87444", "#844a34", "#552c2c")
IRON = ramp("#d8dce8", "#9aa0b8", "#646a88", "#3c3e5c")
GEM_BLUE = ramp("#d8f8ff", "#6ad0f0", "#3a84d0", "#2a4a98")
GEM_RED = ramp("#ffd0d0", "#f05a6a", "#b02e5a", "#6a1e4a")
GEM_GREEN = ramp("#e8ffc8", "#7ae07a", "#3aa060", "#246048")
