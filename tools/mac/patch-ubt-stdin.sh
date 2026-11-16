#!/bin/bash
# UE 5.8 + Xcode 27: 게임 타깃(.app) 빌드의 마지막 단계(xcodebuild PostBuildSync)가
# "Failed to launch task /bin/sh: Bad file descriptor" 로 실패하는 문제를 고친다.
# 원인: UBA 가 이 단계를 stdin 이 닫힌 채로 띄우고, Xcode 27 은 그 상태에서 스킴 사전 스크립트를 못 띄운다.
# 수정: AppleToolChain.cs 에서 xcodebuild 를 /bin/sh 로 감싸 stdin 을 /dev/null 로 준다. (D-080)
#
# 런처에서 엔진을 '확인'하거나 업데이트하면 원래대로 돌아가므로 그때 다시 실행한다.
# 되돌리기: 백업(.orig)을 원래 이름으로 복사.
set -euo pipefail

ENGINE="${1:-/Users/Shared/Epic Games/UE_5.8/Engine}"
UBT_SRC="$ENGINE/Source/Programs/UnrealBuildTool"
FILE="$UBT_SRC/Platform/Apple/AppleToolChain.cs"
ORIG_LINE='					LocalProcess.StartInfo = new ProcessStartInfo("/usr/bin/env", String.Join(" ", Arguments));'

if grep -q "LOCAL PATCH (ProjectB)" "$FILE"; then
  echo "소스는 이미 고쳐져 있음: $FILE"
else
  grep -qF "$ORIG_LINE" "$FILE" || { echo "고칠 줄을 못 찾음 (엔진 버전이 다름?): $FILE" >&2; exit 1; }
  [ -f "$FILE.orig" ] || cp -p "$FILE" "$FILE.orig"
  chmod u+w "$FILE"
  python3 - "$FILE" "$ORIG_LINE" <<'PY'
import sys
path, orig = sys.argv[1], sys.argv[2]
t = "\t\t\t\t\t"
new = (
    t + '// LOCAL PATCH (ProjectB): UBA runs this action with stdin closed, and Xcode 27 then fails to launch\n'
    + t + '// scheme pre-action scripts ("Failed to launch task /bin/sh: Bad file descriptor"). Give xcodebuild /dev/null as stdin.\n'
    + t + '// Original line: new ProcessStartInfo("/usr/bin/env", String.Join(" ", Arguments));\n'
    + t + 'LocalProcess.StartInfo = new ProcessStartInfo("/bin/sh");\n'
    + t + 'LocalProcess.StartInfo.ArgumentList.Add("-c");\n'
    + t + 'LocalProcess.StartInfo.ArgumentList.Add("exec /usr/bin/env " + String.Join(" ", Arguments) + " </dev/null");'
)
s = open(path, encoding="utf-8-sig").read()
open(path, "w", encoding="utf-8").write(s.replace(orig, new, 1))
PY
  echo "소스 고침: $FILE"
fi

export DOTNET_ROOT="$ENGINE/Binaries/ThirdParty/DotNet/10.0/mac-arm64"
export PATH="$DOTNET_ROOT:$PATH" DOTNET_ROLL_FORWARD=Major DOTNET_CLI_TELEMETRY_OPTOUT=1
(cd "$UBT_SRC" && dotnet build UnrealBuildTool.csproj -c Development -nologo -v q)

NEW="$UBT_SRC/bin/Development/net10.0/UnrealBuildTool.dll"
for d in UnrealBuildTool AutomationTool; do
  DST="$ENGINE/Binaries/DotNET/$d/UnrealBuildTool.dll"
  [ -f "$DST.orig" ] || cp -p "$DST" "$DST.orig"
  cp "$NEW" "$DST"
  echo "설치: $DST"
done
echo "완료"
