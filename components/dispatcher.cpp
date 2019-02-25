
#include <QTime>
#include <QDebug>
#include <QImage>
#include <QThread>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}

#include "dispatcher.h"

#include "cuda/algorithm.cuh"

Dispatcher::Dispatcher(QObject *parent) : QObject(parent)
{
    m_iFrameRecieved = 0;
    m_gpu_image      = new ScGPUImage();
    m_gpu_imageRGBA  = new ScGPUImage();
    m_gpu_imageRGBA_swap  = new ScGPUImage();
}

Dispatcher::~Dispatcher()
{
    delete m_gpu_image;
    delete m_gpu_imageRGBA;
    delete m_gpu_imageRGBA_swap;
}

void Dispatcher::setGpuImageBufferPool(SharedBufferPool<ScGPUImage *> *image_pool)
{
    m_image_pool = image_pool;
}

void Dispatcher::setGpuImageBufferPool(SharedBufferPool<ScGPUImage *> *image_pool, int i)
{
    m_image_pools[i] = image_pool;
}

void Dispatcher::slot_frameReady2(void *frame, void *frame_stream, int sid)
{
    slot_frameReady((AVFrame *)frame, (SharedBufferPool<AVFrame *> *)frame_stream, sid);
}

void Dispatcher::slot_frameReady(AVFrame *frame, SharedBufferPool<AVFrame *> *frame_stream, int sid)
{
    m_iFrameRecieved++;

    static unsigned int time_elapsed = 0;
    QTime time;
    time.start();

    ScGPUImage::enScPixFormat pix_fmt = ScGPUImage::getScPixFormat(frame->format);
    if (ScGPUImage::ScPixFormat_Unknown == pix_fmt) {
        frame_stream->returnFreeBuffer(frame);
        return;
    }

    m_gpu_image->copyFromAVFrame(frame);

    /* TODO: gpu generation and send to opengl */
    int wid = getWid(sid);

    m_gpu_imageRGBA->resize(m_gpu_image->m_pixel_width, m_gpu_image->m_pixel_height, ScGPUImage::ScPixFormat_RGBA);
    m_gpu_imageRGBA_swap->resize(m_gpu_image->m_pixel_width, m_gpu_image->m_pixel_height, ScGPUImage::ScPixFormat_RGBA);
    ColorSpaceConvertion(m_gpu_image, m_gpu_imageRGBA);
    if (wid == 1) {
        HorizontalReversalRGBA(m_gpu_imageRGBA, m_gpu_imageRGBA_swap);
        renderToOpenGlTexure(m_gpu_imageRGBA_swap, wid);
    } else if (wid == 2){
        VerticalReversalRGBA(m_gpu_imageRGBA, m_gpu_imageRGBA_swap);
        renderToOpenGlTexure(m_gpu_imageRGBA_swap, wid);
    } else {
        renderToOpenGlTexure(m_gpu_imageRGBA, wid);
    }

    emit signal_dispatchResult(wid);

    time_elapsed += time.elapsed();
    if (m_iFrameRecieved % 200 == 0) {
        qDebug("Recieve total %d frames in %d milli seconds, currunt frame comes from thread %d!",
               m_iFrameRecieved, time_elapsed, sid);
    }

    frame_stream->returnFreeBuffer(frame);
}

void Dispatcher::renderToOpenGlTexure(ScGPUImage *image, int wid)
{
    ScGPUImage *gpuimage = m_image_pools[wid]->getFreeBuffer();


//    cudaResourceDesc viewCudaArrayResourceDesc;
//    viewCudaArrayResourceDesc.resType = cudaResourceTypeArray;
//    viewCudaArrayResourceDesc.res.array.array = image->m_array_addr;
//    err = cudaCreateSurfaceObject(&gpuimage->m_cuda_surface, &viewCudaArrayResourceDesc);
    RenderRGBAImageToSurface(image, gpuimage->m_cuda_surface);
//    err = cudaDestroySurfaceObject(gpuimage->m_cuda_surface);

    m_image_pools[wid]->returnReadyBuffer(gpuimage);
}

int Dispatcher::getWid(int sid)
{
    int wid = 0;
    while (sid > 1) {
        sid >>= 1;
        wid++;
    }

    return wid;
}

int Dispatcher::savePicture(ScGPUImage *image, const char *path)
{
    uchar *data = malloc(image->m_pixel_width * image->m_pixel_height * 4);
    cudaMemcpy2D(data, image->m_pixel_width * 4, image->m_dev_addr, image->m_data_width,
                 image->m_pixel_width * 4, image->m_pixel_height, cudaMemcpyDeviceToHost);
    QImage *t = new QImage(data, image->m_pixel_width, image->m_pixel_height, QImage::Format_RGBA8888);
    t->save(path);
    free(data);
    delete t;
}

int Dispatcher::savePicture(AVFrame *cu_frame, const char *path)
{
    int ret;
    AVFormatContext *oc = NULL;
    ret = avformat_alloc_output_context2(&oc, NULL, NULL, path);

    AVCodec *ocodec = avcodec_find_encoder(oc->oformat->video_codec);
    AVStream *ost = avformat_new_stream(oc, ocodec);

    ost->codec->width = cu_frame->width;
    ost->codec->height = cu_frame->height;
    ost->time_base.num = 1;
    ost->time_base.den = 30;
    ost->codec->thread_count = 0;
    ost->codec->time_base = ost->time_base;
    ost->codec->pix_fmt = ost->codec->codec->pix_fmts[0];
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        ost->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    ret = avcodec_open2(ost->codec, ocodec, NULL);
    ret = avio_open(&oc->pb, path, AVIO_FLAG_WRITE);
    ret = avformat_write_header(oc, NULL);

    int got_pkt;
    AVPacket pkt;
    memset(&pkt, 0, sizeof(AVPacket));
    ret = avcodec_encode_video2(ost->codec, &pkt, cu_frame, &got_pkt);
    if (got_pkt) {
        av_write_frame(oc, &pkt);
    } else {
        ret = avcodec_encode_video2(ost->codec, &pkt, NULL, &got_pkt);
        if (got_pkt) {
            av_write_frame(oc, &pkt);
        }
    }
    av_packet_unref(&pkt);

    ret = av_write_trailer(oc);

    avcodec_close(ost->codec);
    avio_closep(&oc->pb);
    avformat_free_context(oc);
}
