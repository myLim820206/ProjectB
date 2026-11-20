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
    'sign': (405, 0, 871, 126),
    'logo': (32, 27, 180, 84),
    'glass_on': (1634, 748, 1702, 829),
    'glass_off': (1838, 748, 1906, 829),
}


def to_alpha(img):
    a = np.asarray(img).astype(np.float32) / 255.0
    edge = np.concatenate([a[0, :], a[-1, :], a[:, 0], a[:, -1]])
    bg = np.median(edge, axis=0)                                   # 가장자리 바탕색
    lift = np.clip((a - bg[None, None, :]) / np.maximum(1e-3, 1.0 - bg[None, None, :]), 0.0, 1.0)
    alpha = lift.max(axis=2)
    alpha = np.clip((alpha - 0.03) / 0.97, 0.0, 1.0)              # 바탕 잡음 걷기
    pm = np.clip(a - bg[None, None, :] * (1.0 - alpha[..., None]), 0.0, 1.0)
    pm = np.minimum(pm, alpha[..., None])                          # 미리 곱한 색은 알파를 넘지 않는다
    out = np.dstack([pm, alpha])
    return Image.fromarray((out * 255.0 + 0.5).astype(np.uint8), 'RGBA')


def main():
    os.makedirs(OUT, exist_ok=True)
    ref = Image.open(REF).convert('RGB')
    if ref.size != (2000, 1125): ref = ref.resize((2000, 1125), Image.LANCZOS)
    for name, box in CUTS.items():
        to_alpha(ref.crop(box)).save(os.path.join(OUT, name + '.png'))
        print(name, box)


if __name__ == '__main__':
    sys.exit(main())
