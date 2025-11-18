QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    commandhelper.cpp \
    common.cpp \
    logparser.cpp \
    main.cpp \
    mainwindow.cpp \
    order.cpp \
    qlitethread.cpp

HEADERS += \
    commandhelper.h \
    common.h \
    logparser.h \
    mainwindow.h \
    order.h \
    qlitethread.h \
    uilogappender.h

# add by itas109
# 1. headers
INCLUDEPATH += "$$PWD/CSerialPort/include"
include($$PWD/log4qt/Include/log4qt.pri)

# 2. sources
SOURCES += $$PWD/CSerialPort/src/SerialPortBase.cpp
SOURCES += $$PWD/CSerialPort/src/SerialPort.cpp
SOURCES += $$PWD/CSerialPort/src/SerialPortInfoBase.cpp
SOURCES += $$PWD/CSerialPort/src/SerialPortInfo.cpp

win32 {
    SOURCES += $$PWD/CSerialPort/src/SerialPortWinBase.cpp
    SOURCES += $$PWD/CSerialPort/src/SerialPortInfoWinBase.cpp
}

unix {
    SOURCES += $$PWD/CSerialPort/src/SerialPortUnixBase.cpp
    SOURCES += $$PWD/CSerialPort/src/SerialPortInfoUnixBase.cpp
}

# 3. add system libs
win32-msvc*:LIBS += advapi32.lib
win32-msvc*:LIBS += setupapi.lib
win32-g++:LIBS += libsetupapi

# 4. define UNICODE
DEFINES += _UNICODE
# end by itas109

FORMS += \
    mainwindow.ui

TRANSLATIONS += \
    LEDControl2_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

# 软件图标
RC_ICONS = $$PWD/resource/logo.ico

# 设置输出目录
DESTDIR = $$PWD/../build_LED
contains(QT_ARCH, x86_64) {
    # x64
    DESTDIR = $$DESTDIR/x64
} else {
    # x86
    DESTDIR = $$DESTDIR/x86
}

DESTDIR = $$DESTDIR/qt$$QT_VERSION/
message(DESTDIR = $$DESTDIR)

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc

DISTFILES += \
    config/setting.json \
    log4qt/Include/log4qt.pri \
    log4qt/Include/log4qt/.gitignore \
    log4qt/Include/log4qt/CMakeLists.txt \
    log4qt/Include/log4qt/log4qt.pri \
    log4qt/Include/log4qt/object_script.log4qt.Debug \
    log4qt/Include/log4qt/object_script.log4qt.Release \
    log4qt/lib/liblog4qt.a \
    log4qt/lib/log4qt.dll

SUBDIRS += \
    log4qt/Include/log4qt/log4qt.pro
