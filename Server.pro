#INCLUDEPATH += C:/Packages/vcpkg/installed/x86-windows/include/boost
#LIBS += "-LC:\WORK\boost_1_59_0\lib64-msvc-12.0"

QT -= gui
QT += core network
QT += sql


CONFIG += c++17 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    httpserver.cpp \
    main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    httpserver.h \
