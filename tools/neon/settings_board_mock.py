"""환경 설정 창 시안: 지금 전구 · 배치 · 색(핫핑크 테 · 하늘색 선택지 · 흰 ⓧ)은 그대로 두고 모양만 (D-393 예정)

    python tools/neon/settings_board_mock.py 출력폴더   → settings_board_A.png · settings_board_B.png

A 네모 창: 아치를 걷어낸 둥근 모서리 사각 두 겹 테, 전구는 윗변을 따라 한 줄
B 세워 둔 칠판 간판(A형 입간판): 두 겹 테 칠판 + 위 손잡이 가로대(전구 줄) + 아래로 벌어진 다리 둘과 버팀대, 칠판 바탕은 아주 어두운 초록
"""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, FONT_KO, GOLD, hx  # noqa: E402
from neon import circle  # noqa: E402

PINK, CYAN, LAV, FG, GREY, WARM = hx('ff6aaa'), hx('6edcff'), hx('a896e1'), hx('cfd8e6'), hx('8a94a6'), hx('ffe196')
SETX, LABW, ROWH, Y0 = 360, 200, 35, 140  # 게임과 같은 자리 (PDBoardDraw.h SetX · SetLabelW · SetRowH+Gap · SetY0)
BOXX, BOXW = 330, 620
ROWS = [('언어', [('한국어', 1, 66), ('English', 0, 66), ('日本語', 0, 66), ('简体中文', 0, 66), ('繁體中文', 0, 66)]),
        ('화면', [('창', 0, 112), ('전체화면', 0, 112), ('테두리 없음', 1, 112)]),
        ('카드 색', [('네온', 1, 112), ('흑백', 0, 112), ('빨강·하양', 0, 112)]),
        ('프레임 제한', [('60', 1, 112), ('120', 0, 112), ('제한 없음', 0, 112)]),
        ('수직 동기화', [('꺼짐', 0, 120)]),
        ('월드 이펙트 (나이아가라)', [('켜짐', 1, 120)]),
        ('화면 흔들림·번쩍임', [('켜짐', 1, 120)]),
        ('완성 연출', [('전부', 1, 112), ('조커만', 0, 112), ('끔', 0, 112)]),
        ('소리', [('켜짐', 1, 120)]),
        ('효과음 볼륨', 'bar'), ('음악 볼륨', 'bar'), ('곡 (주크박스)', 'track'),
        ('도움말', [('켜짐', 1, 120), ('처음부터 다시', 0, 150)])]


def fill(M, x, y, w, h, c, b):
    """옅은 채움: 굵은 선 하나를 낮은 밝기로"""
    M.ln([(x, y + h / 2), (x + w, y + h / 2)], c, h, b)


def pill(M, x, y, w, label, on, h=30):
    c = CYAN if on else LAV
    fill(M, x + h / 2, y + 3, w - h, h - 6, c, 0.10 if on else 0.05)
    M.rr(x, y, w, h, h / 2, c, 1.6 if on else 1.0, 1.0 if on else 0.45)
    M.word(x + w / 2, y + h / 2 + 5, label, c if on else GREY, 12, [1.0 if on else 0.8] * len(label), font=FONT_KO)


def rows(M, y0):
    for i, (lab, kind) in enumerate(ROWS):
        y = y0 + i * ROWH
        if i == 1: fill(M, SETX - 10, y, 580, 33, CYAN, 0.06)  # 고른 줄 띠
        M.word(SETX, y + 22, lab, FG, 14, font=FONT_KO, anchor_center=False)
        x0, by = SETX + LABW, y + 1.5
        if kind == 'bar' or kind == 'track':
            pill(M, x0, by, 40, '-' if kind == 'bar' else '◀', True)
            pill(M, x0 + 48 + 220 + 8, by, 40, '+' if kind == 'bar' else '▶', True)
            if kind == 'bar':
                M.rr(x0 + 48, by + 5, 220, 20, 4, CYAN, 1.0, 0.6)
                fill(M, x0 + 50, by + 7, 216, 16, CYAN, 0.22)
                M.word(x0 + 158, by + 20, '100%', FG, 12, font=FONT_KO)
            else:
                M.word(x0 + 158, by + 20, '2. Neon Rain Bossa', FG, 13, font=FONT_KO)
            continue
        x = x0
        for (t, on, w) in kind:
            pill(M, x, by, w, t, on)
            x += w + (6 if w == 66 else 8)


def closex(M, cx, cy, r=13):
    M.ln(circle(cx, cy, r, 28), (238, 243, 255), 1.3, 0.75)
    k = r * 0.42
    M.ln([(cx - k, cy - k), (cx + k, cy + k)], (238, 243, 255), 1.5, 0.75)
    M.ln([(cx + k, cy - k), (cx - k, cy + k)], (238, 243, 255), 1.5, 0.75)


def bulbs(M, x0, x1, y, n):
    for k in range(n):
        x = x0 + (x1 - x0) * k / (n - 1)
        if k % 3 == 1:
            M.ln(circle(x, y, 3, 10), (120, 110, 90), 1.0, 0.5)  # 꺼진 전구
        else:
            M.dots.append((x, y, 2.4))  # 켜진 전구 (흰 심 + 따뜻한 빛)


def title(M, cx, y):
    M.word(cx, y, '환경 설정', GOLD, 24, font=FONT_KO)


def board_A():
    """A 네모 창: 지금 창 그대로 아치만 걷어냄"""
    M = Sign()
    M.small_glow = 0.25
    top, bot = 49, 639
    M.rr(BOXX, top, BOXW, bot - top, 16, PINK, 2.4, 0.95)
    M.rr(BOXX + 8, top + 8, BOXW - 16, bot - top - 16, 10, PINK, 1.0, 0.45)
    bulbs(M, BOXX + 28, BOXX + BOXW - 28, top + 20, 21)
    title(M, 640, top + 58)
    closex(M, SETX + LABW + 352 - 13, top + 49 + 14)
    rows(M, Y0)
    M.word(640, 700, 'A 네모 창 — 아치 대신 둥근 모서리 사각, 전구는 윗변 한 줄', GREY, 12, font=FONT_KO)
    return M.draw()


def board_B():
    """B 세워 둔 칠판 간판 (A형 입간판, 정면)"""
    M = Sign()
    M.small_glow = 0.25
    top, bot = 64, 628  # 칠판
    x0, x1 = BOXX, BOXX + BOXW
    # 다리 (테 바깥 양옆 기둥이 위에서 모이고 아래로 벌어짐) + 버팀대
    for s, xa in ((-1, x0), (1, x1)):
        M.ln([(xa + s * 4, 28), (xa + s * 64, 712)], PINK, 2.2, 0.9)
        M.ln([(xa + s * 16, 28), (xa + s * 78, 712)], PINK, 1.0, 0.4)
        M.ln([(xa + s * 56, 708), (xa + s * 88, 712)], GREY, 1.4, 0.5)  # 발
        M.ln(circle(xa + s * 8, 30, 4, 12), GREY, 1.2, 0.7)  # 경첩
    M.ln([(x0 - 40, 668), (x1 + 40, 668)], PINK, 1.2, 0.5)  # 아래 버팀대 (다리 사이)
    # 위 가로대 + 손잡이 구멍 + 전구
    M.rr(x0 - 6, 22, BOXW + 12, 34, 10, PINK, 2.2, 0.9)
    M.rr(640 - 34, 30, 68, 16, 8, PINK, 1.0, 0.5)
    bulbs(M, x0 + 18, 640 - 52, 39, 9)
    bulbs(M, 640 + 52, x1 - 18, 39, 9)
    # 칠판: 두 겹 테 + 아주 어두운 초록 바탕
    fill(M, x0 + 12, top + 12, BOXW - 24, bot - top - 24, hx('3c7c64'), 0.24)  # 0.25 이하 = 빛번짐 없이 바탕만
    M.rr(x0, top, BOXW, bot - top, 6, PINK, 2.4, 0.95)
    M.rr(x0 + 10, top + 10, BOXW - 20, bot - top - 20, 3, PINK, 1.0, 0.45)
    # 분필 받침 (칠판 아래 선반)
    M.ln([(x0 + 30, bot + 10), (x1 - 30, bot + 10)], PINK, 1.6, 0.7)
    M.ln([(x0 + 70, bot + 6), (x0 + 96, bot + 6)], (230, 230, 230), 2.4, 0.6)  # 분필 한 토막
    title(M, 640, top + 46)
    closex(M, SETX + LABW + 352 - 13, top + 36)
    rows(M, Y0 - 14)
    M.word(640, 700, 'B 세워 둔 칠판 간판 — 위 가로대 전구 · 칠판 바탕 · 분필 받침 · 벌어진 다리', GREY, 12, font=FONT_KO)
    return M.draw()


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    board_A().save(os.path.join(outdir, 'settings_board_A.png'))
    board_B().save(os.path.join(outdir, 'settings_board_B.png'))
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
