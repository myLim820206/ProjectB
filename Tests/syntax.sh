#!/usr/bin/env bash
# 언리얼 없이 Slate 화면 코드의 문법·형 검사 (g++ -fsyntax-only + 가짜 언리얼 헤더 Tests/fakeue/).
#   ./Tests/syntax.sh            대상 파일 전부
#   ./Tests/syntax.sh PDLang.cpp 몇 개만 (Source/ProjectB 기준 이름)
# 이건 진짜 빌드가 아니다: 가짜 헤더는 우리가 쓰는 API 의 선언만 흉내 낸다. 통과해도 언리얼 빌드는 따로 확인해야 한다.
# 실패가 우리 코드 문제인지, 가짜 헤더에 API 가 빠진 것인지 먼저 본다 (빠졌으면 실제 엔진 시그니처대로 Tests/fakeue/ 에 추가).
#
# 경고: 언리얼처럼 -Wall -Wextra -Wshadow 를 오류로. 늦춘 것:
#   -Wno-unused-parameter : Slate 가상 함수 덮어쓰기(OnPaint 의 Args 등)가 인자를 다 안 쓰는 게 정상이고, 언리얼(MSVC/clang)도 이 경고를 안 켠다
#   -Wno-missing-field-initializers : Floats.Add({..}) 처럼 일부만 채우는 집합 초기화를 언리얼도 경고하지 않는다
#   -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function : 언리얼 clang 툴체인도 끈다 (MSVC 도 오류로 안 친다). 정리 대상일 뿐 빌드는 깨지지 않음
#   -Wno-switch : 언리얼 clang 툴체인도 -Wno-switch 로 끈다 (enum 값을 switch 가 다 안 다뤄도 경고 없음). MSVC 도 C4062 꺼짐
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT/Source/ProjectB"
CXX=${CXX:-g++}
FLAGS=(-std=c++20 -fsyntax-only -Wall -Wextra -Wshadow -Werror -Wno-unused-parameter -Wno-missing-field-initializers -Wno-switch
	-Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function
	-I "$ROOT/Tests/fakeue" -I "$SRC")
FILES=("$@")
if [ ${#FILES[@]} -eq 0 ]; then # 기본: Source/ProjectB 의 .cpp 전부 (새 파일도 저절로 들어간다. PDCore·PDSynth 는 언리얼 없는 C++ 라 그대로)
	for F in "$SRC"/*.cpp; do FILES+=("${F##*/}"); done
fi

LOG="$(mktemp -d)"
trap 'rm -rf "$LOG"' EXIT
pids=()
for F in "${FILES[@]}"; do
	F="${F##*/}"
	( "$CXX" "${FLAGS[@]}" "$SRC/$F" > "$LOG/$F.log" 2>&1; echo $? > "$LOG/$F.rc" ) &
	pids+=($!)
done
wait "${pids[@]}"

FAIL=0
for F in "${FILES[@]}"; do
	F="${F##*/}"
	if [ "$(cat "$LOG/$F.rc")" = "0" ]; then
		echo "OK   $F"
	else
		echo "FAIL $F"
		sed 's/^/     /' "$LOG/$F.log" | head -n "${SYNTAX_MAX_LINES:-60}"
		FAIL=1
	fi
done
[ $FAIL -eq 0 ] && echo "문법 검사 통과 (가짜 헤더 기준, 언리얼 빌드 확인 전)" || echo "문법 검사 실패"
exit $FAIL
