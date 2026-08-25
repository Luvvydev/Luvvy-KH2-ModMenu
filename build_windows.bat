@echo off
setlocal
where cl >nul 2>nul
if errorlevel 1 (
  echo Run this from a Visual Studio x64 Native Tools Command Prompt.
  exit /b 1
)

if not exist build mkdir build

cl /nologo /O2 /LD /I src src\proxy\dbghelp_proxy.c /link /DEF:src\proxy\dbghelp.def /OUT:build\dbghelp.dll kernel32.lib
if errorlevel 1 exit /b 1

cl /nologo /O2 /LD /I src src\modmenu\modmenu.c /link /OUT:build\KH2ModMenu.dll kernel32.lib user32.lib gdi32.lib
if errorlevel 1 exit /b 1

cl /nologo /O2 /GS- /c src\launcher\launcher.c /Fo:build\launcher.obj
if errorlevel 1 exit /b 1
link /nologo /SUBSYSTEM:WINDOWS /ENTRY:LauncherEntry /NODEFAULTLIB /OUT:build\Luvvy-KH2-Launch.exe build\launcher.obj kernel32.lib user32.lib
if errorlevel 1 exit /b 1

>build\steam_appid.txt echo 2552430

echo.
echo Built:
echo   build\dbghelp.dll
echo   build\KH2ModMenu.dll
echo   build\Luvvy-KH2-Launch.exe
echo   build\steam_appid.txt
