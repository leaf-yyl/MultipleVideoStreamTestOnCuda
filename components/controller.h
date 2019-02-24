#ifndef CONTROLLER_H
#define CONTROLLER_H

extern "C" {
#include "libavcodec/avcodec.h"
}

#include <QObject>

#include "dispatcher.h"
#include "inputparser.h"
#include "videodecoder.h"
#include "utils/scimage.h"
#include "utils/sharedbufferpool.h"
#include "utils/sharedreusableavpacket.h"

class Controller : public QObject
{
    Q_OBJECT
public:
    explicit Controller(QObject *parent = 0);
    ~Controller();

    void addInputFile(QString filepath);

    static const int TEST_NUM = 8;
    static const int TEST_PACKET_CAPACITY = 1;
    static const int TEST_FRAME_CAPACITY = 16;     /* about 1G for 1920x1080 video streams */
    static const int TEST_IMAGE_CAPACITY = 256;     /* about 1G like above */

signals:
    void signal_update(int id);

public slots:
    void slot_update(int id);
    void slot_setGpuImagePool(SharedBufferPool<ScGPUImage *> *image_pool, int id);

private:
    InputParser *m_parser;

    QThread      ma_decoder_thread[TEST_NUM];
    VideoDecoder ma_decoder[TEST_NUM];

    QThread      m_dispatcher_thread;
    Dispatcher   m_dispatcher;

    SharedBufferPool<AVFrame *> m_frame_pool;
    SharedBufferPool<SharedReusableAvPacket *> m_pkt_pool;
    SharedBufferPool<ScGPUImage *> m_image_pool;
};

#endif // CONTROLLER_H
