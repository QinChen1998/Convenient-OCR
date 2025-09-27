# Convenient-OCR项目配置文件
QT       += core gui widgets

CONFIG += c++17

# 应用程序信息
TARGET = ConvenientOCRApplication
VERSION = 1.0.0

# 编译器配置
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# 源文件
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    ocrengine.cpp \
    tesseractocrengine.cpp \
    fileprocessor.cpp \
    screencapture.cpp \
    licensedialog.cpp

# 头文件
HEADERS += \
    mainwindow.h \
    ocrengine.h \
    tesseractocrengine.h \
    fileprocessor.h \
    screencapture.h \
    licensedialog.h

# UI文件
FORMS += \
    mainwindow.ui \
    licensedialog.ui

# 资源文件（如果有图标等）
# RESOURCES += resources.qrc

# Windows特定配置
win32 {
    # Windows应用程序图标
    # RC_ICONS = icon.ico

    # 版本信息
    VERSION_PE_HEADER = $$VERSION
    QMAKE_TARGET_COMPANY = "Convenient-OCR Application"
    QMAKE_TARGET_PRODUCT = "Convenient-OCR Intelligent Text Recognition Tool"
    QMAKE_TARGET_DESCRIPTION = "Convenient-OCR智能文字识别工具"
    QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2024"
}

# 部署配置
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# 调试和发布配置
CONFIG(debug, debug|release) {
    DESTDIR = $$PWD/build/debug
    OBJECTS_DIR = $$PWD/build/debug/obj
    MOC_DIR = $$PWD/build/debug/moc
    RCC_DIR = $$PWD/build/debug/rcc
    UI_DIR = $$PWD/build/debug/ui
}

CONFIG(release, debug|release) {
    DESTDIR = $$PWD/build/release
    OBJECTS_DIR = $$PWD/build/release/obj
    MOC_DIR = $$PWD/build/release/moc
    RCC_DIR = $$PWD/build/release/rcc
    UI_DIR = $$PWD/build/release/ui

    # 发布版本优化
    DEFINES += QT_NO_DEBUG_OUTPUT
}
