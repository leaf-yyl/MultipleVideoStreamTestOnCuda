#include "inputparser.h"

InputParser::InputParser(QObject *parent) : QThread(parent)
{
    m_packet_num = 0;
    qRegisterMetaType<AVCodecID>("AVCodecID");
}

void InputParser::setBufferPool(SharedBufferPool<SharedReusableAvPacket *> *pkt_pool)
{
    m_pkt_pool = pkt_pool;
}
