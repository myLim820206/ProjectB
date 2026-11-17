@echo off
rem Shipping package (D-165): VC++ runtime DLLs next to the exe (-applocaldirectory) and the installer too (-prereqs).
rem Skips the editor build so it works while the editor is Live Coding (-nocompileeditor). Output: Saved\PackagedShipping\Windows
rem D-367: bake the atlas first (tools\bake_atlas.bat, needs a finished editor build, editor closed). Skip with: package_shipping.bat nobake
rem Keep this file ASCII: cmd mis-parses UTF-8 Korean lines.
set UE=C:\Program Files\Epic Games\UE_5.8\Engine
if /i not "%~1"=="nobake" (
  call "%~dp0bake_atlas.bat"
  if errorlevel 1 exit /b 1
)
call "%UE%\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%~dp0..\ProjectB.uproject" -platform=Win64 -clientconfig=Shipping -build -nocompileeditor -cook -stage -pak -prereqs "-applocaldirectory=%UE%\Binaries\ThirdParty\AppLocalDependencies" -archive -archivedirectory="%~dp0..\Saved\PackagedShipping" -nodebuginfo -unattended -utf8output
rem D-282: no PDB in the shipping package (-nodebuginfo, and delete leftovers)
del /s /q "%~dp0..\Saved\PackagedShipping\*.pdb" >nul 2>&1
