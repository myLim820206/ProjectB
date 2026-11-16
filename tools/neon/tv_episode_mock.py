"""채널 TV CH1 '다음 화 예고' 움직이는 시안 (사용자 2026-11-16, D-344) — PDBoardTv.cpp PaintTv CH1 을 옮김

    python3 tools/neon/tv_episode_mock.py 출력폴더   → tv_episode.gif (2배 확대)
흐름: 제4화 예고 "병정들의 행진" (한 글자씩) → 웨이브 4 끝, 지직 → 제5화 예고 "방패 거인의 습격" → 시작, '지금 방송 중' 빨간 점 + 자막 띠 전구가 줄어듦
"""
import math
import os
import random
import sys

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign  # noqa: E402
from neon import circle  # noqa: E402
import soldiers  # noqa: E402

KO = '/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc'
LAV, MONO, GOLD, RED, GREEN, BULB = (168, 150, 225), (222, 232, 245), (255, 210, 110), (255, 138, 138), (156, 255, 176), (255, 225, 150)
K = 2.0  # 확대
PW, PH = 220, 226  # 오른쪽 패널 폭 · TV 높이 (PowCardH)
KIND = {k[0]: k for k in soldiers.KINDS + soldiers.BOSSES}
EP = [  # (화, 제목, 줄들[(이름, 마리, 보스, 모양, 능력)])
    (4, '병정들의 행진', [('병정', 30, False, '보통', ''), ('빠른 병정', 8, False, '빠름', '')]),
    (5, '방패 거인의 습격', [('방패 거인', 1, True, '방패 거인', '정면 공격을 막는 방패'), ('병정', 24, False, '보통', ''), ('탱크', 6, False, '탱크', '')]),
]


def mono(c, a): return tuple(int(v * a) for v in c)


def frame(t):
    # 장면: 0~2.6 제4화 예고 · 2.6~2.8 지직 · 2.8~5.2 제5화 예고 · 5.2~ 지금 방송 중
    M, T = Sign(), []
    X0, Y0 = 20, 30
    P = lambda x, y: (X0 + x, Y0 + y)  # noqa: E731
    M.rr(*P(0, 0), PW, PH, 16, LAV, 2.0, 0.8)
    SX, SY, SW, SH = 12, 12, PW - 52, PH - 24
    M.rr(*P(SX, SY), SW, SH, 22, LAV, 1.5, 0.5)
    M.rr(*P(SX + 4, SY + 4), SW - 8, SH - 8, 18, (156, 198, 255), 2.0, 0.5)
    bx = PW - 54
    M.ln([P(bx - 3, -6), P(bx - 18, -22)], LAV, 1.1, 0.5); M.ln([P(bx + 3, -6), P(bx + 20, -21)], LAV, 1.1, 0.5)
    M.ln([P(bx - 10, 0), P(bx + 10, 0), P(bx + 6, -6), P(bx - 6, -6), P(bx - 10, 0)], LAV, 1.3, 0.5)
    for k in range(4): M.ln([P(PW - 30 + k * 5, 30), P(PW - 30 + k * 5, 70)], LAV, 1.0, 0.4)
    T.append((*P(PW - 22, 88), 'CH', LAV, 7, 'mm')); T.append((*P(PW - 22, 100), '01', MONO, 9, 'mm'))
    kc = P(PW - 22, PH - 50)
    M.ln(circle(*kc, 8, 20), LAV, 1.2, 0.4); a = math.radians(-150); M.ln([kc, (kc[0] + math.cos(a) * 7, kc[1] + math.sin(a) * 7)], LAV, 1.4, 1.0)
    M.ln(circle(kc[0], kc[1] + 26, 8, 20), LAV, 1.2, 0.4)
    CX0, CW = SX + 10, SW - 20
    TkH = 34; TkY = SY + SH - 6 - TkH
    noise = 2.6 <= t < 2.8
    if noise:
        rnd = random.Random(int(t * 60))
        for _ in range(260):
            x, y = rnd.uniform(SX + 6, SX + SW - 8), rnd.uniform(SY + 6, SY + SH - 8)
            M.ln([P(x, y), P(x + rnd.uniform(1, 3), y)], MONO, 1.0, rnd.uniform(0.2, 0.7))
    else:
        ep = EP[0] if t < 2.6 else EP[1]
        t0 = 0.0 if t < 2.6 else 2.8
        now = t >= 5.2
        tag = ('지금 방송 중 · 제%d화' if now else '다음 화 예고 · 제%d화') % ep[0]
        T.append((*P(CX0 + (9 if now else 0), SY + 20), tag, RED if now else GOLD, 9, 'ls'))
        if now and (t * 1.2) % 1 < 0.6: M.ln(circle(*P(CX0 + 2.5, SY + 16.5), 2, 8), (255, 106, 106), 2.0, 1.0)
        T.append((*P(SX + SW - 12, SY + 20), 'CH1', mono(MONO, 0.8), 9, 'rs'))
        ttl = ep[1]; n = max(0, min(len(ttl), int((t - t0) * 14)))
        s = '“' + ttl[:n] + ('_' if n < len(ttl) and (t * 4) % 1 < 0.5 else '') + ('”' if n >= len(ttl) else '')
        T.append((*P(CX0 + CW / 2, SY + 42), s, (255, 255, 255) if ep[2][0][2] else MONO, 13, 'ms'))
        M.ln([P(CX0 + CW * 0.2, SY + 50), P(CX0 + CW * 0.8, SY + 50)], MONO, 1.0, 0.35)
        y = SY + 58
        for nm, cnt, boss, shape, skill in ep[2]:
            for pts, c, wd in soldiers.soldier(KIND[shape], X0 + CX0 + 10, Y0 + y + 8 + (9 if boss else 7) * 0.5, 9 if boss else 7, (t * 1.5) % 1):
                M.ln(pts, mono(MONO, 1.0 if boss else 0.85), 1.0, 1.0 if boss else 0.85)
            T.append((*P(CX0 + 26, y + 4), nm, mono(MONO, 1.0 if boss else 0.88), 10, 'ls'))
            T.append((*P(CX0 + CW, y + 4), '×%d' % cnt, mono(MONO, 0.9), 10, 'rs'))
            if boss: T.append((*P(CX0 + 26, y + 17), skill, mono(MONO, 0.65), 8, 'ls')); y += 13
            y += 18
        # 자막 띠
        M.ln([P(SX + 8, TkY), P(SX + SW - 8, TkY)], MONO, 1.0, 0.35)
        if now:
            tot = 31; left = max(0, int(tot - (t - 5.2) * 9)); frac = left / tot
            msg, col = '남은 적 %d/%d' % (left, tot), MONO
        elif t >= 1.2:
            sec = (2.6 - t) if t < 2.6 else (5.2 - t); frac = 1 - sec / 3.0
            msg, col = '%d초 뒤 자동' % math.ceil(sec), GREEN
        else:
            frac, msg, col = 0, '웨이브 %d 준비' % ep[0], MONO
        T.append((*P(CX0, TkY + 13), msg, col, 10, 'ls'))
        T.append((*P(CX0 + CW, TkY + 13), '처치 %d · 놓침 0' % (96 + (int((t - 5.2) * 9) if now else 0)), mono(MONO, 0.7), 9, 'rs'))
        on = round(16 * frac)
        for i in range(16):
            M.ln(circle(*P(CX0 + CW / 16 * (i + 0.5), TkY + 25), 2.6, 10), GREEN if (col == GREEN) else BULB, 1.2, 1.0 if i < on else 0.15)
    img = M.draw()
    # 글자 + 흑백 TV 결 (주사선 · 굴러가는 띠)
    d = ImageDraw.Draw(img)
    for (x, y, s, c, px, a) in T: d.text((x, y), s, fill=c, font=ImageFont.truetype(KO, px), anchor=a)
    ov = Image.new('RGBA', img.size, (0, 0, 0, 0)); od = ImageDraw.Draw(ov)
    for ly in range(int(Y0 + SY + 6), int(Y0 + SY + SH - 6), 3): od.line([(X0 + SX + 6, ly), (X0 + SX + SW - 6, ly)], fill=(0, 0, 0, 80))
    band = Y0 + SY + (t * 22) % (SH - 20) + 6
    for k in range(12): od.line([(X0 + SX + 6, band + k), (X0 + SX + SW - 6, band + k)], fill=(222, 232, 245, int(9 * (1 - abs(k - 6) / 6))))
    img = Image.alpha_composite(img.convert('RGBA'), ov).convert('RGB')
    box = (0, 0, X0 * 2 + PW, Y0 + PH + 20)
    return img.crop(box).resize((int(box[2] * K), int(box[3] * K)), Image.LANCZOS)


def main(out):
    os.makedirs(out, exist_ok=True)
    dt = 0.05
    frames = [frame(i * dt).convert('P', palette=1, colors=255) for i in range(int(7.0 / dt))]
    frames[0].save(os.path.join(out, 'tv_episode.gif'), save_all=True, append_images=frames[1:], duration=int(dt * 1000), loop=0)
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
