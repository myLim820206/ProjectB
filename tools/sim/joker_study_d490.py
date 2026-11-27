#!/usr/bin/env python3
"""D-489 · D-490 QA 표 (이슈 #33). joker_study 결과 폴더(또는 STUDY 줄 파일)를 읽어 마크다운 표를 찍는다.
   python3 tools/sim/joker_study_d490.py <결과 폴더> [비교할 예전 STUDY 파일]
묶음 이름(파일 이름 앞 또는 group=): normal · starter · aim(스트레이트 플러시 노림 + 행운) · aimoff(노림, 행운 끔) · aimpNN(행운 확률 4장 줄 0.NN) · solo
"""
import math
import pathlib
import statistics as st
import sys
from collections import Counter, defaultdict

NAMES = ["광대", "모자 장수", "웃는 가면", "희비극", "할리퀸", "지휘봉", "잭", "체셔", "방울", "스타"]
HANDS = ["로열", "파이브 카드", "스트레이트 플러시", "포카드", "풀하우스", "플러시", "스트레이트", "트리플", "투 페어", "원 페어"]
UPPERS = ["장미 칠하는 정원사", "트위들덤과 트위들디", "도도새", "애벌레", "공작부인", "거울 나라", "3월 토끼", "하트 왕", "하트 잭", "미친 다과회",
          "흰 토끼", "험프티 덤프티", "붉은 여왕", "가짜 거북", "그리핀", "웃음만 남은 고양이", "하얀 여왕", "바다코끼리와 목수", "하얀 기사", "말하는 꽃들",
          "하트 여왕", "홍학 크로케", "트럼프 근위대", "체스판", "겨울잠쥐", "유니콘", "재버워키", "사자", "꿈꾸는 붉은 왕", "반짝반짝 작은 박쥐"]
MODES = {0: "30", 1: "60", 2: "100", 3: "무한"}
LABEL = {"normal": "", "starter": " 처음 상태", "aim": " 노림 · 행운 40/15%", "aimoff": " 노림 · 행운 끔",
         "aimp20": " 노림 · 행운 20/7.5%", "aimp10": " 노림 · 행운 10/3.75%", "aimp05": " 노림 · 행운 5/2%"}
INTS = ("mode", "upg", "content", "mask", "smart", "pursue", "seed", "stuck", "wave", "clear", "lives", "phases", "uppers")


def parse_line(line, group=None):
    d = {"group": group}
    for tok in line.split()[1:]:
        k, v = tok.split("=", 1)
        d[k] = v
    for k in INTS:
        d[k] = int(d[k])
    for k in ("aimsf", "lucky", "luckyhits", "line3", "line4", "firstline4", "keys"):
        d[k] = int(d.get(k, 0))
    for k in ("built", "first", "can", "firstcan", "hands", "firsthand"):
        d[k] = [int(x) for x in d[k].split(",")] if k in d else [0] * 10
    lg = lambda s: [] if s in (None, "-") else [tuple(int(y) for y in x.split(":")) for x in s.split(",")]
    d["log"] = lg(d.get("log"))
    d["ulog"] = lg(d.get("upperlog"))
    d["llog"] = [] if d.get("luckylog", "-") == "-" else [int(x) for x in d["luckylog"].split(",")]
    return d


def load(path):
    p = pathlib.Path(path)
    files = [p] if p.is_file() else [f for f in sorted(p.glob("*.txt")) if f.name != "jobs.txt"]
    runs = []
    for f in files:
        for line in f.read_text().splitlines():
            if line.startswith("STUDY "):
                runs.append(parse_line(line, f.name.split("-")[0]))
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


def q(v, p):
    s = sorted(v)
    i = (len(s) - 1) * p
    lo, hi = math.floor(i), math.ceil(i)
    return s[lo] + (s[hi] - s[lo]) * (i - lo)


def iqr(v):
    return f"{st.median(v):.0f} · {q(v, .25):.0f}~{q(v, .75):.0f}" if v else "—"


def cells(runs, groups):
    out = defaultdict(list)
    for r in runs:
        if r["group"] in groups and r["upg"] == 0 and r["mask"] == -1:
            out[(r["group"], r["mode"])].append(r)
    return sorted(out.items(), key=lambda kv: (list(groups).index(kv[0][0]), kv[0][1]))


def main(path, old=None):
    runs = load(path)
    print(f"판 {len(runs)}개, 멈춘 판 {sum(r['stuck'] for r in runs)}개\n")
    G_ALL = ("normal", "starter", "aim", "aimoff", "aimp20", "aimp10", "aimp05")

    print("## 모드별 난이도 · 조커 · 상위 조커 (강화 0)\n")
    print("| 묶음 | 판 | 클리어 | 95% 범위 | 무너진 웨이브 중앙 · 25~75% | 판당 조커 | 판당 종류 | 첫 조커 웨이브 | 10 · 30웨이브까지 | 상위 조커 만든 판 | 판당 상위 조커 |")
    print("|---|---|---|---|---|---|---|---|---|---|---|")
    for (g, m), rs in cells(runs, G_ALL):
        n = len(rs)
        k = sum(r["clear"] for r in rs)
        lo, hi = wilson(k, n)
        fail = [r["wave"] for r in rs if not r["clear"]]
        firsts = [min(w for w, _ in r["log"]) for r in rs if r["log"]]
        by = lambda W: st.mean(sum(1 for w, _ in r["log"] if w <= W) for r in rs)
        clr = f"{k}/{n} ({pct(k / n)})" if m != 3 else "—"
        rng = f"{pct(lo)}~{pct(hi)}" if m != 3 else "—"
        print(f"| {MODES[m]}{LABEL.get(g, ' ' + g)} | {n} | {clr} | {rng} | {iqr(fail)} | {st.mean(sum(r['built']) for r in rs):.1f} | {st.mean(sum(1 for b in r['built'] if b) for r in rs):.1f} | {iqr(firsts)} | {by(10):.1f} · {by(30):.1f} | {sum(1 for r in rs if r['uppers'])}/{n} | {st.mean(r['uppers'] for r in rs):.2f} |")
    print()

    print("## 조커별 만든 판 · 레시피 생긴 판 (똑똑한 봇, 노림 없음)\n")
    print("| 조커 | " + " | ".join(f"{MODES[m]} 모드" for m in range(4)) + " |")
    print("|---|---|---|---|---|")
    norm = {m: [r for r in runs if r["group"] == "normal" and r["mode"] == m and r["mask"] == -1] for m in range(4)}
    for j in range(10):
        row = []
        for m in range(4):
            rs = norm[m]
            if not rs:
                row.append("—")
                continue
            n = len(rs)
            row.append(f"{pct(sum(1 for r in rs if r['firstcan'][j]) / n)} → {pct(sum(1 for r in rs if r['built'][j]) / n)} · {st.mean(r['built'][j] for r in rs):.2f}")
        print(f"| {NAMES[j]} | " + " | ".join(row) + " |")
    print()

    print("## 족보 등장 (배치를 마친 판에 그 족보가 있던 판 · 웨이브 비율, 똑똑한 봇)\n")
    print("| 족보 | " + " | ".join(f"{MODES[m]} 모드" for m in range(4)) + " |")
    print("|---|---|---|---|---|")
    for h in range(10):
        row = []
        for m in range(4):
            rs = norm[m]
            if not rs:
                row.append("—")
                continue
            ph = sum(r["phases"] for r in rs)
            row.append(f"{pct(sum(1 for r in rs if r['hands'][h]) / len(rs))} · {100 * sum(r['hands'][h] for r in rs) / ph:.1f}%")
        print(f"| {HANDS[h]} | " + " | ".join(row) + " |")
    print()

    print("## 모자 장수 · 스타 · 행운의 한 장\n")
    print("| 묶음 | 판 | 모자 장수 만든 판 | 스타 만든 판 | 스트레이트 플러시가 판에 생긴 판 | 로열 생긴 판 | 4장 줄이 생긴 판 · 첫 웨이브 | 판당 행운 발동 · 발동 웨이브 | 클리어 |")
    print("|---|---|---|---|---|---|---|---|---|")
    for (g, m), rs in cells(runs, ("normal", "aim", "aimoff", "aimp20", "aimp10", "aimp05")):
        n = len(rs)
        hb = sum(1 for r in rs if r["built"][1])
        sb = sum(1 for r in rs if r["built"][9])
        sf = sum(1 for r in rs if r["hands"][2])
        rf = sum(1 for r in rs if r["hands"][0])
        l4 = [r["firstline4"] for r in rs if r["firstline4"]]
        lw = [w for r in rs for w in r["llog"]]
        clr = f"{sum(r['clear'] for r in rs)}/{n}" if m != 3 else f"무너진 {st.median(r['wave'] for r in rs):.0f}"
        print(f"| {MODES[m]}{LABEL.get(g, ' ' + g)} | {n} | {hb}/{n} ({pct(hb / n)}) | {sb}/{n} | {sf}/{n} | {rf}/{n} | {len(l4)}/{n} · {iqr(l4)} | {st.mean(r['luckyhits'] for r in rs):.2f} · {iqr(lw)} | {clr} |")
    print()

    print("## 상위 조커 종류 (모든 묶음 합산, 조커 하나만 연 판 제외)\n")
    allr = [r for r in runs if r["mask"] == -1]
    c = Counter(u for r in allr for _, u in r["ulog"])
    runs_with = Counter()
    for r in allr:
        for u in set(u for _, u in r["ulog"]):
            runs_with[u] += 1
    waves = defaultdict(list)
    for r in allr:
        for w, u in r["ulog"]:
            waves[u].append(w)
    print(f"전체 {len(allr)}판, 상위 조커 {sum(c.values())}개, 한 번이라도 만든 판 {sum(1 for r in allr if r['ulog'])}판, 나온 종류 {len(c)}/30\n")
    print("| 상위 조커 | 만든 수 | 만든 판 | 만든 웨이브 중앙 |")
    print("|---|---|---|---|")
    for u, k in c.most_common():
        print(f"| {UPPERS[u]} | {k} | {runs_with[u]} | {st.median(waves[u]):.0f} |")
    never = [UPPERS[u] for u in range(30) if u not in c]
    print(f"\n한 번도 안 나온 {len(never)}종: " + " · ".join(never) + "\n")

    solo = [r for r in runs if r["group"] == "solo"]
    if solo:
        by = defaultdict(dict)
        for r in solo:
            by[r["mask"]][r["seed"]] = r
        base = by.get(0, {})
        print("## 조커 하나만 연 판 (60 모드, 10판, 판 위력이 10% 깎여도 만듦)\n")
        print("| 열린 조커 | 클리어 | 평균 무너진 웨이브 | 조커 없음 대비 | 판당 만든 수 | 만든 판 |")
        print("|---|---|---|---|---|---|")
        for mask, d in sorted(by.items()):
            rs = list(d.values())
            diffs = [d[s]["wave"] - base[s]["wave"] for s in d if s in base]
            dm = st.mean(diffs) if diffs else 0
            se = st.stdev(diffs) / math.sqrt(len(diffs)) if len(diffs) > 1 else 0
            name = "없음" if mask == 0 else "전부 (상위 조커 없이)" if mask == 1023 else NAMES[int(math.log2(mask))] + (" (노림)" if rs[0]["aimsf"] else "")
            print(f"| {name} | {sum(r['clear'] for r in rs)}/{len(rs)} | {st.mean(r['wave'] for r in rs):.1f} | {dm:+.1f} ± {1.96 * se:.1f} | {st.mean(sum(r['built']) for r in rs):.1f} | {pct(sum(1 for r in rs if sum(r['built'])) / len(rs))} |")
        print()

    print("## 작은 판 키스톤 (30 모드, 똑똑한 봇이 스스로 찍은 판)\n")
    for g in ("normal", "starter"):
        rs = [r for r in runs if r["group"] == g and r["mode"] == 0]
        if not rs:
            continue
        w = [r for r in rs if (r["keys"] >> 3) & 1]
        wo = [r for r in rs if not (r["keys"] >> 3) & 1]
        f = lambda x: f"{sum(r['clear'] for r in x)}/{len(x)}" if x else "—"
        print(f"- 30{LABEL.get(g, '')}: 작은 판 찍은 판 {len(w)}/{len(rs)} (클리어 {f(w)}), 안 찍은 판 클리어 {f(wo)}")
    print()

    if old:
        o = load(old)
        print("## 같은 씨앗 비교 (QA 26 판 → 이번, 똑똑한 봇 강화 0)\n")
        print("| 모드 | 씨앗 | 클리어 전 → 뒤 | 뒤에만 깬 판 · 전에만 깬 판 | 평균 무너진 웨이브 | 판당 조커 | 첫 조커 웨이브 중앙 | 10웨이브까지 |")
        print("|---|---|---|---|---|---|---|---|")
        for m, c_ in ((0, 0), (0, 1), (1, 0), (2, 0), (3, 0)):
            sel = lambda R, g: {r["seed"]: r for r in R if r["mode"] == m and r["content"] == c_ and r["mask"] == -1 and r["upg"] == 0 and r["smart"] == 1 and r["pursue"] == 0 and r.get("aimsf", 0) == 0 and r["group"] in g}
            a, b = sel(o, ("normal", "starter")), sel(runs, ("normal", "starter"))
            S = sorted(set(a) & set(b))
            if not S:
                continue
            fj = lambda R: st.median([min(w for w, _ in R[s]["log"]) for s in S if R[s]["log"]])
            by10 = lambda R: st.mean(sum(1 for w, _ in R[s]["log"] if w <= 10) for s in S)
            on = sum(1 for s in S if b[s]["clear"] and not a[s]["clear"])
            of = sum(1 for s in S if a[s]["clear"] and not b[s]["clear"])
            name = MODES[m] + (" 처음 상태" if c_ else "")
            print(f"| {name} | {len(S)} | {sum(a[s]['clear'] for s in S)} → {sum(b[s]['clear'] for s in S)} | {on} · {of} | {st.mean(a[s]['wave'] for s in S):.1f} → {st.mean(b[s]['wave'] for s in S):.1f} | {st.mean(sum(a[s]['built']) for s in S):.1f} → {st.mean(sum(b[s]['built']) for s in S):.1f} | {fj(a):.0f} → {fj(b):.0f} | {by10(a):.1f} → {by10(b):.1f} |")
        print()


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else None)
