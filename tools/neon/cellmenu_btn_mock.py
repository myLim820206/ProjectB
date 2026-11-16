"""칸 메뉴(타워 클릭 툴팁) 아래 할 일 줄을 버튼처럼 (시안). 지금은 글자 줄이라 툴팁 설명으로 보여 누르는 줄 모른다

    python tools/neon/cellmenu_btn_mock.py 출력폴더   → cellmenu_btn_mock.png

A 세로 튜브 버튼: 할 일마다 상자 폭 알약 튜브 (메뉴 BREAK TIME 버튼처럼 두 겹), 오른쪽에 정보 글자
B 가로 나란히: 합성(넓게) | 판매(좁게) 두 버튼을 한 줄에. 판매는 빨강 테
C 큰 주 버튼 + 작은 보조: 합성은 채운 큰 버튼 + 두 카드가 합쳐지는 기호 + 숨쉬는 빛, 판매는 오른쪽 아래 작은 글자 버튼
"""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, GOLD, WHITE, FONT_KO, hx  # noqa: E402

FG, DIM, GREY = hx('dce4f0'), hx('5f6878'), hx('8a94a6')
GREEN, RED, LAV = hx('9cffb0'), hx('ff6a6a'), hx('a896e1')
K = 1.3  # 게임 크기(상자 290) × 1.3 으로 그림
TW = 290 * K


def fill(M, x, y, w, h, c, b=0.16):
    """옅은 채움 (굵은 선 하나를 낮은 밝기로)"""
    M.ln([(x, y + h / 2), (x + w, y + h / 2)], c, h, b)


def pill(M, x, y, w, h, c, b=1.0, wd=1.6, double=True):
    M.rr(x, y, w, h, h / 2, c, wd, b)
    if double: M.rr(x + 3, y + 3, w - 6, h - 6, (h - 6) / 2, c, 0.8, b * 0.5)


def top(M, x, y, title):
    """위쪽 툴팁 내용 (지금 그대로)"""
    M.word(x + 14, y + 26, title, GOLD, 15, font=FONT_KO, anchor_center=False)
    M.word(x + 14, y + 48, '위력 2.7k/초 (판의 0%)', GREEN, 13, font=FONT_KO, anchor_center=False)
    M.word(x + 14, y + 70, '공격력 456 · 초당 2.6발 · 치명 5% ×1.50', FG, 12, font=FONT_KO, anchor_center=False)
    M.word(x + 14, y + 92, '사거리 2.8 · 범위 1.5', GREY, 12, font=FONT_KO, anchor_center=False)
    M.word(x + 14, y + 114, '플러시4 연쇄×7.5 · 투 페어 연쇄×7.5', GOLD, 12, font=FONT_KO, anchor_center=False)


def box(M, x, y, h, cap):
    M.rr(x, y, TW, h, 10, GOLD, 1.4, 0.55)
    top(M, x, y, '3 연사 · 합성 레벨 1')
    M.ln([(x + 12, y + 140), (x + TW - 12, y + 140)], DIM, 1.0, 0.8)
    M.word(x + TW / 2, y - 18, cap, WHITE, 18, font=FONT_KO)


def main(outdir):
    os.makedirs(outdir, exist_ok=True)
    M = Sign(0.8)
    M.small_glow = 0.0
    Y = 150
    # ---- A 세로 튜브 버튼
    x = 24
    box(M, x, Y, 262, 'A 세로 튜브 버튼')
    by = Y + 154
    pill(M, x + 12, by, TW - 24, 40, GOLD, 1.0, 1.8)
    fill(M, x + 30, by + 4, TW - 60, 32, GOLD, 0.16)
    M.word(x + 32, by + 26, '합성 · 플러시4 연쇄×7.5', WHITE, 14, font=FONT_KO, anchor_center=False)
    M.word(x + TW - 70, by + 26, '4장', FG, 12, font=FONT_KO, anchor_center=False)
    by += 50
    pill(M, x + 12, by, TW - 24, 40, GREY, 0.7, 1.4)
    M.word(x + 32, by + 26, '판매', FG, 14, font=FONT_KO, anchor_center=False)
    M.word(x + TW - 110, by + 26, '+5 경험치', GREY, 12, font=FONT_KO, anchor_center=False)
    M.word(x + TW / 2, Y + 300, '↑ 마우스를 올린 버튼 (합성) 은 채움 + 흰 글자', DIM, 11, font=FONT_KO)
    # ---- B 가로 나란히
    x = 24 + TW + 30
    box(M, x, Y, 220, 'B 가로 나란히')
    by = Y + 156
    bw = (TW - 36) * 0.68
    pill(M, x + 12, by, bw, 46, GOLD, 1.0, 1.8)
    fill(M, x + 32, by + 5, bw - 40, 36, GOLD, 0.16)
    M.word(x + 12 + bw / 2, by + 21, '합성', WHITE, 15, font=FONT_KO)
    M.word(x + 12 + bw / 2, by + 38, '플러시4 연쇄×7.5 · 4장', GOLD, 11, font=FONT_KO)
    sx, sw = x + 24 + bw, TW - 36 - bw
    pill(M, sx, by, sw, 46, RED, 0.75, 1.4)
    M.word(sx + sw / 2, by + 21, '판매', FG, 15, font=FONT_KO)
    M.word(sx + sw / 2, by + 38, '+5 경험치', GREY, 11, font=FONT_KO)
    M.word(x + TW / 2, Y + 246, '할 일이 셋 이상(조커 · 먹이기)이면 두 줄로', DIM, 11, font=FONT_KO)
    # ---- C 큰 주 버튼 + 작은 보조
    x = 24 + 2 * (TW + 30)
    box(M, x, Y, 244, 'C 큰 주 버튼 + 작은 보조')
    by = Y + 154
    M.rr(x + 12, by, TW - 24, 50, 12, GOLD, 2.2, 1.0)
    fill(M, x + 16, by + 4, TW - 32, 42, GOLD, 0.24)
    # 두 카드가 합쳐지는 기호
    cx, cy = x + 44, by + 25
    M.rr(cx - 14, cy - 13, 14, 20, 3, GOLD, 1.3, 1.0)
    M.rr(cx - 4, cy - 7, 14, 20, 3, GOLD, 1.3, 1.0)
    M.ln([(cx + 14, cy + 2), (cx + 20, cy + 2)], WHITE, 1.2, 0.9)
    M.ln([(cx + 17, cy - 1), (cx + 20, cy + 2), (cx + 17, cy + 5)], WHITE, 1.2, 0.9)
    M.word(x + 74, by + 24, '합성', WHITE, 16, font=FONT_KO, anchor_center=False)
    M.word(x + 74, by + 41, '플러시4 연쇄×7.5', GOLD, 11, font=FONT_KO, anchor_center=False)
    M.word(x + TW - 30, by + 31, '4장 ›', FG, 12, font=FONT_KO)
    sy = by + 62
    M.rr(x + TW - 132, sy, 120, 24, 12, GREY, 1.1, 0.6)
    M.word(x + TW - 72, sy + 17, '판매 · +5 경험치', GREY, 11, font=FONT_KO)
    M.word(x + TW / 2, Y + 270, '합성이 없으면 다음 할 일(조커 · 먹이기)이 큰 버튼', DIM, 11, font=FONT_KO)
    M.word(640, 640, '지금 = 툴팁 아래 글자 줄 (마우스를 올려야 테가 생김) → 처음부터 버튼 테 · 채움으로 누를 수 있게', DIM, 12, font=FONT_KO)
    M.draw().save(os.path.join(outdir, 'cellmenu_btn_mock.png'))
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
