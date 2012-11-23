CONFIG += main_exe
SOURCES = main.cpp \
    TestWtsIntegration.cpp \
    WtsShell.cpp \
    TestRunner.cpp
wts.components = startpage utils customwidgets
wts.top.relative = ../..
include( $$wts.top.relative/common.pri )

RESOURCES += \
    WatchThatSound.qrc

HEADERS += \
    TestWtsIntegration.hpp \
    WtsShell.hpp \
    TestRunner.hpp







