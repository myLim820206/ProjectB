"""번역 조각(ko, en, ja, zh-Hans, zh-Hant 탭 구분)을 strings.tsv 에 합친다 (언어팩, D-159)

    python tools/lang/merge.py 조각.tsv
"""
import os
import sys

HERE = os.path.dirname(__file__)
TSV = os.path.join(HERE, 'strings.tsv')


def main():
    tr = {}
    for ln in open(sys.argv[1], encoding='utf-8'):
        c = ln.rstrip('\n').split('\t')
        if len(c) >= 5:
            tr[c[0]] = c[1:5]
    rows = [l.rstrip('\n').split('\t') for l in open(TSV, encoding='utf-8') if l.strip()]
    used = set()
    out = [rows[0]]
    for c in rows[1:]:
        c = (c + [''] * 6)[:6]
        if c[0] in tr:
            c = [c[0]] + tr[c[0]] + [c[5]]
            used.add(c[0])
        out.append(c)
    open(TSV, 'w', encoding='utf-8', newline='\n').write('\n'.join('\t'.join(c) for c in out) + '\n')
    print('merged', len(used), 'not found', [k for k in tr if k not in used])
    print('still empty', [c[0] for c in out[1:] if not c[1]])


if __name__ == '__main__':
    main()
