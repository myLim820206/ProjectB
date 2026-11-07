#!/usr/bin/env bash
# 규칙 코어를 언리얼 없이 컴파일하고 시험한다. 경고는 오류로 취급 (언리얼 빌드와 비슷한 엄격함)
set -euo pipefail
cd "$(dirname "$0")"
CXX=${CXX:-g++}
mkdir -p build
"$CXX" -std=c++17 -O2 -Wall -Wextra -Wshadow -Werror -o build/core_test core_test.cpp ../Source/ProjectB/TPCore.cpp ../Source/ProjectB/TPSynth.cpp
./build/core_test "$@"
