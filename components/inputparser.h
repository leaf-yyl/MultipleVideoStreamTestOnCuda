#ifndef INPUTPARSER_H
#define INPUTPARSER_H

extern "C"{
#include "libavcodec/avcodec.h"
}

#include <QThread>

#include "utils/sharedlist.h"
#include "utils/sharedbufferpool.h"
#include "utils/sharedreusableavpacket.h"

class InputParser : public QThread
{
    Q_OBJECT
public:
    explicit InputParser(QObject *parent = 0);

    virtual void setInput(QString path) = 0;
    void setBufferPool(SharedBufferPool<SharedReusableAvPacket *> *pkt_pool);

    enum enReturnCode {
        InputParserSuccess = 0x01,
        InpuParserUserCancel,
        InputParserError_InvalidVideoStreamOrFile,
        InputParserError_NoVideoStream,
        InputParserError
    };
    enReturnCode getReturnCode();
    void stop(bool wait = false);

signals:
    void signal_setDecoder(AVCodecID, int, void *);
    void signal_packetReady(void *, void *);
    void signal_parserDone(InputParser *);

public slots:

protected:
    volatile bool m_stop;       /* volatile is sufficient */
    enReturnCode  m_ret;
    SharedList<InputParser *> *m_parser_list;
    SharedBufferPool<SharedReusableAvPacket *> *m_pkt_pool;

    int  m_packet_num;
    void logPacketNum();
};

#endif // INPUTPARSER_H
