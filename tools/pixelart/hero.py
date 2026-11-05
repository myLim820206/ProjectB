"""주인공 캐릭터 24x32. 재질 영역만 지정하고 명암과 외곽선은 자동 처리."""
from palette import (BLUSH, GOLD, HAIR, INK, LEATHER, PANTS, SCARF, SKIN, TUNIC,
                     WHITE)
from spritekit import apply, parse, selout, shade_grid

# H 머리카락, S 피부, E 눈, R 스카프, C 상의, B 벨트, G 버클, P 바지, K 부츠
HERO = [
    "",
    "........HHHHHHHH",
    "......HHHHHHHHHHHH",
    ".....HHHHHHHHHHHHHH",
    "....HHHHHHHHHHHHHHHH",
    "....HHHHHHHHHHHHHHHH",
    "...HHHHHHHHHHHHHHHHHH",
    "...HHHHSSHHHHSSSSHHHH",
    "...HHHSSSSSSSSSSSSHHH",
    "...HHSSSSSSSSSSSSSSHH",
    "...HHSSSEESSSSEESSSHH",
    "...HHSSSEESSSSEESSSHH",
    "....HSSSSSSSSSSSSSSH",
    "....HSSSSSSSSSSSSSSH",
    ".....HSSSSSSSSSSSSH",
    "........SSSSSSSS",
    ".......RRRRRRRRRR",
    ".....RRRRRRRRRRRRRR",
    "....CCCCRRRRRRRRCCCC",
    "...CCCCCCCRRRCCCCCCCC",
    "...CCCCCCCRRRCCCCCCCC",
    "...CC.CCCCCRRCCCCC.CC",
    "...CC.CCCCCRCCCCCC.CC",
    "...SS.BBBBBBGBBBBB.SS",
    "...SS.CCCCCCCCCCCC.SS",
    "......CCCCCCCCCCCC",
    ".....CCCCCCCCCCCCCC",
    ".......PPPP..PPPP",
    ".......PPPP..PPPP",
    ".......KKKK..KKKK",
    "......KKKKK..KKKKK",
    "",
]
RAMPS = {"H": HAIR, "S": SKIN, "E": INK, "R": SCARF, "C": TUNIC, "B": LEATHER,
         "G": GOLD, "P": PANTS, "K": LEATHER}


def hero():
    grid = parse(HERO, 24)
    img = shade_grid(grid, RAMPS, cast={"S": "H", "C": "R", "P": "C"}, edge=("S",))
    img = selout(img, grid, RAMPS)
    return apply(img, [
        # 눈 하이라이트
        (9, 10, WHITE), (15, 10, WHITE),
        # 볼 홍조, 입
        (7, 12, BLUSH), (17, 12, BLUSH), (12, 13, SKIN[2]),
        # 머리카락 결 하이라이트
        (8, 3, HAIR[0]), (9, 3, HAIR[0]), (7, 4, HAIR[0]), (10, 2, HAIR[0]),
        (13, 5, HAIR[0]), (14, 4, HAIR[0]),
        # 앞머리 가닥 사이 그림자
        (9, 7, HAIR[2]), (12, 7, HAIR[2]),
        # 버클 반짝임
        (12, 23, GOLD[0]),
        # 부츠 앞코 하이라이트
        (7, 30, LEATHER[0]), (13, 30, LEATHER[0]),
    ])


if __name__ == "__main__":
    import sys
    from spritekit import scale
    scale(hero(), 12).save(sys.argv[1])
