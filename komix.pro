DEPENDPATH += . src src/widget
INCLUDEPATH += . src/widget src
MOC_DIR = moc
DESTDIR = bin
CONFIG += debug_and_release

# Input
HEADERS += src/utility.hpp \
           src/widget/imagearea.hpp \
           src/widget/mainwindow.hpp \
           src/widget/scaleimage.hpp
SOURCES += src/main.cpp \
           src/utility.cpp \
           src/widget/imagearea.cpp \
           src/widget/mainwindow.cpp \
           src/widget/scaleimage.cpp
RESOURCES = komix.qrc
QT       += xml svg

CONFIG( debug, debug|release  ) {
	OBJECTS_DIR = obj/debug
	TARGET = komix_d
} else {
	OBJECTS_DIR = obj/release
	DEFINES += QT_NO_DEBUG_OUTPUT
	TARGET = komix
}

unix {
	TEMPLATE = app
	QMAKE_POST_LINK=strip $(TARGET)
}

win32 {
	TEMPLATE = vcapp
}