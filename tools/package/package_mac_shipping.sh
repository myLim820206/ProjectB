#!/usr/bin/env bash
# 포커 디펜스 macOS Shipping 패키징 (.app)
#   사용법: tools/package/package_mac_shipping.sh [언리얼 엔진 폴더]
#   기본 엔진 폴더: /Users/Shared/Epic Games/UE_5.8
#   결과: Build/Shipping/Mac/ProjectB.app (다른 맥에 줄 때는 서명·공증이 따로 필요)
set -euo pipefail
UE="${1:-/Users/Shared/Epic Games/UE_5.8}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
UAT="$UE/Engine/Build/BatchFiles/RunUAT.sh"
[ -x "$UAT" ] || { echo "[오류] RunUAT.sh 를 찾지 못했다: $UAT"; exit 1; }
"$UAT" BuildCookRun -project="$ROOT/ProjectB.uproject" -platform=Mac -clientconfig=Shipping \
  -build -cook -stage -pak -archive -archivedirectory="$ROOT/Build/Shipping" -nodebuginfo -noP4 -utf8output
echo "[완료] $ROOT/Build/Shipping/Mac/ProjectB.app"
