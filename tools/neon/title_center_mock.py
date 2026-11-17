"""시작 화면 다듬기 시안 (외부 피드백 반영, D-381 예정)

    python tools/neon/title_center_mock.py 출력폴더   → title_center_mock.png · title_btn_rule.png

피드백 4가지:
1 버튼 색 규칙: 기본 = 하늘색 하나, 주 행동(시작) = 초록, 나가기(종료) = 분홍
2 마우스 올림: 다른 색으로 바꾸지 않고 자기 색 안에서 밝아짐 + 버튼 모양 그대로 밑에 은은한 빛 판
3 아래 빈 공간(바닥 반사 자리) 없앰: 벽돌 벽을 화면 전체에, 로고와 버튼 묶음을 위아래 가운데로
4 유리 바닥 반사 뺌
"""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, CYAN, GOLD, WHITE, FONT_EN, hx  # noqa: E402
from logo_alice import logo_C  # noqa: E402
from neon import suit, circle  # noqa: E402
import logo_alice  # noqa: E402

logo_alice.FONT = FONT_EN  # 리눅스 글꼴 경로 대신 (윈도에서 그림)
GREEN, PINKB, DIM = hx('9cffb0'), hx('ff6aaa'), hx('5f6878')
BTN = [('Start Game', GREEN), ('Permanent Upgrades', CYAN), ('Unlocks', CYAN), ('Settings', CYAN), ('Quit Game', PINKB)]
BW, BH, GAP = 300, 44, 16


def pad(M, x, y, w, h, c, b):
    """마우스 올림 빛 판: 버튼과 같은 둥근 모양을 겹겹이 넓혀 아주 옅게 (가운데가 조금 더 밝게)"""
    for i in range(6):
        e = 2 + i * 2
        M.rr(x - e, y - e, w + 2 * e, h + 2 * e, (h + 2 * e) / 2, c, 6.0, b * (1.0 - i / 6.0))
    M.ln([(x + h / 2, y + h / 2), (x + w - h / 2, y + h / 2)], c, h - 6, b * 1.3)  # 속 채움


def button(M, x, y, label, c, hov):
    """튜브 버튼 (D-254 간판 테두리): 바깥 튜브 + 안쪽 가는 튜브 + 양 끝 볼트. hov = 같은 색으로 밝게 + 빛 판"""
    if hov: pad(M, x, y, BW, BH, c, 0.10)
    else: M.ln([(x + BH / 2, y + BH / 2), (x + BW - BH / 2, y + BH / 2)], c, BH - 8, 0.07)  # 평소 옅은 채움
    M.rr(x, y, BW, BH, BH / 2, c, 2.4 if hov else 1.5, 1.0 if hov else 0.6)
    M.rr(x + 5, y + 5, BW - 10, BH - 10, (BH - 10) / 2, c, 1.0, 0.8 if hov else 0.3)
    for bx in (x + 4 + BH / 4, x + BW - 4 - BH / 4):
        M.ln(circle(bx, y + BH / 2, 2.0, 10), (120, 125, 140), 1.0, 0.6)
    # 글자도 같은 색 (밝기만 다름): 흰색으로 바뀌지 않는다
    M.word(x + BW / 2, y + BH / 2 + 7, label, c, 18, [1.0 if hov else 0.72] * len(label), font=FONT_EN)


def bricks(M):
    BR = (60, 48, 58)
    for r in range(0, 22):
        y = 20 + r * 34
        M.ln([(0, y), (1280, y)], BR, 1.0, 0.3)
        for k in range(0, 22):
            x = k * 64 + (32 if r % 2 else 0)
            M.ln([(x, y), (x, y + 34)], BR, 1.0, 0.3)


def title(hov=2):
    M = Sign()
    bricks(M)  # 화면 전체 벽 (바닥 경계선 · 반사 없음)
    logo_C(M, 400, 205, 1.5)  # 로고 묶음 위아래 가운데 (높이 ~315 → 가운데 360)
    n = len(BTN)
    y0 = 360 - (n * BH + (n - 1) * GAP) / 2
    for i, (lb, c) in enumerate(BTN):
        y = y0 + i * (BH + GAP)
        button(M, 830, y, lb, c, i == hov)
        if i == hov:
            for p in suit('heart')(830 - 24, y + BH / 2, 8): M.ln(p, c, 1.4)  # 고른 줄 표시도 그 버튼 색
    M.rr(1090, 22, 150, 32, 16, GOLD, 1.4)
    for p in suit('diamond')(1108, 38, 6): M.ln(p, GOLD, 1.2)
    M.word(1170, 45, 'Chips 0', GOLD, 15, None, font=FONT_EN)
    M.word(640, 700, 'Up/Down or mouse to choose · Enter or click to confirm', DIM, 11, None, font=FONT_EN)
    return M.draw()


def rule():
    """버튼 규칙표: 세 가지 색 × (평소 / 마우스 올림)"""
    M = Sign(0.9)
    M.word(640, 60, 'Button colors: Default = cyan · Primary = green · Exit = pink', WHITE, 18, None, font=FONT_EN)
    for j, (nm, c) in enumerate((('Default', CYAN), ('Primary', GREEN), ('Exit', PINKB))):
        x = 120 + j * 360
        M.word(x + BW / 2, 130, nm, c, 16, None, font=FONT_EN)
        button(M, x, 160, 'Normal', c, False)
        button(M, x, 240, 'Hover', c, True)
    M.word(640, 340, 'Hover keeps its own color: brighter tube + soft pad of the same shape underneath', DIM, 13, None, font=FONT_EN)
    return M.draw().crop((0, 20, 1280, 380))


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    title().save(os.path.join(outdir, 'title_center_mock.png'))
    rule().save(os.path.join(outdir, 'title_btn_rule.png'))
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
