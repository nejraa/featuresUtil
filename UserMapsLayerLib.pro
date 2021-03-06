#-------------------------------------------------
#
# Project created by QtCreator 2018-09-27T08:33:01
#
#-------------------------------------------------

QT       += core
QT       += qml quick quickwidgets gui

TARGET = UserMapsLayerLib
TEMPLATE = lib

DEFINES += USERMAPSLAYERLIB_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
CONFIG -= debug_and_release debug_and_release_target
# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    mapshaderprogram.cpp \
    triangulate.cpp \
    usermapslayer.cpp \
    usermapsrenderer.cpp \
    usermapsvertexdata.cpp

HEADERS += \
    mapshaderprogram.h \
    triangulate.h \
    usermapslayer.h \
    usermapslayerlib_global.h \
    usermapsrenderer.h \
    usermapsvertexdata.h \
    userpointpositiontype.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

if(android){LIBS += -L$$OUT_PWD/../UtilitiesLib/ -lUtilitiesLib_$$ANDROID_TARGET_ARCH}
else{LIBS += -L$$OUT_PWD/../UtilitiesLib/ -lUtilitiesLib}

INCLUDEPATH += $$PWD/../UtilitiesLib
DEPENDPATH += $$PWD/../UtilitiesLib

if(android){LIBS += -L$$OUT_PWD/../LayerLib/ -lLayerLib_$$ANDROID_TARGET_ARCH}
else{LIBS += -L$$OUT_PWD/../LayerLib/ -lLayerLib}

INCLUDEPATH += $$PWD/../LayerLib
DEPENDPATH += $$PWD/../LayerLib

if(android){LIBS += -L$$OUT_PWD/../UserMapsDataLib/ -lUserMapsDataLib_$$ANDROID_TARGET_ARCH}
else{LIBS += -L$$OUT_PWD/../UserMapsDataLib/ -lUserMapsDataLib}

INCLUDEPATH += $$PWD/../UserMapsDataLib
DEPENDPATH += $$PWD/../UserMapsDataLib

if(android){LIBS += -L$$OUT_PWD/../ShipDataLib/ -lShipDataLib_$$ANDROID_TARGET_ARCH}
else{LIBS += -L$$OUT_PWD/../ShipDataLib/ -lShipDataLib}

INCLUDEPATH += $$PWD/../ShipDataLib
DEPENDPATH += $$PWD/../ShipDataLib

if(android){LIBS += -L$$OUT_PWD/../OpenGLBaseLib/ -lOpenGLBaseLib_$$ANDROID_TARGET_ARCH}
else{LIBS += -L$$OUT_PWD/../OpenGLBaseLib/ -lOpenGLBaseLib}

INCLUDEPATH += $$PWD/../OpenGLBaseLib
DEPENDPATH += $$PWD/../OpenGLBaseLib

if(android){LIBS += -L$$OUT_PWD/../NavUtilsLib/ -lNavUtilsLib_$$ANDROID_TARGET_ARCH}
else{LIBS += -L$$OUT_PWD/../NavUtilsLib/ -lNavUtilsLib}

INCLUDEPATH += $$PWD/../NavUtilsLib
DEPENDPATH += $$PWD/../NavUtilsLib

if(android){LIBS += -L$$OUT_PWD/../LoggingLib/ -lLoggingLib_$$ANDROID_TARGET_ARCH}
else{LIBS += -L$$OUT_PWD/../LoggingLib/ -lLoggingLib}

INCLUDEPATH += $$PWD/../LoggingLib
DEPENDPATH += $$PWD/../LoggingLib

if(android){LIBS += -L$$OUT_PWD/../ColourManagerLib/ -lColourManagerLib_$$ANDROID_TARGET_ARCH}
else{LIBS += -L$$OUT_PWD/../ColourManagerLib/ -lColourManagerLib}

INCLUDEPATH += $$PWD/../ColourManagerLib
DEPENDPATH += $$PWD/../ColourManagerLib

RESOURCES += \
    usermapshader.qrc
