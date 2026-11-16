"""주크박스 시안 3 (사용자 2026-11-16: 실제 주크박스처럼, 안에서 곡 정보 · 이전/다음 곡, 움직이게) — PDBoardTv.cpp PaintJukebox 를 그대로 옮긴 움직이는 GIF

    python3 tools/neon/jukebox_mock.py 출력폴더   → jukebox.gif (3.2배 확대, 1.2초에 ▶ 눌러 곡이 바뀜) + jukebox.png (한 장)
본체 하나(128 × 94): 위 아치 무지개 튜브 셋(박자에 맞춰 차례로 밝아짐) + 둘레를 도는 전구, 양옆 거품 튜브,
유리창(도는 레코드 · 집게 팔 · NOW PLAYING · 곡 이름), 선곡 버튼 ◀ 번호 ▶, 스피커 그릴(박자에 맞춰 떨림), 받침.
곡을 바꾸면 0.7초 동안 레코드가 옆으로 빠졌다 새 레코드가 들어오고 팔이 들렸다 내려온다.
"""
import math
import os
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, PINK, CYAN, GOLD  # noqa: E402
from neon import circle  # noqa: E402

KO = '/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc'
LAV, FG, BULB = (168, 150, 225), (220, 228, 240), (255, 225, 150)
TRACKS = [('Last Call Swing', 80), ('Neon Rain Bossa', 128), ('After Hours Blues', 96), ('Empty Glass Ballad', 66)]  # PDSynth.cpp GJazz
JW, JH = 128, 94


def jukebox(M, T, X0, Y0, k, clock, track, sw_at, sw_dir, hover=0, boss=False):
    """X0, Y0 = 본체 왼쪽 위, k = 배율. PaintJukebox 와 같은 수치"""
    bpm = 132 if boss else TRACKS[track][1]
    bn = clock * bpm / 60
    beat = max(0.0, math.cos(bn * math.tau))
    P = lambda x, y: (X0 + x * k, Y0 + y * k)  # noqa: E731
    W, CX, Bot, ArchH = JW, JW / 2, JH, 26.0
    ArchY = ArchH

    def arch(inn, down):
        rx, ry = W / 2 - inn, ArchH - inn
        pts = [P(CX - rx, ArchY + down)]
        pts += [P(CX + math.cos(math.pi + a * math.pi / 24) * rx, ArchY + math.sin(math.pi + a * math.pi / 24) * ry) for a in range(25)]
        pts.append(P(CX + rx, ArchY + down))
        return pts

    body = arch(0, Bot - 6 - ArchY); body.append(body[0])
    M.ln(body, LAV, 1.6, 0.8)
    M.ln([P(-3, Bot - 6), P(W + 3, Bot - 6), P(W + 1, Bot), P(-1, Bot), P(-3, Bot - 6)], LAV, 1.3, 0.6)
    for i, c in enumerate((PINK, GOLD, CYAN)):
        br = 0.7 + 0.3 * max(0.0, math.cos((bn - i * 0.33) * math.tau))
        M.ln(arch(4 + i * 3.5, Bot - 14 - ArchY), c, 1.3, 0.85 * br)
    for i in range(11):
        t = math.pi + (i + 0.5) * math.pi / 11
        on = (i + math.floor(bn * 2)) % 4 == 0
        bx, by = P(CX + math.cos(t) * (W / 2 + 3), ArchY + math.sin(t) * (ArchH + 3))
        M.ln(circle(bx, by, 1.7 * k, 8), (255, 106, 106) if boss else BULB, 1.0, 1.0 if on else 0.15)
    for s in (-1, 1):
        tx = CX + s * (W / 2 - 17)
        for kk in range(4):
            u = (clock * 0.6 + kk * 0.25 + (0.12 if s > 0 else 0)) % 1
            cx, cy = P(tx, Bot - 16 - u * 44)
            M.ln(circle(cx, cy, (1.3 + 0.6 * u) * k, 8), PINK if s < 0 else CYAN, 1.0, 0.9 * (1 - u))
    # 유리창
    GX, GY, GW, GH = 24, ArchY + 2, W - 48, 30
    gx, gy = P(GX, GY); M.rr(gx, gy, GW * k, GH * k, 6 * k, FG, 1.1, 0.45)
    sw = clock - sw_at; su = min(max(sw / 0.7, 0), 1)
    rk = (su * 2 if su < 0.5 else (1 - su) * 2) if sw < 0.7 else 0  # 레코드가 밑으로 내려가며 흐려졌다 새 레코드가 올라옴
    rc = P(GX + 12, GY + GH / 2 + rk * 8); ra_ = 1 - rk
    M.ln(circle(rc[0], rc[1], 9 * k, 20), FG, 1.0, ra_)
    M.ln(circle(rc[0], rc[1], 3 * k, 10), PINK, 1.0, ra_)
    ra = clock * (bpm / 60) * math.pi * 0.5
    M.ln([(rc[0] + math.cos(ra) * 4 * k, rc[1] + math.sin(ra) * 4 * k), (rc[0] + math.cos(ra) * 8 * k, rc[1] + math.sin(ra) * 8 * k)], FG, 1.0, 0.7 * ra_)
    arm = math.radians(70 - 50 * (1 - abs(su * 2 - 1)) if sw < 0.7 else 40)
    pv = P(GX + 26, GY + 4)
    M.ln([pv, (pv[0] - math.cos(arm) * 12 * k, pv[1] + math.sin(arm) * 12 * k)], GOLD, 1.1, 1.0)
    tx = GX + 30
    T.append((*P(tx, GY + 9), 'BOSS THEME' if boss else 'NOW PLAYING', (255, 106, 106) if boss else GOLD, int(7 * k), 'ls'))
    if sw >= 0.35:
        nm = '보스 웨이브' if boss else TRACKS[track][0]
        f = ImageFont.truetype(KO, int(8 * k)); nw = f.getlength(nm) / k; tw = GW - 34  # 유리창 안 글 폭 (설계 단위)
        clip = (*P(tx, GY + 12), *P(GX + GW - 3, GY + GH - 2))
        if nw <= tw: T.append((*P(tx, GY + 22), nm, FG, int(8 * k), 'ls', clip))
        else:  # 길면 흐른다 (PaintJukebox 와 같이 22/초)
            loop = nw + 24; o = (max(0.0, sw - 1.2) * 22) % loop  # 곡을 바꾸면 처음부터, 1초쯤 멈췄다 흐름
            for kk in range(2): T.append((*P(tx - o + kk * loop, GY + 22), nm, FG, int(8 * k), 'ls', clip))
    # 선곡 버튼
    BS, BY = 16, 62
    for d in (-1, 1):
        btx = W / 2 + d * 30 - BS / 2
        press = sw < 0.15 and sw_dir == d
        hov = hover == d or press
        dy = 1.5 if press else 0
        x, y = P(btx, BY + 2 + dy)
        M.rr(x, y, BS * k, (BS - 4) * k, (BS - 4) / 2 * k, PINK, 1.4 if hov else 1.1, 0.9 if hov else 0.4)
        cc = (btx + BS / 2, BY + BS / 2 + dy)
        M.ln([P(cc[0] + d * 3, cc[1]), P(cc[0] - d * 2, cc[1] - 3.5), P(cc[0] - d * 2, cc[1] + 3.5), P(cc[0] + d * 3, cc[1])], (255, 255, 255) if hov else FG, 1.1, 1.0)
    for i in range(len(TRACKS)):
        kx = CX - (len(TRACKS) - 1) * 4.5 + i * 9
        x, y = P(kx - 3, BY + 5)
        M.rr(x, y, 6 * k, 6 * k, 1.5 * k, GOLD if i == track else LAV, 1.0, 0.9 if i == track else 0.2)
    for i in range(9):
        sx = CX - 32 + i * 8
        lh = 8 + 3 * beat * (1 if i % 2 else 0.6)
        M.ln([P(sx, Bot - 11 - lh), P(sx, Bot - 11)], LAV, 1.0, 0.5 + 0.3 * beat)


def frame(clock, track, sw_at, sw_dir, hover, k=3.2, boss=False):
    M, T = Sign(), []
    X0, Y0 = 40, 40
    jukebox(M, T, X0, Y0, k, clock, track, sw_at, sw_dir, hover, boss)
    img = M.draw(); d = ImageDraw.Draw(img)
    for t in T:
        (x, y, s, c, px, a), clip = t[:6], (t[6] if len(t) > 6 else None)
        if clip is None: d.text((x, y), s, fill=c, font=ImageFont.truetype(KO, px), anchor=a); continue
        lay = Image.new('RGB', img.size, (0, 0, 0)); ImageDraw.Draw(lay).text((x, y), s, fill=c, font=ImageFont.truetype(KO, px), anchor=a)
        box = tuple(int(v) for v in clip); img.paste(ImageChops.lighter(img.crop(box), lay.crop(box)), box[:2])
    return img.crop((0, 0, int(JW * k + 80), int(JH * k + 70)))


def main(out):
    os.makedirs(out, exist_ok=True)
    frames, dt, n = [], 0.05, 110
    for f in range(n):
        c = f * dt
        hover = 1 if 0.8 <= c < 1.2 else 0          # 마우스를 ▶ 에 올렸다가
        track, sw_at = (2, 1.2) if c >= 1.2 else (0, -1.2)  # 1.2초에 누름 → 다음 곡
        frames.append(frame(c, track, sw_at, 1, hover).convert('P', palette=1, colors=255))
    frames[0].save(os.path.join(out, 'jukebox.gif'), save_all=True, append_images=frames[1:], duration=int(dt * 1000), loop=0)
    frame(0.3, 0, -10, 1, 0).save(os.path.join(out, 'jukebox.png'))
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
