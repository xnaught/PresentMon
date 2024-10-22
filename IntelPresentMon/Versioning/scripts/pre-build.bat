setlocal

:: Generate a unique identifier for the build (e.g., based on date-time)
for /f "delims=" %%i in ('powershell -Command "Get-Date -Format yyyy.M.d.HH:mm:ss"') do set DATETIME=%%i

:: Get the current git commit hash (full and short)
for /f "delims=" %%i in ('git rev-parse HEAD') do set GIT_HASH=%%i
for /f "delims=" %%i in ('git rev-parse --short HEAD') do set GIT_HASH_SHORT=%%i

:: Check for uncommitted changes
call git diff --quiet
if errorlevel 1 (
    set UNCOMMITTED_CHANGES=true
) else (
    set UNCOMMITTED_CHANGES=false
)

:: Write to header file in the generated directory
if not exist generated mkdir generated
echo #define PM_BID_GIT_HASH "%GIT_HASH%" > generated\build_id.h
echo #define PM_BID_GIT_HASH_SHORT "%GIT_HASH_SHORT%" >> generated\build_id.h
echo #define PM_BID_TIME "%DATETIME%" >> generated\build_id.h
echo #define PM_BID_UID "%GIT_HASH%-%DATETIME%" >> generated\build_id.h
echo #define PM_BID_DIRTY %UNCOMMITTED_CHANGES% >> generated\build_id.h

endlocal
