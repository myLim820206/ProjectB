"""상위 조커 30종 기호 → Source/ProjectB/PDUpperArt.inl (D-231)

    python tools/neon/gen_upper_art.py

tools/neon/upper_jokers.py 의 그림 함수(m1~m30)를 중심 (0, 0), 크기 R = 1 로 계산해 점 배열로 굽는다.
게임은 이 점을 틀 반 너비 R 로 늘려 네온 선으로 그린다 (그림을 고치면 이 스크립트를 다시 실행).
글자 기호('J', 'z')는 따로 적는다.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
import upper_jokers as U  # noqa: E402

ROOT = os.path.join(os.path.dirname(__file__), '..', '..')
OUT = os.path.join(ROOT, 'Source', 'ProjectB', 'PDUpperArt.inl')


def main():
    pts, lines, texts, index = [], [], [], []
    for k, (fn, name, parts) in enumerate(U.UPPER):
        l0, t0 = len(lines), len(texts)
        for ln in fn(0.0, 0.0, 1.0):
            if ln and ln[0] == 'text':
                texts.append((ln[1], ln[2], ln[3], ln[4]))
                continue
            p0 = len(pts)
            pts.extend(ln)
            lines.append((p0, len(ln)))
        index.append((l0, len(lines) - l0, t0, len(texts) - t0))
    with open(OUT, 'w', encoding='utf-8-sig') as f:
        f.write('// 생성됨: tools/neon/gen_upper_art.py (tools/neon/upper_jokers.py 그림). 직접 고치지 말 것\n')
        f.write('// 점은 중심 (0, 0), 틀 반 너비 1 기준\n')
        f.write('static const float GPDUpperPts[][2] = {\n')
        for i in range(0, len(pts), 6):
            f.write('\t' + ' '.join('{%.3ff, %.3ff},' % (x, y) for x, y in pts[i:i + 6]) + '\n')
        f.write('};\nstatic const int GPDUpperLines[][2] = { // 첫 점, 점 수\n')
        for i in range(0, len(lines), 8):
            f.write('\t' + ' '.join('{%d, %d},' % l for l in lines[i:i + 8]) + '\n')
        f.write('};\nstruct FPDUpperText { float X, Y; const TCHAR* S; float Px; };\n')
        f.write('static const FPDUpperText GPDUpperTexts[] = {\n')
        for x, y, s, px in texts:
            f.write('\t{%.3ff, %.3ff, TEXT("%s"), %.3ff},\n' % (x, y, s, px))
        f.write('};\nstatic const int GPDUpperIndex[30][4] = { // 첫 선, 선 수, 첫 글자, 글자 수\n')
        for i in range(0, len(index), 5):
            f.write('\t' + ' '.join('{%d, %d, %d, %d},' % e for e in index[i:i + 5]) + '\n')
        f.write('};\n')
    print('saved', OUT, 'points', len(pts), 'lines', len(lines), 'texts', len(texts))


if __name__ == '__main__':
    main()
