
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
}

#include <QDebug>

#include "videodecoder.h"

VideoDecoder::VideoDecoder(QObject *parent) : QObject(parent)
{
    m_ctx    = nullptr;
    m_iFrameDecoded = 0;
    m_frame = av_frame_alloc();
}

VideoDecoder::~VideoDecoder()
{
    if (nullptr != m_ctx) {
        if (m_ctx->extradata_size != 0) {
            free( m_ctx->extradata );
        }
        avcodec_free_context(&m_ctx);
    }

    av_frame_free(&m_frame);
}

void VideoDecoder::setFlag(unsigned int flag)
{
    m_flag = flag;
}

void VideoDecoder::setFrameBufferPool(SharedBufferPool<AVFrame *> *frame_pool)
{
    m_frame_pool = frame_pool;
}

void VideoDecoder::slot_setDecoderId(AVCodecID id, int size, void *extra_data)
{
    AVCodec *codec = avcodec_find_decoder(id);
    if (nullptr == codec) {
        return;
    }

    m_ctx = avcodec_alloc_context3(codec);
    if (nullptr == m_ctx) {
        return;
    }

    m_ctx->thread_count = 1;
    if (codec->capabilities & AV_CODEC_CAP_TRUNCATED) {
        m_ctx->flags |= AV_CODEC_FLAG_TRUNCATED;
    }
    if (0 != size) {
        m_ctx->extradata = (uint8_t*)malloc(size);
        memcpy(m_ctx->extradata, extra_data, size);
        m_ctx->extradata_size = size;
    }

    if (avcodec_open2(m_ctx, codec, nullptr) < 0) {
        avcodec_free_context(&m_ctx);
        return ;
    }

    qDebug("Video decoder has been created in thread %d!", m_flag);
}

void VideoDecoder::slot_packetReady(void *var1, void *var2)
{
    int ret = 0;
    int got_picture = 0;

//    qDebug("Recieve packet in thread %d!", m_flag);

    SharedReusableAvPacket *pkt = (SharedReusableAvPacket *)var1;
    SharedBufferPool<SharedReusableAvPacket *> *pkt_stream = (SharedBufferPool<SharedReusableAvPacket *> *) var2;
    if (nullptr != m_ctx) {

        AVFrame *frame = m_frame_pool->getFreeBuffer();
//        qDebug("Get free frame in thread %d!", m_flag);

        if (0 == m_iFrameDecoded) {
            ret = avcodec_decode_video2(m_ctx, m_frame, &got_picture, pkt->getAvPacket());
            if (got_picture && (ret >= 0)) {
                m_iFrameDecoded++;
                if (nullptr == frame->data[0]) {
                    av_image_alloc(frame->data, frame->linesize, m_frame->width, m_frame->height, m_frame->format, 64);
                }
                av_frame_copy(frame, m_frame);
                av_frame_copy_props(frame, m_frame);
                frame->format = m_frame->format;
                frame->width = m_frame->width;
                frame->height = m_frame->height;
                emitFrameReadySignal(frame, m_frame_pool, m_flag);
            } else {
                m_frame_pool->returnFreeBuffer(frame);
                char err[64] = {0};
                qDebug( av_make_error_string(err, 64, ret) );
            }
        } else {
            if (nullptr == frame->data[0]) {
                av_image_alloc(frame->data, frame->linesize, m_frame->width, m_frame->height, m_frame->format, 64);
            }

            ret = avcodec_decode_video2(m_ctx, frame, &got_picture, pkt->getAvPacket());
            if (got_picture && (ret >= 0)) {
                m_iFrameDecoded++;
                emitFrameReadySignal(frame, m_frame_pool, m_flag);
            } else {
                qDebug("Return free frame in thread %d!", m_flag);
                m_frame_pool->returnFreeBuffer(frame);
            }
        }
    }

    pkt->lock();
    pkt->addCurFlag(m_flag);
    if (pkt->isFree()) {
        pkt->resetCurFlag();
        pkt_stream->returnFreeBuffer(pkt);
    }
    pkt->unlock();

//    qDebug("Genrerate packet done in tHread %d!", m_flag);
}

void VideoDecoder::slot_packetDone()
{
    int ret = 0;
    int got_picture = 1;
    AVPacket *pkt = av_packet_alloc();

    if (nullptr != m_ctx) {

        while(got_picture) {
            AVFrame *frame = m_frame_pool->getFreeBuffer();
            if (0 == m_iFrameDecoded) {
                ret = avcodec_decode_video2(m_ctx, m_frame, &got_picture, pkt);
                if (got_picture && (ret >= 0)) {
                    m_iFrameDecoded++;
                    if (nullptr == frame->data[0]) {
                        av_image_alloc(frame->data, frame->linesize, m_frame->width, m_frame->height, m_frame->format, 64);
                    }
                    av_frame_copy(frame, m_frame);
                    emitFrameReadySignal(frame, m_frame_pool, m_flag);
                } else {
                    m_frame_pool->returnFreeBuffer(frame);
                }
            } else {
                if (nullptr == frame->data[0]) {
                    av_image_alloc(frame->data, frame->linesize, m_frame->width, m_frame->height, m_frame->format, 64);
                }

                ret = avcodec_decode_video2(m_ctx, frame, &got_picture, pkt);
                if (got_picture && (ret >= 0)) {
                    m_iFrameDecoded++;
                    emitFrameReadySignal(frame, m_frame_pool, m_flag);
                } else {
                    m_frame_pool->returnFreeBuffer(frame);
                }
            }
        }
    }

    av_packet_free(&pkt);
    qDebug("Video decoder %d has decoded %d frames!", m_flag, m_iFrameDecoded);
}


void VideoDecoder::emitFrameReadySignal(AVFrame *frame, SharedBufferPool<AVFrame *> *frame_pool, int sid)
{
    if (AV_PIX_FMT_YUV420P != frame->format) {
        qDebug("Unexpected pixel format!");
    }

    emit signal_FrameReady2(frame, frame_pool, sid);
}
