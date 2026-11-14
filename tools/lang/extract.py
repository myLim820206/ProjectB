"""게임 코드의 한국어 글자를 모은다 (언어팩, D-159).

    python tools/lang/extract.py            # 개수만
    python tools/lang/extract.py --tsv OUT  # 번역 표 틀 (ko 열만 채움, 이미 있는 번역은 유지)

키는 한국어 원문 그대로 (gettext 방식). 게임은 그릴 때 이 원문으로 번역을 찾는다.
"""
import collections
import glob
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(__file__), '..', '..', 'Source', 'ProjectB')
HANG = re.compile('[가-힣]')
LIT = re.compile(r'TEXT\("((?:[^"\\]|\\.)*)"\)')


def strings():
    per = collections.Counter()
    seen = {}
    for f in sorted(glob.glob(os.path.join(ROOT, '*.cpp')) + glob.glob(os.path.join(ROOT, '*.h')) + glob.glob(os.path.join(ROOT, '*.inl'))):
        for line in open(f, encoding='utf-8-sig'):
            cut = line.find('//')
            for m in LIT.finditer(line):
                if cut != -1 and cut < m.start():
                    continue
                v = m.group(1)
                if HANG.search(v):
                    per[os.path.basename(f)] += 1
                    seen.setdefault(v, os.path.basename(f))
    return seen, per


def main():
    seen, per = strings()
    print(per.most_common())
    print('unique', len(seen), 'chars', sum(len(k) for k in seen))
    if len(sys.argv) > 2 and sys.argv[1] == '--tsv':
        out = sys.argv[2]
        old = {}
        if os.path.exists(out):
            for ln in open(out, encoding='utf-8'):
                cols = ln.rstrip('\n').split('\t')
                if cols and cols[0] != 'ko':
                    old[cols[0]] = cols
        with open(out, 'w', encoding='utf-8', newline='\n') as fo:
            fo.write('ko\ten\tja\tzh-Hans\tzh-Hant\tfile\n')
            for k, f in seen.items():
                c = old.get(k, [k, '', '', '', ''])
                c = (c + [''] * 5)[:5]
                fo.write('\t'.join(c) + '\t' + f + '\n')


if __name__ == '__main__':
    main()
