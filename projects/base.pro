TEMPLATE = app

macx {
	CONFIG += app_bundle
	CONFIG += x86 ppc
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.4
}

CONFIG += debug_and_release

CONFIG(debug, debug|release) {
	OBJECTS_DIR = ../../obj/debug/
	MOC_DIR = ../../obj/debug/
	RCC_DIR = ../../obj/debug/
}
else {
	OBJECTS_DIR = ../../obj/release/
	MOC_DIR = ../../obj/release/
	RCC_DIR = ../../obj/release/
}

DESTDIR = ../../

RESOURCES += ../../src/timely.qrc

HEADERS += \
	../../src/TimelyMainWindow.h \
	../../src/TimeLine.h \

SOURCES += \
	../../src/TimelyMainWindow.cpp \
	../../src/TimeLine.cpp \
	../../src/timely.cpp \
