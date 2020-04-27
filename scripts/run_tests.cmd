@echo off
setlocal enabledelayedexpansion

set version=
for /f "tokens=2,3 delims=<>" %%a in ('type "%~dp0..\presentmon\presentmon.vcxproj" ^| findstr "<PresentMonVersion>"') do (
    if "%%a"=="PresentMonVersion" set version=%%b
)
if "%version%"=="" (
    echo [31mFAIL: could not determine PresentMonVersion[0m
    @exit /b 1
)

echo [93mTesting PresentMon %version%[0m
echo.

set build_platforms=
set build_platforms=%build_platforms% x86
set build_platforms=%build_platforms% x64
::set build_platforms=%build_platforms% arm
::set build_platforms=%build_platforms% arm64

set build_configs=
set build_configs=%build_configs% debug
set build_configs=%build_configs% release

set exe_prefixes=
set exe_prefixes=%exe_prefixes% 32
set exe_prefixes=%exe_prefixes% 64
::set exe_prefixes=%exe_prefixes% arm
::set exe_prefixes=%exe_prefixes% arm64

echo [96mBuilding...[90m
for %%a in (%build_platforms%) do for %%b in (%build_configs%) do (
    echo %%a-%%b
    msbuild /nologo /verbosity:quiet /p:Platform=%%a,Configuration=%%b "%~dp0..\PresentMon.sln"
    if not "!errorlevel!"=="0" (
        echo [31mFAIL[0m
        exit /b 1
    )
)

echo.
echo [96mChecking generated files...[0m
set errorcount=0
for %%a in (%exe_prefixes%) do for %%b in (%build_configs%) do call :check_exist "%~dp0..\build\%%b\bin\PresentMonTests%%a.exe"
for %%a in (%exe_prefixes%) do for %%b in (%build_configs%) do call :check_pm "%~dp0..\build\%%b\bin\PresentMon%%a-%version%.exe"
if not "%errorcount%"=="0" exit /b 1


echo.
echo [96mTesting functionality...[0m
for %%a in (%exe_prefixes%) do for %%b in (%build_configs%) do call :gtests "%~dp0..\build\%%b\bin\PresentMon%%a-%version%.exe"
echo.
echo.
if not "%errorcount%"=="0" (
    echo [31mFAIL[0m
    exit /b 1
)

echo [32mPASS[0m
exit /b 0

:check_exist
    echo [90m%~dpnx1[0m
    if exist %1 exit /b 0
    echo [31merror: expected build output does not exist[0m
    set /a errorcount=%errorcount%+1
    exit /b 1

:check_pm
    call :check_exist %1
    if not "%errorlevel%"=="0" exit /b 1

    set appver=
    for /f "tokens=1,2" %%a in ('%1 --version 2^>^&1') do if "%%a"=="PresentMon" if not "%%b"=="requires" set appver=%%b
    if "%appver%"=="development" set appver=dev
    if not "%version%"=="%appver%" (
        echo [31mApp and build disagree on version: %version% vs. %appver%[0m
        set /a errorcount=%errorcount%+1
        exit /b 1
    )
    exit /b 0

:gtests
    echo [90m%~1[0m
    "%~dp0..\build\release\bin\PresentMonTests64.exe" --presentmon=%1 --testdir="%~dp0..\tests\gold" --delete
    if not "%errorlevel%"=="0" set /a errorcount=%errorcount%+1
    echo.
    exit /b 1
