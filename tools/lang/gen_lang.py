"""번역 표(strings.tsv) → Source/ProjectB/PDLangTable.inl (언어팩, D-159)

    python tools/lang/extract.py --tsv tools/lang/strings.tsv   # 새 한국어 글자를 표에 더한다 (번역은 유지)
    python tools/lang/gen_lang.py                                # 게임에 넣을 표를 만든다

열: ko, en, ja, zh-Hans, zh-Hant (+ file 참고용). printf 형식(%s %d %.1f %%)은 원문과 같은 순서로 둔다.
"""
import os
import re
import sys

HERE = os.path.dirname(__file__)
TSV = os.path.join(HERE, 'strings.tsv')
OUT = os.path.join(HERE, '..', '..', 'Source', 'ProjectB', 'PDLangTable.inl')
SPEC = re.compile(r'%[-+ #0]*\d*(?:\.\d+)?[sdifuxXc%]')


def esc(s, i):
    # ko 열은 코드의 TEXT("...") 안쪽 그대로 (이미 이스케이프됨). 번역 열은 역슬래시·따옴표를 이스케이프
    return s if i == 0 else s.replace('\\', '\\\\').replace('"', '\\"')


def main():
    rows = []
    bad = 0
    for ln in open(TSV, encoding='utf-8'):
        cols = ln.rstrip('\n').split('\t')
        if not cols or cols[0] == 'ko' or not cols[0]:
            continue
        cols = (cols + [''] * 6)[:5]
        want = SPEC.findall(cols[0])
        for i in range(1, 5):
            if cols[i] and SPEC.findall(cols[i]) != want:
                print('형식 다름:', cols[0], '|', cols[i]); bad += 1; cols[i] = ''
        rows.append(cols)
    with open(OUT, 'w', encoding='utf-8', newline='\r\n') as f:
        f.write('// 자동 생성: tools/lang/gen_lang.py (tools/lang/strings.tsv). 직접 고치지 않는다\n')
        f.write('static const TCHAR* const PDLangRows[][5] = {\n')
        for c in rows:
            f.write('\t{' + ', '.join('TEXT("%s")' % esc(x, i) for i, x in enumerate(c)) + '},\n')
        f.write('};\n')
        f.write('static const int32 PDLangRowCount = %d;\n' % len(rows))
    n = [sum(1 for c in rows if c[i]) for i in range(1, 5)]
    print('rows', len(rows), 'translated en/ja/zh-Hans/zh-Hant', n, 'format mismatches', bad)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
