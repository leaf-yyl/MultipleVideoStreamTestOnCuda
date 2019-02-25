#-------------------------------------------------
#
# Project created by QtCreator 2019-02-22T13:08:03
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG(debug, debug|release){
    DEFINES += _DEBUG
}

TARGET = DemoTest
TEMPLATE = app

QMAKE_CXXFLAGS += -fpermissive

INCLUDEPATH += $$PWD \
    /opt/cuda10.0/include \
    /opt/ffmpeg/include

LIBS += \
#cuda
    -L/opt/cuda10.0/lib64 \
    -lcuda -lcudart \
#ffmpeg
    -L/opt/ffmpeg/lib \
    -lavformat -lavcodec -lavutil -lswscale -lswresample

SOURCES += main.cpp\
        mainwindow.cpp \
    utils/sharedreusabledata.cpp \
    utils/sharedreusableavpacket.cpp \
    components/inputparser.cpp \
    components/inputfileparser.cpp \
    components/videodecoder.cpp \
    components/dispatcher.cpp \
    components/controller.cpp \
    scglwidget.cpp \
    utils/scimage.cpp \
    glcudawidget.cpp

HEADERS  += mainwindow.h \
    utils/sharedreusabledata.h \
    utils/sharedlist.h \
    utils/sharedbufferpool.h \
    utils/sharedreusableavpacket.h \
    components/inputparser.h \
    components/inputfileparser.h \
    components/videodecoder.h \
    components/dispatcher.h \
    components/controller.h \
    scglwidget.h \
    utils/scimage.h \
    glcudawidget.h \
    $$files(cuda/*.cuh)

FORMS    += mainwindow.ui


#cuda toolkit path
CUDA_SDK=/opt/cuda10.0

CUDA_SOURCES = $$files(cuda/*.cu)

CUDA_INCLUDEPATH = $$join(INCLUDEPATH,' -I','-I', '')

SYSTEM_TYPE = 64            # '32' or '64', depending on your system
CUDA_ARCH   = sm_52           # Type of CUDA architecture, for example 'compute_10', 'compute_11', 'sm_10'
NVCC_OPTIONS = --use_fast_math

CUDA_OBJECTS_DIR = ./

nvcc.input = CUDA_SOURCES
nvcc.output = $$CUDA_OBJECTS_DIR/${QMAKE_FILE_BASE}_cuda.o

# Configuration of the nvcc compiler
CONFIG(debug, debug|release) {
    # Debug mode
    nvcc.commands = $$CUDA_SDK/bin/nvcc $$NVCC_OPTIONS --machine $$SYSTEM_TYPE -arch=$$CUDA_ARCH \
            $$CUDA_INCLUDEPATH -D_DEBUG -Xcompiler -fpic -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
} else {
    # Release mode
    nvcc.commands = $$CUDA_SDK/bin/nvcc $$NVCC_OPTIONS --machine $$SYSTEM_TYPE -arch=$$CUDA_ARCH \
            $$CUDA_INCLUDEPATH -O3 -Xcompiler -fpic -c -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
}

QMAKE_EXTRA_COMPILERS += nvcc
