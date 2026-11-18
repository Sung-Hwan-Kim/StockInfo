cmake_minimum_required(VERSION 3.21)

project(StockInfo VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Network)
qt_standard_project_setup()

qt_add_executable(StockInfo
    src/main.cpp
    src/AppSettings.h
    src/AppSettings.cpp
    src/StockService.h
    src/StockService.cpp
    src/Theme.h
    src/IndexPanel.h
    src/IndexPanel.cpp
    src/StockListWidget.h
    src/StockListWidget.cpp
    src/SettingsDialog.h
    src/SettingsDialog.cpp
    src/MainWindow.h
    src/MainWindow.cpp
)

set_target_properties(StockInfo PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.stockinfo.viewer"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
)

target_link_libraries(StockInfo PRIVATE Qt6::Widgets Qt6::Network)

if(MSVC)
    # 소스 파일의 한글 문자열을 UTF-8로 처리
    target_compile_options(StockInfo PRIVATE /utf-8 /W3)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT StockInfo)
endif()

# Windows: 빌드 후 Qt DLL/플러그인을 실행 파일 옆에 복사해서 바로 실행 가능하게 함
option(STOCKINFO_DEPLOY_QT "Run windeployqt after build (Windows)" ON)
if(WIN32 AND STOCKINFO_DEPLOY_QT)
    get_target_property(_qmake Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin "${_qmake}" DIRECTORY)
    find_program(WINDEPLOYQT windeployqt HINTS "${_qt_bin}" NO_DEFAULT_PATH)
    if(WINDEPLOYQT)
        add_custom_command(TARGET StockInfo POST_BUILD
            COMMAND "${WINDEPLOYQT}" --no-translations --no-system-d3d-compiler
                    --no-opengl-sw --no-compiler-runtime "$<TARGET_FILE:StockInfo>"
            COMMENT "windeployqt: Qt 런타임 복사"
            VERBATIM)
    endif()
endif()

include(GNUInstallDirs)
install(TARGETS StockInfo
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
