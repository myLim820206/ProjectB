"""문서용 에셋 샘플 시트. 사용법: python3 tools/pixelart/sample_sheet.py [출력 경로]"""
import pathlib
import sys

from PIL import Image, ImageDraw

import fish
import hero
import props
import tiles
from spritekit import scale

BG = (28, 24, 40)
FG = (230, 226, 240)


def main(out):
    sheet = Image.new("RGB", (1040, 640), BG)
    d = ImageDraw.Draw(sheet)

    def put(img, x, y, s):
        im = scale(img, s)
        sheet.paste(im, (x, y), im)
        return im

    d.text((24, 16), "Character 24x32", fill=FG)
    put(hero.hero(), 24, 36, 6)
    d.text((200, 16), "Props", fill=FG)
    put(props.tree(), 200, 36, 5)
    put(props.bush(), 370, 136, 5)
    put(props.torch(), 500, 126, 5)
    d.text((580, 16), "Fish 32x16 (procedural species)", fill=FG)
    for i, n in enumerate(fish.SPECIES):
        put(fish.fish(n), 580 + (i % 2) * 220, 40 + (i // 2) * 110, 6)
    d.text((24, 280), "Tiles 16x16 (seamless, shown 2x2)", fill=FG)
    for i, n in enumerate(tiles.ALL):
        t = tiles.ALL[n]()
        rep = Image.new("RGBA", (32, 32))
        for ox in (0, 16):
            for oy in (0, 16):
                rep.paste(t, (ox, oy))
        col, row = i % 6, i // 6
        put(rep, 24 + col * 166, 304 + row * 166, 4)
        d.text((24 + col * 166, 304 + row * 166 + 132), n, fill=(160, 156, 180))
    sheet.save(out)
    print("saved", out)


if __name__ == "__main__":
    default = pathlib.Path(__file__).resolve().parents[2] / "docs/images/art-sample.png"
    main(sys.argv[1] if len(sys.argv) > 1 else default)
