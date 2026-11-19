# StockInfo 모바일 앱 (Qt Quick). Android용, PC에서는 미리보기로 실행할 수 있다.

qt_add_executable(StockInfoMobile
    main.cpp
    AppController.h
    AppController.cpp
    QuoteListModel.h
    QuoteListModel.cpp
    ChartItem.h
    ChartItem.cpp
)

qt_add_qml_module(StockInfoMobile
    URI StockInfo
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/HomePage.qml
        qml/IndexCard.qml
        qml/StockList.qml
        qml/StockRow.qml
        qml/DetailPage.qml
        qml/StatItem.qml
        qml/SettingsPage.qml
        qml/AboutDialog.qml
        qml/IconButton.qml
)

# 앱 아이콘 (버전 정보 화면, 창 아이콘)
qt_add_resources(StockInfoMobile "mobile_icons"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/../resources"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/../resources/icons/icon_128.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/../resources/icons/icon_256.png"
)

target_link_libraries(StockInfoMobile PRIVATE
    stockinfo_core
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

# 버전 1.6.0 → 버전 코드 10600
math(EXPR STOCKINFO_VERSION_CODE
     "${PROJECT_VERSION_MAJOR} * 10000 + ${PROJECT_VERSION_MINOR} * 100 + ${PROJECT_VERSION_PATCH}")

if(ANDROID)
    set_target_properties(StockInfoMobile PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
        QT_ANDROID_TARGET_SDK_VERSION 34
        QT_ANDROID_MIN_SDK_VERSION 28
        QT_ANDROID_VERSION_NAME "${PROJECT_VERSION}"
        QT_ANDROID_VERSION_CODE ${STOCKINFO_VERSION_CODE}
    )

    # HTTPS(시세 조회)에 필요한 OpenSSL 3 (KDAB android_openssl 빌드, tools/install_android_sdk.ps1이 내려받음)
    set(STOCKINFO_ANDROID_OPENSSL "C:/Android/android_openssl" CACHE PATH "android_openssl 폴더 (ssl_3/<abi>/libssl_3.so)")
    set(_ssl_libs "")
    foreach(_abi arm64-v8a armeabi-v7a x86_64 x86)
        foreach(_lib libcrypto_3.so libssl_3.so)
            if(EXISTS "${STOCKINFO_ANDROID_OPENSSL}/ssl_3/${_abi}/${_lib}")
                list(APPEND _ssl_libs "${STOCKINFO_ANDROID_OPENSSL}/ssl_3/${_abi}/${_lib}")
            endif()
        endforeach()
    endforeach()
    if(_ssl_libs)
        # 여러 ABI 파일을 모두 넘기면 androiddeployqt가 ABI별로 골라 넣는다
        set_property(TARGET StockInfoMobile APPEND PROPERTY QT_ANDROID_EXTRA_LIBS ${_ssl_libs})
    else()
        message(WARNING "OpenSSL(android_openssl)을 찾지 못했습니다: ${STOCKINFO_ANDROID_OPENSSL}\n"
                        "HTTPS 시세 조회가 실패합니다. tools/install_android_sdk.ps1 을 실행하세요.")
    endif()
endif()

if(WIN32)
    set_target_properties(StockInfoMobile PROPERTIES WIN32_EXECUTABLE ON)
    # PC 미리보기: Qt 런타임 + QML 모듈 복사
    get_target_property(_qmake Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin "${_qmake}" DIRECTORY)
    find_program(WINDEPLOYQT_MOBILE windeployqt HINTS "${_qt_bin}" NO_DEFAULT_PATH)
    if(WINDEPLOYQT_MOBILE)
        add_custom_command(TARGET StockInfoMobile POST_BUILD
            COMMAND "${WINDEPLOYQT_MOBILE}" --no-translations --no-system-d3d-compiler --no-opengl-sw
                    --no-compiler-runtime --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/qml"
                    "$<TARGET_FILE:StockInfoMobile>"
            COMMENT "windeployqt: 모바일 미리보기용 Qt 런타임 복사"
            VERBATIM)
    endif()
endif()
