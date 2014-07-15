######################################################################
# Automatically generated by qmake (2.01a) Thu Jun 19 23:59:29 2014
######################################################################

TEMPLATE = app
TARGET =
DEPENDPATH += ../../object-files
INCLUDEPATH += /usr/include/qwt-qt4
LIBS += -l qwt-qt4 -L../../../lib/mosquitto-1.1.2/lib ../../../lib/mosquitto-1.1.2/lib/cpp/libmosquittopp.so.1 ../../../lib/mosquitto-1.1.2/lib/libmosquitto.so.1

# Input
HEADERS += Channel.h connectionuart.h EmbSysVisu.h \
    ../../Middleware/Subscriber.h \
    ../../Middleware/Message.h \
    ../../Middleware/MQTTv3.h \
    ../../Middleware/Publisher.h \
    tools.h \
    datalogger.h
FORMS += ConnectionUART.ui EmbSysVisu.ui
SOURCES += Channel.cpp connectionuart.cpp main.cpp \
    EmbSysVisu.cpp \
    ../../Middleware/Message.cpp \
    ../../Middleware/MQTTv3.cpp \
    ../../Middleware/Publisher.cpp \
    ../../Middleware/Subscriber.cpp \
    datalogger.cpp
