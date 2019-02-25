
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
}

#include <QDebug>

#include "videodecoder.h"

VideoDecoder::VideoDecoder()
{
    m_iFrameDecoded = 0;
    m_ctx   = nullptr;
    m_frame = nullptr;
}

VideoDecoder::~VideoDecoder()
{
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

    /* set essential infos for decoder context */
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

    m_frame = av_frame_alloc();
    qDebug("Video decoder has been created in thread %d!", m_flag);
}

void VideoDecoder::slot_packetReady(void *var1, void *var2)
{
    int ret = 0;
    int got_picture = 0;

    SharedReusableAvPacket *pkt = (SharedReusableAvPacket *)var1;
    SharedBufferPool<SharedReusableAvPacket *> *pkt_stream = (SharedBufferPool<SharedReusableAvPacket *> *) var2;
    if (nullptr != m_ctx) {

        /* generate the first decoded frame */
        AVFrame *frame = m_frame_pool->getFreeBuffer();
        if (0 == m_iFrameDecoded) {
            ret = avcodec_decode_video2(m_ctx, m_frame, &got_picture, pkt->getAvPacket());
            if (got_picture && (ret >= 0)) {
                if (nullptr == frame->data[0]) {
                    av_image_alloc(frame->data, frame->linesize, m_frame->width, m_frame->height, m_frame->format, 64);
                }
                av_frame_copy(frame, m_frame);
                av_frame_copy_props(frame, m_frame);
                frame->format = m_frame->format;
                frame->width  = m_frame->width;
                frame->height = m_frame->height;
                emitFrameReadySignal(frame, m_frame_pool, m_flag);
            } else {
                m_frame_pool->returnFreeBuffer(frame);
            }
        } else {
            if (nullptr == frame->data[0]) {
                av_image_alloc(frame->data, frame->linesize, m_frame->width, m_frame->height, m_frame->format, 64);
            }

            ret = avcodec_decode_video2(m_ctx, frame, &got_picture, pkt->getAvPacket());
            if (got_picture && (ret >= 0)) {
                emitFrameReadySignal(frame, m_frame_pool, m_flag);
            } else {
                m_frame_pool->returnFreeBuffer(frame);
            }
        }
    } else {
        qDebug("Decodec context is not established!");
    }

    /* THe code below should be arranged more logically */
    pkt->lock();
    pkt->addCurFlag(m_flag);
    if (pkt->isFree()) {
        pkt->resetCurFlag();
        pkt_stream->returnFreeBuffer(pkt);
    }
    pkt->unlock();
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
                    if (nullptr == frame->data[0]) {
                        av_image_alloc(frame->data, frame->linesize, m_frame->width, m_frame->height, m_frame->format, 64);
                    }
                    av_frame_copy(frame, m_frame);
                    av_frame_copy_props(frame, m_frame);
                    frame->format = m_frame->format;
                    frame->width  = m_frame->width;
                    frame->height = m_frame->height;
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
                    emitFrameReadySignal(frame, m_frame_pool, m_flag);
                } else {
                    m_frame_pool->returnFreeBuffer(frame);
                }
            }
        }
    }

    av_packet_free(&pkt);
    qDebug("Video decoder %d has decoded %d frames!", m_flag, m_iFrameDecoded);

    if (nullptr != m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }

    if (nullptr != m_ctx) {
        avcodec_free_context(&m_ctx);
        m_ctx = nullptr;
    }

    emitSignalDecoderDone();
}

void VideoDecoder::emitFrameReadySignal(AVFrame *frame, SharedBufferPool<AVFrame *> *frame_pool, int sid)
{
    if (AV_PIX_FMT_YUV420P != frame->format) {
        qDebug("Unexpected pixel format!");
    }

    m_iFrameDecoded++;
    emit signal_FrameReady(frame, frame_pool, sid);
}

void VideoDecoder::emitSignalDecoderDone()
{
    emit signal_decoderDone(this);
}
