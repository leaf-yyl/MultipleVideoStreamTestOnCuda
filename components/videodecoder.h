
#ifndef VIDEODECODER_H
#define VIDEODECODER_H

extern "C" {
#include "libavformat/avformat.h"
}

#include <QObject>

#include "utils/sharedbufferpool.h"
#include "utils/sharedreusableavpacket.h"

class VideoDecoder : public QObject
{
    Q_OBJECT
public:
    explicit VideoDecoder(QObject *parent = 0);
    ~VideoDecoder();

    void setFlag(unsigned int flag);
    void setFrameBufferPool(SharedBufferPool<AVFrame *> *frame_pool);

signals:
    void signal_FrameReady(AVFrame *, SharedBufferPool<AVFrame *> *, int);
    void signal_FrameReady2(void *, void *, int);

public slots:
    void slot_setDecoderId(AVCodecID id, int size, void *extra_data);
    void slot_packetReady(void *pkt, void *pkt_stream);
    void slot_packetDone();

private:
    SharedBufferPool<AVFrame *> *m_frame_pool;

    int m_flag;
    int m_width;
    int m_height;
    int m_iFrameDecoded;
    AVFrame *m_frame;
    AVCodecContext *m_ctx;

    void emitFrameReadySignal(AVFrame *frame, SharedBufferPool<AVFrame *> *frame_pool, int sid);
};

#endif // VIDEODECODER_H
