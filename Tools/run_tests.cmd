:: Copyright 2020 Intel Corporation
:: 
:: Permission is hereby granted, free of charge, to any person obtaining a copy
:: of this software and associated documentation files (the "Software"), to
:: deal in the Software without restriction, including without limitation the
:: rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
:: sell copies of the Software, and to permit persons to whom the Software is
:: furnished to do so, subject to the following conditions:
:: 
:: The above copyright notice and this permission notice shall be included in
:: all copies or substantial portions of the Software.
:: 
:: THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
:: IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
:: FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
:: AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
:: LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
:: FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
:: IN THE SOFTWARE.
@echo off
setlocal enabledelayedexpansion

set version=
for /f "tokens=2,3 delims=<>" %%a in ('type "%~dp0..\PresentMon.props" ^| findstr "<PresentMonVersion>"') do (
    if "%%a"=="PresentMonVersion" set version=%%b
)
if "%version%"=="" (
    echo [31mFAIL: could not determine PresentMonVersion[0m
    exit /b 1
)

echo [40;93mTesting PresentMon %version%
echo.

set build_configs=
set build_configs=%build_configs% debug
set build_configs=%build_configs% release

set build_platforms=
set build_platforms=%build_platforms% x86
set build_platforms=%build_platforms% x64
set build_platforms=%build_platforms% arm
set build_platforms=%build_platforms% arm64

set test_platforms=
set test_platforms=%test_platforms% x86
set test_platforms=%test_platforms% x64

echo [96mBuilding...[90m
for %%a in (%build_platforms%) do for %%b in (%build_configs%) do (
    echo %%a-%%b
    msbuild /nologo /verbosity:quiet /maxCpuCount /property:Platform=%%a,Configuration=%%b "%~dp0..\PresentMon.sln"
    if not "!errorlevel!"=="0" (
        echo [31mFAIL[0m
        exit /b 1
    )
)

echo.
echo [96mChecking generated files...[0m
set errorcount=0
for %%a in (%build_platforms%) do for %%b in (%build_configs%) do call :check_exist "%~dp0..\build\%%b\PresentMon-%version%-%%a.exe"
for %%a in (%build_platforms%) do for %%b in (%build_configs%) do call :check_exist "%~dp0..\build\%%b\PresentMonTests-%version%-%%a.exe"
for %%a in (%test_platforms%)  do for %%b in (%build_configs%) do call :check_pm_version "%~dp0..\build\%%b\PresentMon-%version%-%%a.exe"
if not "%errorcount%"=="0" exit /b 1

echo.
echo [96mTesting functionality...[0m
for %%a in (%test_platforms%) do for %%b in (%build_configs%) do call :gtests "%~dp0..\build\%%b\PresentMon-%version%-%%a.exe"
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

:check_pm_version
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
    echo [90m%~f1[0m
    "%~dp0..\build\release\PresentMonTests-%version%-x64.exe" --presentmon=%1 --testdir="%~dp0..\Tests\Gold" --delete
    if not "%errorlevel%"=="0" set /a errorcount=%errorcount%+1
    echo.
    exit /b 1
