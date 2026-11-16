"""채널 TV (D-316, D-318) · 주크박스 간판 (D-319) 시안

    python3 tools/neon/tv_jukebox_mock.py 출력폴더   → tv_channels.png (CH1 · CH2 · CH3 · 숨은 채널) · jukebox_sign.png
모든 채널 아래에 같은 방송 자막 띠(전구 16 + 남은 적 + 처치 · 놓침). 판 위력 숫자 · 타워 순위는 뺐다.
"""
import math
import os
import random
import sys

from PIL import ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, PINK, CYAN, GOLD  # noqa: E402
from neon import circle, suit  # noqa: E402

KO = '/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc'
LAV, MONO, DIM, BULB = (168, 150, 225), (222, 232, 245), (95, 104, 120), (255, 225, 150)


def tv(M, T, x, y, ch, title):
    w, h = 290, 226
    M.rr(x, y, w, h, 16, LAV, 2.0, 0.8)
    sx, sy, sw, sh = x + 12, y + 12, w - 52, h - 24
    M.rr(sx, sy, sw, sh, 22, LAV, 1.4, 0.5)
    M.ln([(x + w - 54, y), (x + w - 72, y - 22)], LAV, 1.1, 0.5); M.ln([(x + w - 54, y), (x + w - 34, y - 21)], LAV, 1.1, 0.5)
    kx, ky = x + w - 22, y + h - 50
    M.ln(circle(kx, ky, 8, 20), (255, 255, 255), 1.4, 0.8)
    a = math.radians(-150 + ch * 60); M.ln([(kx, ky), (kx + math.cos(a) * 7, ky + math.sin(a) * 7)], (255, 255, 255), 1.4, 1.0)
    M.ln(circle(kx, ky + 26, 8, 20), LAV, 1.2, 0.4)
    T.append((x + w - 22, y + 100, '%02d' % (ch + 1) if ch < 3 else '??', MONO, 10, 'mm'))
    T.append((x + w / 2, y - 34, title, MONO, 13, 'mm'))
    cx0, cw = sx + 10, sw - 20
    tky = sy + sh - 40
    M.ln([(sx + 8, tky), (sx + sw - 8, tky)], MONO, 1.0, 0.35)
    T.append((cx0, tky + 13, '남은 적 23/92', MONO, 10, 'lm')); T.append((cx0 + cw, tky + 13, '처치 412 · 놓침 2', MONO, 9, 'rm'))
    for i in range(16):
        b = 1.0 if i < 4 else 0.15
        M.ln(circle(cx0 + cw / 16 * (i + 0.5), tky + 26, 2.6, 10), BULB, 1.2, b)
    T.append((sx + sw - 12, sy + 20, ['CH1', 'CH2', 'CH3', 'CH ??'][ch], MONO, 9, 'rm'))
    return sx, sy, sw, sh, cx0, cw, tky


def main(out):
    os.makedirs(out, exist_ok=True)
    M, T = Sign(), []
    titles = ['CH1 다음 웨이브', 'CH2 포커 중계', 'CH3 지직', '숨은 채널 (다이얼 빠르게 7번)']
    for ch in range(4):
        x = 20 + ch * 315
        sx, sy, sw, sh, cx0, cw, tky = tv(M, T, x, 140, ch, titles[ch])
        if ch == 0:
            T.append((cx0, sy + 20, '다음 웨이브 15', MONO, 11, 'lm'))
            rows = [('재생 왕', 1, True), ('병정', 48, False), ('탱크', 12, False), ('빠른 병정', 20, False)]
            yy = sy + 42
            for nm, n, boss in rows:
                M.rr(cx0 + 4, yy - 8, 12, 16, 3, MONO, 1.2, 1.0 if boss else 0.7)
                T.append((cx0 + 26, yy, nm, MONO, 10, 'lm')); T.append((cx0 + cw, yy, '×%d' % n, MONO, 10, 'rm'))
                if boss: T.append((cx0 + 26, yy + 14, '체력이 계속 차오른다', DIM, 9, 'lm')); yy += 14
                yy += 20
        elif ch == 1:
            T.append((cx0, sy + 20, 'LIVE · LATE NIGHT POKER', MONO, 9, 'lm'))
            for k, r in enumerate(['10', 'J', 'Q', 'K', 'A']):
                cx = cx0 + 12 + k * (cw - 24) / 4
                M.rr(cx - 12, sy + 40, 24, 34, 3, MONO, 1.0, 0.9)
                T.append((cx, sy + 52, r, MONO, 10, 'mm'))
            T.append((sx + sw / 2, sy + 110, 'ROYAL FLUSH!', MONO, 16, 'mm'))
        elif ch == 2:
            rnd = random.Random(4)
            for _ in range(300):
                px, py = rnd.uniform(sx + 6, sx + sw - 8), rnd.uniform(sy + 6, tky - 4)
                M.ln([(px, py), (px + rnd.uniform(1, 3), py)], MONO, 1.0, rnd.uniform(0.2, 0.7))
        else:
            T.append((sx + sw / 2, sy + 50, 'BAR', CYAN, 22, 'mm')); T.append((sx + sw / 2, sy + 80, 'Alice', PINK, 24, 'mm'))
            T.append((sx + sw / 2, tky - 14, 'LATE NIGHT POKER', MONO, 9, 'mm'))
    T.append((640, 60, '채널 TV: 오른쪽 아래 다이얼을 누르면 채널이 바뀐다 (딸깍 + 0.2초 지직, 고른 채널 저장)', MONO, 16, 'mm'))
    img = M.draw(); d = ImageDraw.Draw(img)
    for (x, y, s, c, px, a) in T: d.text((x, y), s, fill=c, font=ImageFont.truetype(KO, px), anchor=a)
    img.save(os.path.join(out, 'tv_channels.png'))
    # 주크박스 간판 (3배 크기로)
    M, T = Sign(), []
    k, x, y, w, h = 3.0, 640 - 91 * 3.0, 200, 182 * 3.0, 68 * 3.0
    for wx in (x + 16 * k, x + w - 16 * k): M.ln([(wx, 60), (wx, y)], DIM, 1.0, 0.5)
    M.rr(x, y, w, h, 10 * k, LAV, 2.4, 0.95)
    M.rr(x + 5 * k, y + 5 * k, w - 10 * k, h - 10 * k, 5 * k, LAV, 1.2, 0.5)
    T.append((x + w / 2, y + 15 * k, 'NOW PLAYING', GOLD, 26, 'mm'))
    T.append((x + w / 2, y + 37 * k, '1. Last Call Swing', MONO, 30, 'mm'))
    for d_ in (-1, 1):
        bx = x + 8 * k if d_ < 0 else x + w - 30 * k
        M.rr(bx, y + 26 * k, 22 * k, 22 * k, 11 * k, PINK, 1.6, 0.8)
        c = (bx + 11 * k, y + 37 * k)
        M.ln([(c[0] + d_ * 12, c[1]), (c[0] - d_ * 9, c[1] - 15), (c[0] - d_ * 9, c[1] + 15), (c[0] + d_ * 12, c[1])], MONO, 1.4, 1.0)
    for i in range(12):
        M.ln(circle(x + 18 * k + i * (w - 36 * k) / 11, y + h - 9 * k, 2.6 * k, 12), BULB, 1.4, 1.0 if i % 4 == 1 else 0.15)
    T.append((640, 620, '주크박스 간판 (예전 남은 적 간판 자리, 3배 확대): 흐르는 곡 이름 · ◀ ▶ 곡 넘기기 · 박자에 맞춰 옮겨 가는 전구', MONO, 16, 'mm'))
    img = M.draw(); d = ImageDraw.Draw(img)
    for (x2, y2, s, c, px, a) in T: d.text((x2, y2), s, fill=c, font=ImageFont.truetype(KO, px), anchor=a)
    img.save(os.path.join(out, 'jukebox_sign.png'))
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
