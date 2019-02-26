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
#include "utils/sharedlist.h"
#include "utils/sharedbufferpool.h"
#include "utils/sharedreusableavpacket.h"

class Controller : public QObject
{
    Q_OBJECT
public:
    explicit Controller(QObject *parent = 0);
    ~Controller();

    void addInputFile(QString filepath);
    void closeInputStream();

    static const int TEST_NUM = 8;
    static const int TEST_PACKET_CAPACITY = 32;
    static const int TEST_FRAME_CAPACITY = 256;     /* about 1G for 1920x1080 video streams */
    static const int TEST_IMAGE_CAPACITY = 256;     /* about 1G like above */

signals:
    void signal_update(int id);

public slots:
    void slot_update(int id);
    void slot_setGpuImagePool(SharedBufferPool<ScGPUImage *> *image_pool, int id);

    void slot_parserDone(InputParser *parser);
    void slot_decoderDone(VideoDecoder *decoder);

private:
    InputParser *m_parser;

    QThread      ma_decoder_thread[TEST_NUM];
    VideoDecoder *ma_decoder[TEST_NUM];

    QThread      m_dispatcher_thread;
    Dispatcher   m_dispatcher;

    SharedBufferPool<AVFrame *> m_frame_pool;
    SharedBufferPool<SharedReusableAvPacket *> m_pkt_pool;
    SharedBufferPool<ScGPUImage *> m_image_pool;

    /* all parsers that is running */
    SharedList<InputParser *> ml_parser;

    /* TODO:a simple thread pool, need to move controller and opengl
     * operations to threads instead of in UI thread */
    QList<QThread *> ml_decoderthreads;
    QThread *findFreeThread();
};

#endif // CONTROLLER_H
