QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    commandhelper.cpp \
    common.cpp \
    main.cpp \
    mainwindow.cpp \
    order.cpp \
    qlitethread.cpp

HEADERS += \
    commandhelper.h \
    common.h \
    mainwindow.h \
    order.h \
    qlitethread.h

# add by itas109
# 1. headers
INCLUDEPATH += "$$PWD/CSerialPort/include"
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

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc

DISTFILES += \
    config/setting.json
