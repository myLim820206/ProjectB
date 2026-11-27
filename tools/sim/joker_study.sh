#!/usr/bin/env bash
# 조커 · 모드 연구 (이슈 #5): 봇을 판마다 따로 돌려 CPU 코어만큼 나눠 돌린다.
#   tools/sim/joker_study.sh <결과 폴더> [동시 프로세스 수]
# 일감 목록은 아래 JOBS (모드 강화 해금 조커비트 똑똑함 노림 문턱 씨앗수 묶음이름). 끝나면 joker_study.py 로 모은다.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${1:?결과 폴더}"
PAR="${2:-$(nproc)}"
mkdir -p "$OUT"
cd "$ROOT/Tests"
mkdir -p build
CXX=${CXX:-clang++}
"$CXX" -std=c++17 -O2 -Wall -Wextra -Wshadow -Werror -o build/study core_test.cpp ../Source/ProjectB/PDCore.cpp ../Source/ProjectB/PDSynth.cpp
BIN="$ROOT/Tests/build/study"
JOBS=(
	# 일반 난이도 · 자연스러운 조커 수 (똑똑한 조커 봇, 모든 조커 열림)
	"2 0 0 -1 1 0 0 20 normal"
	"2 10 0 -1 1 0 0 20 normal"
	"1 0 0 -1 1 0 0 30 normal"
	"1 10 0 -1 1 0 0 30 normal"
	"0 0 0 -1 1 0 0 40 normal"
	"0 10 0 -1 1 0 0 40 normal"
	"0 0 1 -1 1 0 0 40 starter"
	"0 0 0 -1 0 0 0 40 oldbot"
	# 조커 하나만 연 판 (60 모드, 강화 없음, 판 위력이 10% 깎여도 만든다)
	"1 0 0 0 1 0 0.10 20 solo"
	"1 0 0 1023 1 0 0.10 20 solo"
)
for J in 0 1 2 3 4 5 6 7 8 9; do JOBS+=("1 0 0 $((1 << J)) 1 0 0.10 20 solo"); done
LIST="$OUT/jobs.txt"
: > "$LIST"
for Line in "${JOBS[@]}"; do
	set -- $Line
	for ((S = 0; S < $8; ++S)); do echo "$1 $2 $3 $4 $5 $6 $7 $S $9" >> "$LIST"; done
done
# 오래 걸리는 100 모드부터 (끝이 고르게)
echo "일감 $(wc -l < "$LIST")개, 동시 $PAR"
xargs -P "$PAR" -L 1 bash -c 'f="'"$OUT"'/$8-m$0-u$1-c$2-k$3-s$4-$7.txt"; [ -s "$f" ] && grep -q "^STUDY" "$f" && exit 0; "'"$BIN"'" 1 study $0 $1 $2 $3 $7 $4 $5 $6 > "$f.tmp" && mv "$f.tmp" "$f"' < "$LIST"
echo "끝"
