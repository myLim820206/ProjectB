"""Alice 인게임 시안 그림(docs/images/ui-1120/reference.webp, 2000 × 1125)에서 바뀌지 않는 그림을 잘라
게임이 실행 중에 읽는 PNG(Content/UIArt/*.png)로 만든다 (D-403).

    python tools/ui_art/cut_ref.py

- sign      : 야자수 + 네온 줄 + 분홍 타원 안 'WAVE' (파란 상자와 숫자는 게임이 그린다)
- logo      : BAR 꼬리표 + Alice 필기체 + 마티니
- glass_on  : 켜진 칵테일 잔 (오렌지 조각 · 꼬치 · 술)
- glass_off : 꺼진 칵테일 잔
바탕(거의 검은 남색 · 보랏빛 패널)을 빼서 알파로 만든다: 가장자리 바탕색 b 를 재고,
알파 a = 바탕보다 밝은 정도, 색 = (화소 - b·(1 - a)) — 게임 붓은 미리 곱한 알파(PreMultipliedAlpha)라 색을 미리 곱한 채로 저장한다.
"""
import os
import sys

import numpy as np
from PIL import Image

ROOT = os.path.join(os.path.dirname(__file__), '..', '..')
REF = os.path.join(ROOT, 'docs', 'images', 'ui-1120', 'reference.webp')
OUT = os.path.join(ROOT, 'Content', 'UIArt')

# 시안 그림 좌표 (2000 × 1125). 게임 좌표 = × 0.64
CUTS = {
    'sign': (405, 0, 1185, 128),   # 왼쪽이 열린 파란 상자(위 · 오른쪽 · 아래 테)까지. 상자 안 숫자는 지우고 게임이 그린다 (D-407)
    'logo': (32, 27, 180, 84),
    'glass_on': (1634, 748, 1702, 829),
    'glass_off': (1838, 748, 1906, 829),
    'tv': (1616, 0, 1995, 632),   # 안테나 · 몸통 · 테 · 배속 버튼 · 다이얼 · 스피커 (D-408). 화면 안쪽과 '?' 는 지우고 게임이 그린다
    # 오른쪽 아래 (D-414): 위아래 네온 줄 · 칵테일 칸 · 스킬 버튼 · 시작 버튼. 잔 다섯과 'N웨이브 후 획득' 은 지우고 게임이 그린다.
    # rpanel_ko 는 한국어 글자(칵테일 · 스 킬 · 시 작)를 남긴 그림, rpanel 은 글자까지 지운 그림 (다른 언어 · 웨이브 중 시작 버튼은 코드 글자)
    'rpanel_ko': (1618, 632, 1992, 1115),
    'rpanel': (1618, 632, 1992, 1115),
}
ERASE = {  # 그림에서 지울 곳 (시안 좌표, 채울 색)
    'sign': [((882, 22, 1166, 119), (2, 8, 23))],        # 상자 안 숫자 '3/20'
    'tv': [((1652, 165, 1953, 508), (5, 13, 21)),        # 화면 안쪽 (방송 내용)
           ((1922, 38, 1992, 102), (6, 9, 21))],         # 오른쪽 위 '?' (게임이 따로 그림)
    # 채울 색 자리에 정수 X 를 주면 줄마다 그 열의 색으로 채운다 (가로로 같은 바탕 · 그라데이션 칸)
    'rpanel_ko': [((1634, 745, 1980, 834), 1628),        # 잔 다섯
                  ((1838, 704, 1972, 734), 1628)],       # '3웨이브 후 획득'
    'rpanel': [((1634, 745, 1980, 834), 1628), ((1838, 704, 1972, 734), 1628),
               ((1756, 668, 1852, 710), 1628),           # '칵테일'
               ((1765, 905, 1880, 968), 1742),           # '스 킬'
               ((1748, 1008, 1900, 1095), 1728)],        # '시 작'
}


def to_alpha(img):
    a = np.asarray(img).astype(np.float32) / 255.0
    edge = np.concatenate([a[0, :], a[-1, :], a[:, 0], a[:, -1]])
    bg = np.median(edge, axis=0)                                   # 가장자리 바탕색
    lift = np.clip((a - bg[None, None, :]) / np.maximum(1e-3, 1.0 - bg[None, None, :]), 0.0, 1.0)
    alpha = lift.max(axis=2)
    alpha = np.clip((alpha - 0.006) / 0.994, 0.0, 1.0)            # 바탕 잡음만 살짝 걷기 (넓은 빛번짐은 남긴다, D-412)
    pm = np.clip(a - bg[None, None, :] * (1.0 - alpha[..., None]), 0.0, 1.0)
    pm = np.minimum(pm, alpha[..., None])                          # 미리 곱한 색은 알파를 넘지 않는다
    out = np.dstack([pm, alpha])
    return Image.fromarray((out * 255.0 + 0.5).astype(np.uint8), 'RGBA')


def main():
    os.makedirs(OUT, exist_ok=True)
    ref = Image.open(REF).convert('RGB')
    if ref.size != (2000, 1125): ref = ref.resize((2000, 1125), Image.LANCZOS)
    for name, box in CUTS.items():
        part = ref.crop(box)
        from PIL import ImageDraw
        for (x0, y0, x1, y1), col in ERASE.get(name, []):
            if isinstance(col, int):  # 줄마다 X 열의 색으로
                for y in range(y0, y1 + 1):
                    c = ref.getpixel((col, y))
                    ImageDraw.Draw(part).line([(x0 - box[0], y - box[1]), (x1 - box[0], y - box[1])], fill=c)
            else:
                ImageDraw.Draw(part).rectangle([x0 - box[0], y0 - box[1], x1 - box[0], y1 - box[1]], fill=col)
        to_alpha(part).save(os.path.join(OUT, name + '.png'))
        print(name, box)


if __name__ == '__main__':
    sys.exit(main())
