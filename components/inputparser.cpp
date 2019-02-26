#include "inputparser.h"

InputParser::InputParser(QObject *parent) : QThread(parent)
{
    m_stop = false;
    m_packet_num = 0;
    m_ret = InputParserSuccess;
    qRegisterMetaType<AVCodecID>("AVCodecID");
}

void InputParser::setBufferPool(SharedBufferPool<SharedReusableAvPacket *> *pkt_pool)
{
    m_pkt_pool = pkt_pool;
}

InputParser::enReturnCode InputParser::getReturnCode()
{
    return m_ret;
}

void InputParser::stop(bool wait)
{
    m_stop = true;
    if (wait) {
        while (isRunning()) {
            msleep(50);
        }
    }
}

void InputParser::logPacketNum()
{
    qDebug("Total %d packets has been parsed in this parser!", m_packet_num);
}
