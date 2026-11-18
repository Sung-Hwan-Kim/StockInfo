<p align="center">
  <img src="resources/icons/icon_128.png" width="96" alt="StockInfo 아이콘">
</p>

<h1 align="center">StockInfo — 코스피 · 코스닥 주식 정보</h1>

<p align="center">Qt 6 위젯으로 만든 멀티플랫폼(Windows / macOS / Linux) 주식 시세 뷰어</p>

## 실행 화면

| 다크 모드 (기본) | 라이트 모드 |
|:---:|:---:|
| <img src="docs/screenshot-dark.png" width="400" alt="다크 모드 실행 화면"> | <img src="docs/screenshot-light.png" width="400" alt="라이트 모드 실행 화면"> |

<p align="center">
  <img src="docs/settings-dark.png" width="560" alt="환경설정 창"><br>
  <sub>환경설정 창 — 갱신 시간, 글꼴 크기, 스킨, 종목 검색/추가</sub>
</p>

## 기능

- **코스피/코스닥 지수**를 크게 표시: 현재값, 전일 대비, 등락률, 시가/고가/저가, 장 상태, 거래대금
  - 창 크기에 맞춰 글자 크기가 자동으로 조절됩니다.
- **관심 종목 리스트**: 현재가, 대비, 등락률, 거래량 (상승 빨강 / 하락 파랑)
  - 종목이 많으면 스크롤바가 생깁니다.
  - 창이 좁으면 거래량 → 대비 순서로 열을 숨기고, 마우스를 올리면 툴팁에 전체 정보를 보여줍니다.
  - 종목의 실제 시장(코스피/코스닥)을 자동으로 판별해서 알맞은 목록에 표시합니다.
- **스킨**: 다크 모드(기본) / 라이트 모드
  - OS 테마와 상관없이 같은 모양으로 보이며, Windows에서는 제목 표시줄 색도 함께 바뀝니다.
  - [설정 → 스킨] 메뉴나 환경설정 창에서 바로 전환됩니다.
- **갱신 시간**을 초 단위로 설정 (1 ~ 3600초)
- **종목 추가**: 종목명 또는 코드로 검색 → 추가 / 삭제 / 순서 변경
- **창 위치·크기·최대화 상태 저장** → 다음 실행 때 그대로 복원
- 항상 위에 표시, 목록 글꼴 크기 설정
- 종목/지수 더블클릭 → 네이버 금융 페이지 열기
- 종목 목록에서 마우스 오른쪽 버튼 → 보기 / 삭제 / 종목 추가

## 단축키

| 키 | 동작 |
|---|---|
| F5 | 지금 새로고침 |
| Ctrl+, | 환경설정 |
| Ctrl+N | 종목 추가/관리 |
| Ctrl+Shift+D | 다크 모드 |
| Ctrl+Shift+L | 라이트 모드 |
| Ctrl+T | 항상 위에 표시 |
| Ctrl+Q | 종료 |

## 설정 파일 (INI)

실행 파일과 같은 폴더의 `StockInfo.ini`에 저장됩니다.
그 폴더에 쓸 수 없으면(예: Program Files, macOS 앱 번들) 사용자 설정 폴더에 저장됩니다.
정확한 경로는 환경설정 창 아래쪽이나 [도움말 → 정보]에서 볼 수 있습니다.

```ini
[Settings]
RefreshSeconds=5      ; 갱신 시간(초)
ListFontSize=11       ; 종목 목록 글꼴 크기(pt)
AlwaysOnTop=false
Skin=Dark             ; Dark(기본) 또는 Light

[Window]              ; 창 위치/크기 (자동 저장)
X=100
Y=60
Width=680
Height=860
Maximized=false

[Stocks]
size=2
1\Code=005930
1\Name=삼성전자
1\Market=KOSPI
2\Code=247540
2\Name=에코프로비엠
2\Market=KOSDAQ
```

저장된 위치가 현재 연결된 모니터 밖에 있으면(모니터를 뺀 경우 등) 기본 위치에 표시합니다.

## 빌드

### Windows (Visual Studio 2019 + Qt 6.7.0)

```bat
build_vs2019.bat
```

- 결과물: `build\Release\StockInfo.exe` (빌드 후 `windeployqt`가 Qt DLL을 자동으로 복사하므로 바로 실행 가능)
- Visual Studio 2019에서 열기: `build\StockInfo.sln`
- Debug 빌드: `build_vs2019.bat Debug`
- Qt 경로가 다르면 `build_vs2019.bat`의 `QT_DIR`을 수정하세요.

### macOS / Linux

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=<Qt 설치경로>/6.7.0/<macos|gcc_64>
cmake --build build
```

## 아이콘

| 파일 | 용도 |
|---|---|
| `resources/icons/StockInfo.ico` | Windows 실행 파일 아이콘 (`resources/StockInfo.rc.in`으로 내장, 버전 정보 포함) |
| `resources/icons/StockInfo.icns` | macOS 앱 번들 아이콘 |
| `resources/icons/icon_*.png` | 창 · 작업 표시줄 아이콘 (Qt 리소스로 내장) |

아이콘을 고치려면 `tools/make_icon.py`를 수정한 뒤 다시 생성합니다 (Pillow 필요).

```sh
python tools/make_icon.py
```

## 폴더 구조

```
src/            소스 코드 (MainWindow, IndexPanel, StockListWidget, SettingsDialog, Skin …)
resources/      아이콘, Windows 리소스(.rc) 템플릿
tools/          아이콘 생성 스크립트
docs/           README 스크린샷
```

## 데이터 출처

네이버 금융의 공개 시세 API(비공식)를 사용합니다.
API가 바뀌면 동작하지 않을 수 있고, 표시되는 시세는 참고용입니다.
