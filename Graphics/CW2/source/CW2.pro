TEMPLATE = app
TARGET = CW2Build
INCLUDEPATH += . /opt/local/include

QT += widgets opengl gui 

LIBS += -lGLU -lglut

# Input
HEADERS += CW2Window.h CW2Widget.h CW2PPMImg.h CW2MatWindow.h
SOURCES += CW2Window.cpp CW2Main.cpp CW2Widget.cpp CW2PPMImg.cpp CW2MatWindow.cpp