HOME     = ../..
include( $${HOME}/sfd-common.pri )

QT += core
QT -= gui

TARGET = cfgfortran
CONFIG += c++11
CONFIG += console
CONFIG -= app_bundle
INCLUDEPATH  = ../../include


#INCLUDEPATH = /home/dmitry/energom/sur191/include
TEMPLATE = app


SOURCES += cfgfortran.cpp

LIBS += -lrt


#QMAKE_CXXFLAGS += -g -O0 -fprofile-arcs -ftest-coverage

HEADERS += \
    ../../include/sfdfortran.h
