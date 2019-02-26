

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
    m_ret = InputParserSuccess;
}

InputFileParser::~InputFileParser()
{

}

void InputFileParser::setInput(QString path)
{
    m_filepath = path;
}

void InputFileParser::run()
{
    AVFormatContext *fc = NULL;
    if (avformat_open_input(&fc, m_filepath.toStdString().c_str(), NULL, NULL)) {
        m_ret = InputParserError_InvalidVideoStreamOrFile;
        emitSignalParserDone();
        return;
    }

    if (avformat_find_stream_info(fc, NULL) < 0) {
        avformat_close_input(&fc);
        m_ret = InputParserError_NoVideoStream;
        emitSignalParserDone();
        return;
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
        m_ret = InputParserError_NoVideoStream;
        emitSignalParserDone();
        return;
    }

    AVCodecContext *ctx = fc->streams[video_stream_index]->codec;
    emitSignalSetDecoder(ctx->codec_id, ctx->extradata_size, ctx->extradata);

    /* read packet */
    AVPacket *pkt = av_packet_alloc();
    while (av_read_frame(fc, pkt) >= 0) {
        if (pkt->stream_index == video_stream_index) {
            SharedReusableAvPacket *sp = m_pkt_pool->getFreeBuffer();
            sp->resize(pkt->size);
            av_packet_copy_props(sp->getAvPacket(), pkt);
            memcpy(sp->getAvPacket()->data, pkt->data, pkt->size);
            sp->getAvPacket()->size = pkt->size;

            emitSignalPacketReady(sp, m_pkt_pool);
        }
        av_packet_unref(pkt);

        if (m_stop) {
            m_ret = InpuParserUserCancel;
            break;
        }
    }

    /* parser is done and free resource */
    logPacketNum();
    av_packet_free(&pkt);
    avformat_close_input(&fc);
    emitSignalParserDone();
}

void InputFileParser::emitSignalSetDecoder(AVCodecID id, int data_size, void *extra_data)
{
    emit signal_setDecoder(id, data_size, extra_data);
}

void InputFileParser::emitSignalPacketReady(void *sp, void *pkt_pool)
{
    m_packet_num++;
    emit signal_packetReady(sp, pkt_pool);
}

void InputFileParser::emitSignalParserDone()
{
    emit signal_parserDone(this);
}
