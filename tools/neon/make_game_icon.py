"""게임 아이콘 만들기 (D-501): L3 시안 = 필기체 Alice + 왼쪽 위 BAR (10도), 48px 이하는 A 하나

    python3 tools/neon/make_game_icon.py

크기마다 따로 그려(game_icon_mock.render) 아래 파일을 만든다:
- Build/Windows/Application.ico          (16 · 24 · 32 · 48 · 64 · 128 · 256)
- Build/Mac/Application.icns             (16 ~ 1024, 옛 Xcode 방식)
- Build/Mac/Resources/Assets.xcassets/AppIcon.appiconset/  (새 Xcode 방식, png + Contents.json)
- DOC/assets/images/game-icon.png        (1024 미리보기)
"""
import json
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
import game_icon_mock as G  # noqa: E402

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
NAME = 'L3'


def main():
    G.ICONS = G.SET4
    img = {n: G.render(NAME, n) for n in (16, 24, 32, 48, 64, 128, 256, 512, 1024)}

    win = os.path.join(ROOT, 'Build', 'Windows')
    os.makedirs(win, exist_ok=True)
    ico = [16, 24, 32, 48, 64, 128, 256]
    img[256].save(os.path.join(win, 'Application.ico'), sizes=[(n, n) for n in ico], append_images=[img[n] for n in ico if n != 256])

    mac = os.path.join(ROOT, 'Build', 'Mac')
    os.makedirs(mac, exist_ok=True)
    img[1024].save(os.path.join(mac, 'Application.icns'), append_images=[img[n] for n in (16, 32, 64, 128, 256, 512)])

    aset = os.path.join(mac, 'Resources', 'Assets.xcassets', 'AppIcon.appiconset')
    os.makedirs(aset, exist_ok=True)
    entries = []
    for pt in (16, 32, 128, 256, 512):
        for sc in (1, 2):
            fn = f'icon_{pt}x{pt}{"@2x" if sc == 2 else ""}.png'
            img[pt * sc].save(os.path.join(aset, fn))
            entries.append({'idiom': 'mac', 'size': f'{pt}x{pt}', 'scale': f'{sc}x', 'filename': fn})
    with open(os.path.join(aset, 'Contents.json'), 'w') as f:
        json.dump({'images': entries, 'info': {'version': 1, 'author': 'xcode'}}, f, indent=2)
    with open(os.path.join(mac, 'Resources', 'Assets.xcassets', 'Contents.json'), 'w') as f:
        json.dump({'info': {'version': 1, 'author': 'xcode'}}, f, indent=2)

    img[1024].save(os.path.join(ROOT, 'DOC', 'assets', 'images', 'game-icon.png'))


if __name__ == '__main__':
    main()
