#ifndef INPUTPARSER_H
#define INPUTPARSER_H

extern "C"{
#include "libavcodec/avcodec.h"
}

#include <QThread>

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
        InputParserError
    };

signals:
    void signal_setDecoder(AVCodecID, int, void *);
    void signal_packetReady(void *, void *);
    void signal_parserDone();

public slots:

protected:
    int m_packet_num;
    enReturnCode m_ret;
    SharedBufferPool<SharedReusableAvPacket *> *m_pkt_pool;
};

#endif // INPUTPARSER_H
