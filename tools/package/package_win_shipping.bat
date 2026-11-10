@echo off
chcp 65001 >nul
rem 포커 디펜스 Windows Shipping 패키징 (exe)
rem   사용법: tools\package\package_win_shipping.bat [언리얼 엔진 폴더]
rem   기본 엔진 폴더: C:\Program Files\Epic Games\UE_5.8
rem   결과: Build\Shipping\Windows\ProjectB.exe (이 폴더째로 복사해서 실행)
rem   에디터는 닫고 실행할 것. 처음에는 셰이더 컴파일 때문에 오래 걸린다
setlocal
set "UE=C:\Program Files\Epic Games\UE_5.8"
if not "%~1"=="" set "UE=%~1"
set "ROOT=%~dp0..\.."
set "UAT=%UE%\Engine\Build\BatchFiles\RunUAT.bat"
if not exist "%UAT%" (
  echo [오류] RunUAT.bat 를 찾지 못했다: "%UAT%"
  echo 엔진 폴더를 인자로 넘겨 줘. 예: tools\package\package_win_shipping.bat "D:\Epic\UE_5.8"
  exit /b 1
)
call "%UAT%" BuildCookRun -project="%ROOT%\ProjectB.uproject" -platform=Win64 -clientconfig=Shipping ^
  -build -cook -stage -pak -archive -archivedirectory="%ROOT%\Build\Shipping" -prereqs -nodebuginfo -noP4 -utf8output
if errorlevel 1 (
  echo [실패] 위 로그의 첫 번째 Error 줄을 알려 줘
  exit /b 1
)
echo [완료] %ROOT%\Build\Shipping\Windows\ProjectB.exe
endlocal
