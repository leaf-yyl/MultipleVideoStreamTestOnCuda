
#include "sharedreusableavpacket.h"

SharedReusableAvPacket::SharedReusableAvPacket(QObject *parent)
    : SharedReusableData(parent)
{
    m_capacity = 0;
    m_avpacket = av_packet_alloc();
    av_init_packet(m_avpacket);
}

SharedReusableAvPacket::~SharedReusableAvPacket()
{
    if (nullptr != m_avpacket) {
        av_packet_free(&m_avpacket);
    }
}

void SharedReusableAvPacket::resize(unsigned long size)
{
    if (m_capacity < size) {
        if (nullptr != m_avpacket->data) {
            free(m_avpacket->data);
        }

        m_capacity = size;
        m_avpacket->data = malloc(size);
    }
}

AVPacket* SharedReusableAvPacket::getAvPacket()
{
    return m_avpacket;
}
