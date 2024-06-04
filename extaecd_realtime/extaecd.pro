TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        gpiodev.cpp \
        main.cpp
LIBS += -levent -lgpiod

HEADERS += \
    gpiodev.h
