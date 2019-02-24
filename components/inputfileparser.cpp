

extern "C" {
#include "libavformat/avformat.h"
}

#include <QDebug>

#include "inputfileparser.h"

InputFileParser::InputFileParser(QObject *parent) : InputParser(parent)
{
    /* TODO: register these in a global codec initialize function */
    av_register_all();
    avcodec_register_all();
}

void InputFileParser::setInput(QString path)
{
    m_filepath = path;
}

void InputFileParser::run()
{
    AVFormatContext *fc = NULL;
    if (avformat_open_input(&fc, m_filepath.toStdString().c_str(), NULL, NULL)) {
        goto ERR_RETURN;
    }

    if (avformat_find_stream_info(fc, NULL) < 0) {
        avformat_close_input(&fc);
        goto ERR_RETURN;
    }

    int video_stream_index = -1;
    for (unsigned int i = 0; i < fc->nb_streams; i++) {
        if (AVMEDIA_TYPE_VIDEO == fc->streams[i]->codec->codec_type) {
            video_stream_index = i;
            break;
        }
    }

    if (video_stream_index < 0) {
        avformat_close_input(&fc);
        goto ERR_RETURN;
    }

    AVCodecContext *ctx = fc->streams[video_stream_index]->codec;
    emit signal_setDecoder(ctx->codec_id, ctx->extradata_size, ctx->extradata);

    AVPacket *pkt = av_packet_alloc();
    while (av_read_frame(fc, pkt) >= 0) {
        if (pkt->stream_index == video_stream_index) {
            SharedReusableAvPacket *sp = m_pkt_pool->getFreeBuffer();
            sp->resize(pkt->size);
            av_packet_copy_props(sp->getAvPacket(), pkt);
            memcpy(sp->getAvPacket()->data, pkt->data, pkt->size);
            sp->getAvPacket()->size = pkt->size;

            m_packet_num++;

//            qDebug("Send %d th packet to decoder!", m_packet_num);
            emit signal_packetReady(sp, m_pkt_pool);
        }
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);

    /* notify decoder to fetch frames */
    emit signal_parserDone();

    avformat_close_input(&fc);
    m_ret = InputParserSuccess;
    return;

ERR_RETURN:
    m_ret = InputParserError;
    return;
}
