"""모든 에셋을 원본 해상도 PNG로 out/ 에 내보낸다. 언리얼 가져오기와 3D 미리보기가 이 폴더를 쓴다."""
import pathlib

import fish
import hero
import props
import tiles

OUT = pathlib.Path(__file__).parent / "out"


def main():
    OUT.mkdir(exist_ok=True)
    items = {"hero": hero.hero}
    items.update({f"tile_{k}": v for k, v in tiles.ALL.items()})
    items.update({f"prop_{k}": v for k, v in props.ALL.items()})
    items.update({f"fish_{k}": (lambda n=k: fish.fish(n)) for k in fish.SPECIES})
    for name, fn in items.items():
        fn().save(OUT / f"{name}.png")
    print(f"{len(items)} assets -> {OUT}")


if __name__ == "__main__":
    main()
