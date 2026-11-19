@echo off
chcp 65001 >nul
rem ============================================================
rem  StockInfo 안드로이드 APK 빌드 (Qt 6.7.0 + Android SDK/NDK r26b + JDK 17)
rem  사용법: build_android.bat [Release^|Debug]
rem  결과물: build-android\StockInfo-android.apk (서명됨, 폰/태블릿/에뮬레이터 공용)
rem  도구 설치: tools\install_android_sdk.ps1
rem ============================================================
setlocal

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Release"

set "QT_ROOT=C:\Qt\Qt6.7.0\6.7.0"
if not defined ANDROID_SDK_ROOT set "ANDROID_SDK_ROOT=C:\Android\Sdk"
set "ANDROID_NDK_ROOT=%ANDROID_SDK_ROOT%\ndk\26.1.10909125"
if not defined JAVA_HOME for /d %%D in ("C:\Program Files\Microsoft\jdk-17*") do set "JAVA_HOME=%%~D"
set "NINJA=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"

if not exist "%QT_ROOT%\android_arm64_v8a\bin\qt-cmake.bat" (
    echo [오류] 안드로이드용 Qt를 찾을 수 없습니다: %QT_ROOT%\android_arm64_v8a
    exit /b 1
)
if not exist "%ANDROID_NDK_ROOT%" (
    echo [오류] Android NDK를 찾을 수 없습니다: %ANDROID_NDK_ROOT%
    echo        tools\install_android_sdk.ps1 로 설치하세요.
    exit /b 1
)
if not exist "%JAVA_HOME%\bin\java.exe" (
    echo [오류] JDK 17을 찾을 수 없습니다. winget install Microsoft.OpenJDK.17
    exit /b 1
)
rem JDK 17(Windows)에서 임시 폴더 경로에 따라 Gradle이 "Unable to establish loopback connection"으로
rem 실패하는 문제가 있어, Java 소켓용 임시 폴더를 따로 지정한다
if not exist "%ANDROID_SDK_ROOT%\.sock" mkdir "%ANDROID_SDK_ROOT%\.sock"
set "JAVA_TOOL_OPTIONS=-Djdk.net.unixdomain.tmpdir=%ANDROID_SDK_ROOT%\.sock"

echo JAVA_HOME=%JAVA_HOME%
echo ANDROID_SDK_ROOT=%ANDROID_SDK_ROOT%

cd /d "%~dp0"

rem 폰/태블릿(arm64-v8a)과 에뮬레이터(x86_64)를 한 APK에
call "%QT_ROOT%\android_arm64_v8a\bin\qt-cmake.bat" -S . -B build-android -G Ninja ^
    -DCMAKE_MAKE_PROGRAM="%NINJA%" ^
    -DCMAKE_BUILD_TYPE=%CONFIG% ^
    -DANDROID_SDK_ROOT="%ANDROID_SDK_ROOT%" ^
    -DANDROID_NDK_ROOT="%ANDROID_NDK_ROOT%" ^
    -DQT_HOST_PATH="%QT_ROOT%\msvc2019_64" ^
    -DQT_ANDROID_ABIS="arm64-v8a;x86_64"
if errorlevel 1 exit /b 1

cmake --build build-android --target apk
if errorlevel 1 exit /b 1

rem ── APK 서명 ──
rem Release APK는 서명이 없으면 설치할 수 없다.
rem 기본: Android 표준 디버그 키(%USERPROFILE%\.android\debug.keystore, 없으면 만든다)
rem 직접 만든 배포용 키를 쓰려면: STOCKINFO_KEYSTORE, STOCKINFO_KEY_ALIAS, STOCKINFO_KEYSTORE_PASS 환경 변수 지정
set "BUILD_TOOLS=%ANDROID_SDK_ROOT%\build-tools\34.0.0"
set "UNSIGNED=build-android\mobile\android-build\build\outputs\apk\release\android-build-release-unsigned.apk"
if /i "%CONFIG%"=="Debug" set "UNSIGNED=build-android\mobile\android-build\build\outputs\apk\debug\android-build-debug.apk"
set "OUT_APK=build-android\StockInfo-android.apk"

if not defined STOCKINFO_KEYSTORE (
    set "STOCKINFO_KEYSTORE=%USERPROFILE%\.android\debug.keystore"
    set "STOCKINFO_KEY_ALIAS=androiddebugkey"
    set "STOCKINFO_KEYSTORE_PASS=android"
)
if not exist "%STOCKINFO_KEYSTORE%" (
    if not exist "%USERPROFILE%\.android" mkdir "%USERPROFILE%\.android"
    "%JAVA_HOME%\bin\keytool.exe" -genkeypair -keystore "%STOCKINFO_KEYSTORE%" -storepass %STOCKINFO_KEYSTORE_PASS% ^
        -alias %STOCKINFO_KEY_ALIAS% -keypass %STOCKINFO_KEYSTORE_PASS% -keyalg RSA -keysize 2048 -validity 10000 ^
        -dname "CN=Android Debug,O=Android,C=US"
    if errorlevel 1 exit /b 1
)

"%BUILD_TOOLS%\zipalign.exe" -p -f 4 "%UNSIGNED%" build-android\aligned.apk
if errorlevel 1 exit /b 1
call "%BUILD_TOOLS%\apksigner.bat" sign --ks "%STOCKINFO_KEYSTORE%" --ks-key-alias %STOCKINFO_KEY_ALIAS% ^
    --ks-pass pass:%STOCKINFO_KEYSTORE_PASS% --key-pass pass:%STOCKINFO_KEYSTORE_PASS% --out "%OUT_APK%" build-android\aligned.apk
if errorlevel 1 exit /b 1
del build-android\aligned.apk
call "%BUILD_TOOLS%\apksigner.bat" verify "%OUT_APK%"
if errorlevel 1 exit /b 1

echo.
echo 빌드 완료: %~dp0%OUT_APK%
endlocal
