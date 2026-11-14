@echo off
rem 쉬핑 패키지 (D-165): VC++ 런타임 DLL 을 실행 파일 옆에 넣고(-applocaldirectory), 설치 프로그램도 담는다(-prereqs)
rem 에디터가 Live Coding 중이어도 되게 에디터 빌드는 건너뛴다(-nocompileeditor). 결과: Saved\PackagedShipping\Windows
set UE=C:\Program Files\Epic Games\UE_5.8\Engine
call "%UE%\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%~dp0..\ProjectB.uproject" -platform=Win64 -clientconfig=Shipping -build -nocompileeditor -cook -stage -pak -prereqs "-applocaldirectory=%UE%\Binaries\ThirdParty\AppLocalDependencies" -archive -archivedirectory="%~dp0..\Saved\PackagedShipping" -unattended -utf8output
