#ifndef DISPATCHER_H
#define DISPATCHER_H

extern "C" {
#include "libavcodec/avcodec.h"
}

#include <QObject>

#include "utils/scimage.h"
#include "utils/sharedbufferpool.h"

class Dispatcher : public QObject
{
    Q_OBJECT
public:
    explicit Dispatcher(QObject *parent = 0);
    ~Dispatcher();

    void setGpuImageBufferPool(SharedBufferPool<ScGPUImage *> *image_pool);
    void setGpuImageBufferPool(SharedBufferPool<ScGPUImage *> *image_pool, int i);

signals:
    void signal_dispatchResult(int );

public slots:
    void slot_frameReady2(void *frame, void *frame_stream, int sid);
    void slot_frameReady(AVFrame *frame, SharedBufferPool<AVFrame *> *frame_stream, int sid);

private:
    int m_iFrameRecieved;
    SharedBufferPool<ScGPUImage *> *m_image_pool;
    SharedBufferPool<ScGPUImage *> *m_image_pools[9];
    ScGPUImage *m_gpu_image;
    ScGPUImage *m_gpu_imageRGBA;

    void renderToOpenGlTexure(ScGPUImage *image, int wid);

    int getWid(int sid);
    int savePicture(ScGPUImage *image, const char *path);
    int savePicture(AVFrame *cu_frame, const char *path);
};

#endif // DISPATCHER_H
