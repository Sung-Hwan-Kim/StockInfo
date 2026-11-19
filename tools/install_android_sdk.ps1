# Android 빌드 도구 설치 (Qt 6.7 기준: JDK 17, platform 34, build-tools 34, NDK r26b, OpenSSL 3)
# 사용법: powershell -ExecutionPolicy Bypass -File tools\install_android_sdk.ps1 [-SdkRoot C:\Android\Sdk] [-NoEmulator]
param(
    [string]$SdkRoot = "C:\Android\Sdk",
    [string]$JavaHome = "",
    [switch]$NoEmulator
)
$ErrorActionPreference = "Stop"
if ($SdkRoot -match "\s") { throw "SDK 경로에 공백이 없어야 합니다: $SdkRoot" }

if (-not $JavaHome) {
    $jdk = Get-ChildItem "C:\Program Files\Microsoft" -Directory -Filter "jdk-17*" -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $jdk) { throw "JDK 17을 찾을 수 없습니다. (winget install Microsoft.OpenJDK.17)" }
    $JavaHome = $jdk.FullName
}
$env:JAVA_HOME = $JavaHome
Write-Host "JAVA_HOME = $JavaHome"

# 1) 명령줄 도구 (Google 공식)
$latest = Join-Path $SdkRoot "cmdline-tools\latest"
if (-not (Test-Path "$latest\bin\sdkmanager.bat")) {
    New-Item -ItemType Directory -Force $SdkRoot | Out-Null
    $dl = Join-Path $SdkRoot "_download"
    New-Item -ItemType Directory -Force $dl | Out-Null
    $zip = Join-Path $dl "commandlinetools-win.zip"
    Write-Host "명령줄 도구 내려받는 중..."
    Invoke-WebRequest -UseBasicParsing "https://dl.google.com/android/repository/commandlinetools-win-13114758_latest.zip" -OutFile $zip
    Expand-Archive $zip -DestinationPath $dl -Force
    New-Item -ItemType Directory -Force (Join-Path $SdkRoot "cmdline-tools") | Out-Null
    if (Test-Path $latest) { Remove-Item $latest -Recurse -Force }
    Move-Item (Join-Path $dl "cmdline-tools") $latest
    Remove-Item $dl -Recurse -Force
}
$sdkmanager = "$latest\bin\sdkmanager.bat"

# 2) 라이선스 동의: "y" 를 여러 줄 담은 파일을 cmd 입력 리디렉션으로 전달
#    (PowerShell 파이프는 sdkmanager.bat 까지 입력이 전달되지 않는 경우가 있음)
Write-Host "Android SDK 라이선스 동의..."
$yesFile = Join-Path $SdkRoot "_yes.txt"
[IO.File]::WriteAllText($yesFile, ((1..80 | ForEach-Object { "y" }) -join "`r`n") + "`r`n")
cmd /c "$sdkmanager --sdk_root=$SdkRoot --licenses < $yesFile" | Out-Null

# 3) 패키지 설치
$packages = @("platform-tools", "platforms;android-34", "build-tools;34.0.0", "ndk;26.1.10909125")
if (-not $NoEmulator) {
    $packages += @("emulator", "system-images;android-34;google_apis;x86_64")
}
Write-Host ("설치: " + ($packages -join ", "))
# 설치 중에도 라이선스 확인을 물으면 y
$pkgArgs = ($packages | ForEach-Object { "`"$_`"" }) -join " "
cmd /c "$sdkmanager --sdk_root=$SdkRoot $pkgArgs < $yesFile"
if ($LASTEXITCODE -ne 0) { throw "sdkmanager 실패 ($LASTEXITCODE)" }
Remove-Item $yesFile -ErrorAction SilentlyContinue

# 실제로 설치됐는지 확인 (라이선스 문제로 건너뛰어도 종료 코드가 0일 수 있음)
$required = @("platforms/android-34", "build-tools/34.0.0", "ndk/26.1.10909125", "platform-tools")
if (-not $NoEmulator) { $required += @("emulator", "system-images/android-34/google_apis/x86_64") }
$missing = $required | Where-Object { -not (Test-Path (Join-Path $SdkRoot $_)) }
if ($missing) { throw ("설치되지 않은 패키지: " + ($missing -join ", ")) }

# 4) 안드로이드용 OpenSSL 3 (Qt 네트워크의 HTTPS에 필요. Qt 문서가 안내하는 KDAB android_openssl 빌드)
#    https://github.com/KDAB/android_openssl
$opensslDir = Join-Path (Split-Path $SdkRoot -Parent) "android_openssl"
foreach ($abi in @("arm64-v8a", "x86_64")) {
    $dir = Join-Path $opensslDir "ssl_3/$abi"
    New-Item -ItemType Directory -Force $dir | Out-Null
    foreach ($lib in @("libcrypto_3.so", "libssl_3.so")) {
        $dst = Join-Path $dir $lib
        if (-not (Test-Path $dst)) {
            Write-Host "OpenSSL 내려받는 중: $abi/$lib"
            Invoke-WebRequest -UseBasicParsing "https://github.com/KDAB/android_openssl/raw/master/ssl_3/$abi/$lib" -OutFile $dst
        }
    }
}
Write-Host "OpenSSL: $opensslDir"

& $sdkmanager --sdk_root=$SdkRoot --list_installed
Write-Host "완료: $SdkRoot"
