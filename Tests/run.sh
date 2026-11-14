#!/usr/bin/env bash
# 포커 디펜스 규칙 코어를 언리얼 없이 컴파일하고 시험한다. 경고는 오류로 취급 (언리얼 빌드와 비슷한 엄격함)
#   ./Tests/run.sh [봇 판 수, 0이면 봇 생략]
set -euo pipefail
cd "$(dirname "$0")"
CXX=${CXX:-g++}
mkdir -p build
# 새 파일로 만든 뒤 바꿔치기: 돌고 있는 봇 시험의 실행 파일을 덮어쓰지 않는다
OUT=build/core_test.$$
"$CXX" -std=c++17 -O2 -Wall -Wextra -Wshadow -Werror -o "$OUT" core_test.cpp ../Source/ProjectB/PDCore.cpp ../Source/ProjectB/PDSynth.cpp
mv -f "$OUT" build/core_test
./build/core_test "$@"
