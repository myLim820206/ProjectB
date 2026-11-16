"""칸 메뉴 상자 시안 (D-312, D-313) — 타워 클릭 = 툴팁이 그 자리에 고정되며 메뉴가 된다

    python3 tools/neon/cell_menu_mock.py 출력폴더   → cell_menu_A.png (막 연 상태) · cell_menu_B.png (합성 줄에 마우스) · cell_menu_C.png (조커)

위 = 툴팁 내용(줄여서) 또는 줄에 마우스를 올렸을 때 전후 비교, 아래 = 할 일 줄 (합성 · 조커 · 먹이기 · 합치기 · 판매).
상자는 클릭한 칸이 든 족보 칸들을 피해서 처음 뜬 자리에 고정. 줄에 마우스 = 판에서 그 족보만 밝게 + 재료 → 클릭한 칸 점선.
"""
import os
import sys

from PIL import ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(__file__))
from logo_mock import Sign, PINK, CYAN, GOLD, S  # noqa: E402

KO = '/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc'
FG, DIM, GRN, RED, SKY, LAV = (220, 228, 240), (95, 104, 120), (156, 255, 176), (255, 106, 106), (95, 242, 255), (168, 150, 225)
CS, FX, FY = 44, 240, 120
SUITC = {'s': (127, 184, 255), 'h': (255, 106, 138), 'd': (255, 217, 106), 'c': (138, 255, 196)}


def board(M, towers, lit=(), dim_others=False, target=None):
    for y in range(10):
        for x in range(16):
            M.rr(FX + x * CS + 3, FY + y * CS + 3, CS - 6, CS - 6, 6, (40, 44, 60), 0.6, 0.35)
    for (x, y), (rk, su) in towers.items():
        b = 0.25 if dim_others and (x, y) not in lit else 0.9
        M.rr(FX + x * CS + 6, FY + y * CS + 6, CS - 12, CS - 12, 6, SUITC[su], 1.4, b)
    for (x, y) in lit:
        M.rr(FX + x * CS + 2, FY + y * CS + 2, CS - 4, CS - 4, 7, GOLD, 2.2, 1.0)
    if target:
        tx, ty = target
        M.rr(FX + tx * CS, FY + ty * CS, CS, CS, 8, SKY, 2.8, 1.0)
        for (x, y) in lit:
            if (x, y) != target:
                M.ln([(FX + x * CS + CS / 2, FY + y * CS + CS / 2), (FX + tx * CS + CS / 2, FY + ty * CS + CS / 2)], SKY, 1.0, 0.7)


def text(img, items):
    d = ImageDraw.Draw(img)
    for (x, y, s, c, px, anchor) in items:
        d.text((x, y), s, fill=c, font=ImageFont.truetype(KO, px), anchor=anchor)


def menu(M, T, x, y, top, rows, hov=-1, edge=GOLD):
    w, top_h, rh = 280, 18 * len(top) + 20, 26
    h = top_h + rh * len(rows) + 10
    M.rr(x, y, w, h, 8, edge, 1.4, 0.8)
    M.ln([(x + 10, y + top_h), (x + w - 10, y + top_h)], DIM, 0.8, 0.6)
    for i, (s, c, px) in enumerate(top):
        T.append((x + 12, y + 14 + i * 18, s, c, px, 'lm'))
    for i, (s, c, right) in enumerate(rows):
        ry = y + top_h + 6 + i * rh
        if i == hov:
            M.rr(x + 6, ry, w - 12, rh - 4, 8, c, 1.6, 1.0)
        T.append((x + 18, ry + rh / 2 - 2, s, (255, 255, 255) if i == hov else c, 13, 'lm'))
        T.append((x + w - 16, ry + rh / 2 - 2, right, DIM if i != hov else FG, 11, 'rm'))
    return w, h


TOW = {(3, 4): ('5', 'h'), (4, 4): ('6', 's'), (5, 4): ('7', 'd'), (6, 4): ('8', 'c'), (7, 4): ('9', 'h'), (5, 5): ('7', 's'), (8, 6): ('K', 'd'), (9, 6): ('K', 'c'), (2, 2): ('A', 's')}
STRAIGHT = [(3, 4), (4, 4), (5, 4), (6, 4), (7, 4)]


def main(out):
    os.makedirs(out, exist_ok=True)
    rows = [('합성 · 스트레이트9 (5장)', GOLD, '5장'), ('합성 · 원 페어 (2장)', GOLD, '2장'), ('조커: 지휘봉', PINK, '재료 1/3'), ('판매', FG, '+12 경험치')]
    base = [('7 ♦ · 합성 레벨 1', SUITC['d'], 14), ('위력 412/초 (판의 6%)', GRN, 12), ('공격력 18 · 초당 2.4발 · 치명 5% ×1.50', FG, 11), ('사거리 3.0 · 범위 0.6', DIM, 11), ('스트레이트9 · 원 페어 · 연쇄 ×1.25', GOLD, 11)]
    # A: 막 연 상태
    M, T = Sign(), []
    board(M, TOW)
    M.rr(FX + 5 * CS + 1, FY + 4 * CS + 1, CS - 2, CS - 2, 8, PINK, 2.4, 1.0)
    menu(M, T, FX + 5 * CS + 20, FY + 5 * CS + 60, base, rows)
    T.append((640, 40, '시안 A · 7♦ 클릭: 툴팁이 그 자리에 고정되며 메뉴 (상자는 족보 칸을 피해 아래에)', FG, 15, 'mm'))
    img = M.draw(); text(img, T); img.save(os.path.join(out, 'cell_menu_A.png'))
    # B: 합성 줄에 마우스 = 위가 전후 비교, 판에 그 족보만 + 점선
    M, T = Sign(), []
    board(M, TOW, lit=STRAIGHT, dim_others=True, target=(5, 4))
    cmp_ = [('합성하면: 7 ♦ · 스트레이트 Lv2', GOLD, 14), ('위력 1,830/초 → 2,960/초 (+62%)', GRN, 12), ('재료 5장의 위력 합 → 합성 타워 하나', DIM, 10), ('공격력 18 → 41 · 초당 2.4 → 3.1발', FG, 11), ('사거리 3.0 → 3.4 · 범위 0.6 → 0.8', DIM, 11)]
    menu(M, T, FX + 5 * CS + 20, FY + 5 * CS + 60, cmp_, rows, hov=0)
    T.append((640, 40, '시안 B · 합성 줄에 마우스: 위 = 전후 비교, 판 = 그 족보만 밝게 + 재료 → 클릭한 칸 점선', FG, 15, 'mm'))
    img = M.draw(); text(img, T); img.save(os.path.join(out, 'cell_menu_B.png'))
    # C: 조커 클릭
    M, T = Sign(), []
    board(M, TOW)
    M.rr(FX + 9 * CS + 1, FY + 6 * CS + 1, CS - 2, CS - 2, 8, PINK, 2.4, 1.0)
    jtop = [('지휘봉 Lv7', PINK, 14), ('위력 2,104/초 (판의 21%)', GRN, 12), ('조커 레벨 7 · 경험치 120/260', PINK, 11), ('맞닿은 같은 무늬 타워 연사 +20%', FG, 11)]
    jrows = [('먹이기 · 원 페어 (2장)', PINK, '경험치 +60'), ('먹이기 · 트리플 (3장)', PINK, '경험치 +110'), ('상위 조커: 하얀 기사', GOLD, '희비극 Lv10'), ('판매', FG, '+40 경험치')]
    menu(M, T, FX + 9 * CS - 300, FY + 6 * CS - 200, jtop, jrows, edge=PINK)
    T.append((640, 40, '시안 C · 조커 클릭: 먹이기 · 합치기/상위 조커 · 판매 (같은 상자, D-313)', FG, 15, 'mm'))
    img = M.draw(); text(img, T); img.save(os.path.join(out, 'cell_menu_C.png'))
    print('ok')


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else '.')
