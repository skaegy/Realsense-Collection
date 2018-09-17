#-------------------------------------------------
#
# Project created by QtCreator 2018-07-12T19:01:08
#
#-------------------------------------------------

QT       += core gui bluetooth
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
requires(qtConfig(listwidget))

TARGET = RealsenseCollectData
TEMPLATE = app
CONFIG += console
# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x000000
# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        rscollectdata.cpp \
        rscapturethread.cpp

HEADERS += \
        rscollectdata.h \
    rscapturethread.h

FORMS += \
        rscollectdata.ui

INCLUDEPATH += /usr/local/include \
                /usr/local/include/opencv \
                /usr/local/include/opencv2 \
                /usr/include/librealsense2 \   #laptop
                /usr/include/librealsense2/hpp #laptop
                #/usr/local/include/librealsense2 \   #pc
                #/usr/local/include/librealsense2/hpp #pc



LIBS += /usr/local/lib/libopencv_highgui.so \
        /usr/local/lib/libopencv_core.so    \
        /usr/local/lib/libopencv_imgcodecs.so    \
        /usr/local/lib/libopencv_imgproc.so \
        /usr/lib/x86_64-linux-gnu/librealsense2.so #laptop
        #/usr/local/lib/librealsense2.so #pc


