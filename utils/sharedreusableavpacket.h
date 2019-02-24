#ifndef SHAREDREUSABLEAVPACKET_H
#define SHAREDREUSABLEAVPACKET_H

extern "C" {
#include "libavcodec/avcodec.h"
}

#include "sharedreusabledata.h"

class SharedReusableAvPacket : public SharedReusableData
{
public:
    explicit SharedReusableAvPacket(QObject *parent = nullptr);
    ~SharedReusableAvPacket();

    void resize(unsigned long size);
    AVPacket *getAvPacket();

private:
    unsigned int m_capacity;
    AVPacket *m_avpacket;
};

#endif // SHAREDREUSABLEAVPACKET_H
