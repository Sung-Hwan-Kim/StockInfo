@echo off
chcp 65001 >nul
rem ============================================================
rem  StockInfo - Visual Studio 2019 + Qt 6.7.0 (msvc2019_64) 빌드
rem  사용법: build_vs2019.bat [Release^|Debug]
rem  결과물: build\Release\StockInfo.exe
rem  VS2019에서 열기: build\StockInfo.sln
rem ============================================================
setlocal

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Release"

set "QT_DIR=C:\Qt\Qt6.7.0\6.7.0\msvc2019_64"
if not exist "%QT_DIR%\bin\qmake.exe" (
    echo [오류] Qt를 찾을 수 없습니다: %QT_DIR%
    exit /b 1
)

cd /d "%~dp0"

cmake -S . -B build -G "Visual Studio 16 2019" -A x64 -DCMAKE_PREFIX_PATH="%QT_DIR%"
if errorlevel 1 exit /b 1

cmake --build build --config %CONFIG% --parallel
if errorlevel 1 exit /b 1

echo.
echo 빌드 완료: %~dp0build\%CONFIG%\StockInfo.exe
endlocal
