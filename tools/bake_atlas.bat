@echo off
rem Atlas bake (D-367): replaces editor F8. Opens the editor binary in a -game window, bakes, saves Content/Sprites/T_PDAtlas_vN and quits (~20 s).
rem Baking is a GPU render, so a -nullrhi commandlet cannot do it. Close the editor first (it cannot overwrite assets it has loaded).
rem Keep this file ASCII: cmd mis-parses UTF-8 Korean lines.
setlocal
set UE=C:\Program Files\Epic Games\UE_5.8\Engine
set LOG=%~dp0..\Saved\Logs\BakeAtlas.log
del "%LOG%" >nul 2>&1
start "" /wait "%UE%\Binaries\Win64\UnrealEditor.exe" "%~dp0..\ProjectB.uproject" -game -windowed -ResX=1280 -ResY=720 -pdbakeatlas -unattended -abslog="%LOG%"
findstr /c:"PDATLAS bake done ok" "%LOG%" >nul
if errorlevel 1 (
  echo [bake_atlas] FAILED - see %LOG%
  exit /b 1
)
echo [bake_atlas] OK
exit /b 0
