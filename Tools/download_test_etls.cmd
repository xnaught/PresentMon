@echo off
setlocal
set src="https://github.com/GameTechDev/PresentMon/releases/download/PresentMonTestEtls/PresentMonTestEtls.exe"
set dst="%~dp0..\Tests\Gold\PresentMonTestEtls.exe"
set ba=
set sz=
for %%a in (bitsadmin.exe) do set ba=%%~$PATH:a
for %%a in (7z.exe)        do set sz=%%~$PATH:a
if "%ba%"=="" goto manual
if not exist "%~dp0..\Tests\Gold\." goto manual

echo Downloading %src% into %dst%
bitsadmin /transfer dl /download /dynamic /priority FOREGROUND %src% %dst% > NUL
if %errorlevel% neq 0 goto manual
if not exist %dst% goto manual

echo Uncompressing PresentMonTestEtls.exe
pushd "%~dp0..\Tests\Gold"
rem BEGIN WORKAROUND: Windows security seems to think it's a virus if I try to
rem run it right away, so get user to run it explicitly or use 7z if it is
rem available.
::PresentMonTestEtls.exe
if "%sz%"=="" (
    echo Run PresentMonTestEtls.exe in the Tests\Gold directory to uncompress the test ETLs
    explorer .
    pause
) else (
    "%sz%" x PresentMonTestEtls.exe > NUL
)
:: END WORKAROUND
popd
exit /b 0

:manual
    echo error: failed to download PresentMonTestEtls
    echo.
    echo To do this manually:
    echo 1^) Download the compressed ETLs https://github.com/GameTechDev/PresentMon/releases/download/PresentMonTestEtls/PresentMonTestEtls.exe into Tests\Gold\
    echo 2^) Unzip the ETLs by running Tests\Gold\PresentMonTestEtls.exe
    exit /b 1
