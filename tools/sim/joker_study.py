#!/usr/bin/env python3
"""조커 · 모드 연구 결과 모으기 (이슈 #5). tools/sim/joker_study.sh 가 만든 폴더를 읽어 마크다운 표를 찍는다.
   python3 tools/sim/joker_study.py <결과 폴더>
"""
import math
import pathlib
import statistics as st
import sys
from collections import defaultdict

NAMES = ["광대", "모자 장수", "웃는 가면", "희비극", "할리퀸", "지휘봉", "잭", "체셔", "방울", "스타"]
MODES = {0: "30", 1: "60", 2: "100", 3: "무한"}
GOAL = {0: 30, 1: 60, 2: 100, 3: 200}
LABEL = {"normal": "", "pursue": " (레시피 노림 봇)", "starter": " (처음 상태: 조커 4종)", "oldbot": " (예전 봇)"}


def parse(path):
    runs = []
    for f in sorted(pathlib.Path(path).glob("*.txt")):
        if f.name == "jobs.txt":
            continue
        group = f.name.split("-")[0]
        for line in f.read_text().splitlines():
            if not line.startswith("STUDY "):
                continue
            d = {"group": group}
            for tok in line.split()[1:]:
                k, v = tok.split("=", 1)
                d[k] = v
            for k in ("mode", "upg", "content", "mask", "smart", "pursue", "seed", "stuck", "wave", "clear", "lives", "phases", "uppers", "jnow", "jmaxlv", "feeds"):
                d[k] = int(d[k])
            d["slack"] = float(d["slack"])
            for k in ("built", "first", "can", "firstcan"):
                d[k] = [int(x) for x in d[k].split(",")]
            d["log"] = [] if d["log"] == "-" else [tuple(int(y) for y in x.split(":")) for x in d["log"].split(",")]
            runs.append(d)
    return runs


def wilson(k, n, z=1.96):
    if n == 0:
        return (0, 0)
    p = k / n
    den = 1 + z * z / n
    c = (p + z * z / (2 * n)) / den
    h = z * math.sqrt(p * (1 - p) / n + z * z / (4 * n * n)) / den
    return (max(0, c - h), min(1, c + h))


def pct(x):
    return f"{100 * x:.0f}%"


def med(v):
    return st.median(v) if v else float("nan")


def q(v, p):
    if not v:
        return float("nan")
    s = sorted(v)
    i = (len(s) - 1) * p
    lo, hi = math.floor(i), math.ceil(i)
    return s[lo] + (s[hi] - s[lo]) * (i - lo)


def main(path):
    runs = parse(path)
    stuck = [r for r in runs if r["stuck"]]
    print(f"판 {len(runs)}개 읽음, 멈춘 판 {len(stuck)}개\n")

    # 1. 일반 난이도
    print("## 모드별 난이도 (똑똑한 조커 봇, 모든 조커 열림)\n")
    print("| 모드 | 영구 강화 | 판 | 클리어 | 95% 범위 | 무너진 웨이브 (클리어 못 한 판) 중앙 · 25~75% | 남은 목숨 (클리어 판) | 판당 조커 | 판당 상위 조커 |")
    print("|---|---|---|---|---|---|---|---|---|")
    for grp in ("normal", "pursue", "starter", "oldbot"):
        cells = defaultdict(list)
        for r in runs:
            if r["group"] == grp:
                cells[(r["mode"], r["upg"])].append(r)
        for (m, u), rs in sorted(cells.items()):
            n = len(rs)
            k = sum(r["clear"] for r in rs)
            lo, hi = wilson(k, n)
            fail = [r["wave"] for r in rs if not r["clear"]]
            lives = [r["lives"] for r in rs if r["clear"]]
            jk = st.mean(sum(r["built"]) for r in rs)
            up = st.mean(r["uppers"] for r in rs)
            label = LABEL[grp]
            fw = f"{med(fail):.0f} · {q(fail, .25):.0f}~{q(fail, .75):.0f}" if fail else "—"
            lv = f"{st.mean(lives):.1f}" if lives else "—"
            print(f"| {MODES[m]}{label} | {u} | {n} | {k}/{n} ({pct(k / n)}) | {pct(lo)}~{pct(hi)} | {fw} | {lv} | {jk:.1f} | {up:.2f} |")
    print()

    # 1-2. 조커 등장 요약 (이슈 #25): 첫 조커 웨이브, 판당 수 · 종류, 조커 없는 판
    print("## 조커 등장 요약 (강화 0)\n")
    print("| 모드 | 판 | 판당 조커 | 판당 종류 | 첫 조커 웨이브 중앙 · 25~75% | 조커 0개 판 | 10웨이브까지 | 모자 장수 만든 판 | 스타 만든 판 | 광대 만든 판 |")
    print("|---|---|---|---|---|---|---|---|---|---|")
    for grp in ("normal", "pursue", "starter"):
        for m in (0, 1, 2, 3):
            rs = [r for r in runs if r["group"] == grp and r["mode"] == m and r["upg"] == 0]
            if not rs:
                continue
            n = len(rs)
            firsts = [min(w for (w, _) in r["log"]) for r in rs if r["log"]]
            kinds = st.mean(sum(1 for b in r["built"] if b) for r in rs)
            by10 = st.mean(sum(1 for (w, _) in r["log"] if w <= 10) for r in rs)
            zero = sum(1 for r in rs if not r["log"])
            fw = f"{med(firsts):.0f} · {q(firsts, .25):.0f}~{q(firsts, .75):.0f}" if firsts else "—"
            hb = lambda j: f"{sum(1 for r in rs if r['built'][j])}/{n}"
            print(f"| {MODES[m]}{LABEL[grp]} | {n} | {st.mean(sum(r['built']) for r in rs):.1f} | {kinds:.1f} | {fw} | {zero} | {by10:.1f} | {hb(1)} | {hb(9)} | {hb(0)} |")
    print()

    # 2. 조커 만들기 난이도
    for grp, m, u in [(g, m, 0) for g in ("normal", "pursue") for m in (0, 1, 2, 3)]:
        rs = [r for r in runs if r["group"] == grp and r["mode"] == m and r["upg"] == u]
        if not rs:
            continue
        n = len(rs)
        print(f"## 조커 만들기 · {MODES[m]} 모드 강화 0{LABEL[grp]} ({n}판)\n")
        print("| 조커 | 레시피가 판에 생긴 판 | 처음 생긴 웨이브 (중앙) | 생겨 있던 웨이브 비율 | 실제로 만든 판 | 판당 만든 수 | 처음 만든 웨이브 (중앙) | 만든 웨이브 중앙 · 25~75% |")
        print("|---|---|---|---|---|---|---|---|")
        for j in range(10):
            can_runs = sum(1 for r in rs if r["firstcan"][j])
            fc = [r["firstcan"][j] for r in rs if r["firstcan"][j]]
            can_share = sum(r["can"][j] for r in rs) / max(1, sum(r["phases"] for r in rs))
            b_runs = sum(1 for r in rs if r["built"][j])
            b_mean = st.mean(r["built"][j] for r in rs)
            fb = [r["first"][j] for r in rs if r["first"][j]]
            bw = [w for r in rs for (w, jj) in r["log"] if jj == j]
            bws = f"{med(bw):.0f} · {q(bw, .25):.0f}~{q(bw, .75):.0f}" if bw else "—"
            fcs = f"{med(fc):.0f}" if fc else "—"
            print(f"| {NAMES[j]} | {pct(can_runs / n)} | {fcs} | {pct(can_share)} | {pct(b_runs / n)} | {b_mean:.2f} | {med(fb) if fb else '—'} | {bws} |")
        print()

    # 3. 웨이브까지 만든 조커 수 (100 모드 판의 기록을 웨이브로 자름)
    print("## 웨이브까지 만든 조커 수 (똑똑한 조커 봇, 강화 0, 그 웨이브까지 살아 있던 판만)\n")
    print("| 모드 | 웨이브 5 | 10 | 20 | 30 | 45 | 60 | 80 | 100 |")
    print("|---|---|---|---|---|---|---|---|---|")
    for m in (0, 1, 2, 3):
        rs = [r for r in runs if r["group"] == "normal" and r["mode"] == m and r["upg"] == 0]
        row = []
        if not rs:
            continue
        for w in (5, 10, 20, 30, 45, 60, 80, 100):
            alive = [r for r in rs if r["wave"] >= w]
            if not alive or w > GOAL[m]:
                row.append("—")
                continue
            row.append(f"{st.mean(sum(1 for (bw, _) in r['log'] if bw <= w) for r in alive):.1f} ({len(alive)}판)")
        print(f"| {MODES[m]} | " + " | ".join(row) + " |")
    print()

    # 4. 조커 하나만 열기
    solo = [r for r in runs if r["group"] == "solo"]
    if solo:
        by = defaultdict(dict)
        for r in solo:
            by[r["mask"]][r["seed"]] = r
        base = by.get(0, {})
        print(f"## 조커 하나만 연 판 (60 모드, 강화 0, 판 위력이 10% 깎여도 만듦)\n")
        print("| 열린 조커 | 판 | 클리어 | 평균 웨이브 | 조커 없음 대비 (같은 씨앗 짝 비교) | 판당 만든 수 | 만든 판 비율 |")
        print("|---|---|---|---|---|---|---|")
        rows = []
        for mask, d in by.items():
            rs = list(d.values())
            n = len(rs)
            k = sum(r["clear"] for r in rs)
            mw = st.mean(r["wave"] for r in rs)
            diffs = [d[s]["wave"] - base[s]["wave"] for s in d if s in base]
            dm = st.mean(diffs) if diffs else 0
            se = (st.stdev(diffs) / math.sqrt(len(diffs))) if len(diffs) > 1 else 0
            nb = st.mean(sum(r["built"]) for r in rs)
            made = sum(1 for r in rs if sum(r["built"]) > 0) / n
            name = "없음" if mask == 0 else "전부 (상위 조커 없이)" if mask == 1023 else NAMES[int(math.log2(mask))]
            rows.append((dm if mask else -999, f"| {name} | {n} | {k}/{n} | {mw:.1f} | {dm:+.1f} ± {1.96 * se:.1f} | {nb:.1f} | {pct(made)} |"))
        for _, line in sorted(rows, key=lambda x: -x[0]):
            print(line)
        print()

    if stuck:
        print("## 멈춘 판\n")
        for r in stuck:
            print(f"- {r['group']} 모드 {MODES[r['mode']]} 씨앗 {r['seed']} 웨이브 {r['wave']}")


if __name__ == "__main__":
    main(sys.argv[1])
