#-------------------------------------------------
# SmartServoGui
# Project created by QtCreator 2014-03-21T10:06:09
#-------------------------------------------------

TARGET      = SmartServoGui
TEMPLATE    = app
CONFIG     += qt
CONFIG     += c++11
QT         += core svg gui widgets

DESTDIR     = build/
RCC_DIR     = build/
UI_DIR      = build/
MOC_DIR     = build/
OBJECTS_DIR = build/

# QtSerialPort
QT_VERSION = $$[QT_VERSION]
QT_VERSION = $$split(QT_VERSION, ".")
QT_VER_MAJ = $$member(QT_VERSION, 0)
QT_VER_MIN = $$member(QT_VERSION, 1)
contains(QT_VER_MAJ, 5) | greaterThan(QT_VER_MIN, 6) {
   QT += serialport
   #DEFINES += FEATURE_QTSERIAL
   #message(Using QtSerialPort instead of OS specific implementation.)
}

# OS specifics build settings
unix {
    *-g++* {
        message("Using GCC compiler")
        QMAKE_CXXFLAGS += -std=c++11
        QMAKE_CXXFLAGS += -pthread
        QMAKE_CXXFLAGS += -Wno-unused-parameter -Wno-unused-variable
    }
    *clang* {
        message("Using LLVM compiler")
        QMAKE_CXXFLAGS += -stdlib=libc++ -Wno-unused-parameter -Wno-unused-variable
        LIBS += -stdlib=libc++
    }

    unix:!macx {
        message("Building on Linux/BSD plateform")
    }
    unix:macx {
        message("Building on macOS plateform")
        QMAKE_LFLAGS += -F /System/Library/Frameworks/
        LIBS += -framework IOKit -framework CoreFoundation

        # Force RPATH to look into the 'Frameworks' dir? Doesn't really seems to work...
        #QMAKE_RPATHDIR += @executable_path/../Frameworks
        #QMAKE_LFLAGS   += -Wl,-rpath,@executable_path/../Frameworks

        # Force Qt to use a particular SDK version
        #QMAKE_MAC_SDK = macosx10.11
        #QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9
    }
}
win32 {
    message("Building on Windows plateform")

    *-g++* {
        message("Using MinGW compiler")
        QMAKE_CXXFLAGS += -pthread -Wno-unused-parameter -Wno-unused-variable
    }
    *-msvc* {
        message("Using MSVC compiler")
    }
}

INCLUDEPATH = "$$PWD/../inc"

# SmartServoFramework sources
SOURCES    += ../src/*.cpp \
    ../src/dynamixel/Dynamixel.cpp \
    ../src/dynamixel/DynamixelController.cpp \
    ../src/dynamixel/DynamixelSimpleAPI.cpp \
    ../src/dynamixel/DynamixelTools.cpp \
    ../src/dynamixel/ServoAX.cpp \
    ../src/dynamixel/ServoDynamixel.cpp \
    ../src/dynamixel/ServoEX.cpp \
    ../src/dynamixel/ServoMX.cpp \
    ../src/dynamixel/ServoX.cpp \
    ../src/dynamixel/ServoXL.cpp \
    ../src/herkuleX/HerkuleX.cpp \
    ../src/herkuleX/HerkuleXController.cpp \
    ../src/herkuleX/HerkuleXSimpleAPI.cpp \
    ../src/herkuleX/HerkuleXTools.cpp \
    ../src/herkuleX/ServoDRS.cpp \
    ../src/herkuleX/ServoHerkuleX.cpp \
    ../src/mercury/Mercury.cpp \
    ../src/mercury/ServoV1.cpp \
    ../src/mercury/ServoMercury.cpp \
    ../src/mercury/MercuryTools.cpp
HEADERS    += ../inc/*.h \
    ../inc/Dynamixel/Dynamixel.h \
    ../inc/Dynamixel/DynamixelController.h \
    ../inc/Dynamixel/DynamixelSimpleAPI.h \
    ../inc/Dynamixel/DynamixelTools.h \
    ../inc/HerkuleX/HerkuleX.h \
    ../inc/HerkuleX/HerkuleXController.h \
    ../inc/HerkuleX/HerkuleXSimpleAPI.h \
    ../inc/HerkuleX/HerkuleXTools.h \
    ../inc/dynamixel/Dynamixel.h \
    ../inc/dynamixel/DynamixelController.h \
    ../inc/dynamixel/DynamixelSimpleAPI.h \
    ../inc/dynamixel/DynamixelTools.h \
    ../inc/dynamixel/ServoAX.h \
    ../inc/dynamixel/ServoDynamixel.h \
    ../inc/dynamixel/ServoEX.h \
    ../inc/dynamixel/ServoMX.h \
    ../inc/dynamixel/ServoX.h \
    ../inc/dynamixel/ServoXL.h \
    ../inc/herkuleX/HerkuleX.h \
    ../inc/herkuleX/HerkuleXController.h \
    ../inc/herkuleX/HerkuleXSimpleAPI.h \
    ../inc/herkuleX/HerkuleXTools.h \
    ../inc/herkuleX/ServoDRS.h \
    ../inc/herkuleX/ServoHerkuleX.h \
    ../inc/mercury/ControlTablesMercury.h \
    ../inc/mercury/Mercury.h \
    ../inc/mercury/MercuryController.h \
    ../inc/mercury/MercurySimpleAPI.h \
    ../inc/mercury/MercuryTools.h \
    ../inc/mercury/MercuryV1.h \
    ../inc/mercury/ServoMercury.h \
    ../inc/mercury/ServoV1.h

# GUI application sources
SOURCES    += src/main.cpp \
              src/mainwindow.cpp \
              src/tabSerial.cpp \
              src/advancescanner.cpp \
              src/widgetSerialScan.cpp \
              src/widgetSerialError.cpp \
              src/widgetRegisterTable.cpp \
              src/qabout.cpp \
              src/settings.cpp

HEADERS    += src/mainwindow.h \
              src/tabSerial.h \
              src/widgetSerialScan.h \
              src/widgetSerialError.h \
              src/widgetRegisterTable.h \
              src/advancescanner.h \
              src/qabout.h \
              src/settings.h

FORMS      += ui/mainwindow.ui \
              ui/advancescanner.ui \
              ui/qabout.ui \
              ui/settings.ui \
              ui/tabSerial.ui \
              ui/widgetSerialScan.ui \
              ui/widgetSerialError.ui \
              ui/widgetRegisterTable.ui

RESOURCES  += resources/resources.qrc

ICON        = resources/app/icon.icns
RC_ICONS    = resources/app/icon.ico

# Use "lupdate SmartServoGui.pro" to update translation files
# Then "lrelease SmartServoGui.pro" to build translated files
TRANSLATIONS = resources/lang/es.ts resources/lang/fr.ts resources/lang/it.ts



# macOS deploy rules:
# (Or you can just use macdeployqt...)
unix:macx {
    FW_DIR = build/$${TARGET}.app/Contents/Frameworks
    QT_DIR = /usr/local/lib/

    # Copy libraries into the package
    #QMAKE_POST_LINK += (mkdir -p $${FW_DIR})
    #QMAKE_POST_LINK += && (cp ../build/libSmartServoFramework.dylib $${FW_DIR})
    #QMAKE_POST_LINK += && (if [ ! -d $${FW_DIR}/QtCore.framework/ ]; then cp -R $${QT_DIR}/QtCore.framework $${FW_DIR}; fi)
    #QMAKE_POST_LINK += && (if [ ! -d $${FW_DIR}/QtSvg.framework/ ]; then cp -R $${QT_DIR}/QtSvg.framework $${FW_DIR}; fi)
    #QMAKE_POST_LINK += && (if [ ! -d $${FW_DIR}/QtGui.framework/ ]; then cp -R $${QT_DIR}/QtGui.framework $${FW_DIR}; fi)
    #QMAKE_POST_LINK += && (if [ ! -d $${FW_DIR}/QtWidgets.framework/ ]; then cp -R $${QT_DIR}/QtWidgets.framework $${FW_DIR}; fi)
    #QMAKE_POST_LINK += && (if [ ! -d $${FW_DIR}/QtSerialPort.framework/ ]; then cp -R $${QT_DIR}/QtSerialPort.framework $${FW_DIR}; fi)

    # Use bundled libraries (rewrite rpaths)
    APP = build/$${TARGET}.app/Contents/MacOS/$${TARGET}
    #QMAKE_POST_LINK += && (install_name_tool -change $${PWD}/build/libSmartServoFramework.dylib @executable_path/../Frameworks/libSmartServoFramework.dylib $${APP})
    #QMAKE_POST_LINK += && (install_name_tool -change @rpath/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore $${APP})
    #QMAKE_POST_LINK += && (install_name_tool -change @rpath/QtSvg.framework/Versions/5/QtSvg @executable_path/../Frameworks/QtSvg.framework/Versions/5/QtSvg $${APP})
    #QMAKE_POST_LINK += && (install_name_tool -change @rpath/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui $${APP})
    #QMAKE_POST_LINK += && (install_name_tool -change @rpath/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets $${APP})
    #QMAKE_POST_LINK += && (install_name_tool -change @rpath/QtSerialPort.framework/Versions/5/QtSerialPort @executable_path/../Frameworks/QtSerialPort.framework/Versions/5/QtSerialPort $${APP})
}
