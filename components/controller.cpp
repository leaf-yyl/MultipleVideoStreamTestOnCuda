
#include "controller.h"
#include "inputfileparser.h"

Controller::Controller(QObject *parent) : QObject(parent)
{
    for (int i= 0; i < TEST_PACKET_CAPACITY; i++)
    {
        SharedReusableAvPacket *sp = new SharedReusableAvPacket();
        sp->initFlags(0xff);
        m_pkt_pool.returnFreeBuffer(sp);
    }

    for (int i= 0; i < TEST_FRAME_CAPACITY; i++)
    {
        AVFrame *frame = av_frame_alloc();
        m_frame_pool.returnFreeBuffer(frame);
    }

    m_parser = nullptr;

    m_dispatcher.moveToThread(&m_dispatcher_thread);
    connect(&m_dispatcher, SIGNAL(signal_dispatchResult(int)),
            this, SLOT(slot_update(int)));

    m_dispatcher_thread.start();
}

Controller::~Controller()
{
    for (int i= 0; i < TEST_PACKET_CAPACITY; i++)
    {
        SharedReusableAvPacket *sp = m_pkt_pool.getFreeBuffer();
        delete sp;
    }

    for (int i= 0; i < TEST_FRAME_CAPACITY; i++)
    {
        AVFrame *frame = m_frame_pool.getFreeBuffer();
        av_frame_free(&frame);
    }

    if (nullptr != m_parser) {
        delete m_parser;
    }
}

void Controller::addInputFile(QString filepath)
{
    closeInputStream();

    m_parser = new InputFileParser(this);
    m_parser->setInput(filepath);
    m_parser->setBufferPool(&m_pkt_pool);
    ml_parser.push(m_parser);

    for (int i = 0; i < TEST_NUM; i++)
    {
        ma_decoder[i] = new VideoDecoder();
        ma_decoder[i]->moveToThread(ma_decoder_thread + i);
        ma_decoder[i]->setFrameBufferPool(&m_frame_pool);
        ma_decoder[i]->setFlag(1 << i);

        connect(m_parser, SIGNAL(signal_setDecoder(AVCodecID, int, void *)),
                ma_decoder[i], SLOT(slot_setDecoderId(AVCodecID, int, void *)));
        connect(m_parser, SIGNAL(signal_packetReady(void*,void*)),
                ma_decoder[i], SLOT(slot_packetReady(void*,void*)));
        connect(m_parser, SIGNAL(finished()), ma_decoder[i], SLOT(slot_packetDone()));
        connect(m_parser, SIGNAL(signal_parserDone(InputParser*)), this, SLOT(slot_parserDone(InputParser*)));
        connect(ma_decoder[i], SIGNAL(signal_decoderDone(VideoDecoder*)), this, SLOT(slot_decoderDone(VideoDecoder*)));

        connect(ma_decoder[i], SIGNAL(signal_FrameReady(AVFrame *, SharedBufferPool<AVFrame *> *, int)),
                &m_dispatcher, SLOT(slot_frameReady(AVFrame*,SharedBufferPool<AVFrame*>*,int)));
        connect(ma_decoder[i], SIGNAL(signal_FrameReady2(void *, void *, int)),
                &m_dispatcher, SLOT(slot_frameReady2(void*, void *,int)));

        ma_decoder_thread[i].start();
    }

    m_parser->start();
}

void Controller::closeInputStream()
{
    if (nullptr != m_parser) {
        m_parser->stopAndWaitForParserDone();
        m_parser = nullptr;
    }
}

void Controller::slot_update(int id)
{
    emit signal_update(id);
}

void Controller::slot_setGpuImagePool(SharedBufferPool<ScGPUImage *> *image_pool, int id)
{
    m_dispatcher.setGpuImageBufferPool(image_pool, id);
}

void Controller::slot_parserDone(InputParser *parser)
{
    /* recycle parser, but we delete it now */
    delete parser;
}

void Controller::slot_decoderDone(VideoDecoder *decoder)
{
    delete decoder;
}
