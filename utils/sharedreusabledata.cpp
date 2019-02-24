#include "sharedreusabledata.h"

SharedReusableData::SharedReusableData(QObject *parent) : QObject(parent)
{
    m_mutex = new QMutex();
}

SharedReusableData::~SharedReusableData()
{
    delete m_mutex;
}

void SharedReusableData::initFlags(unsigned int flags){
    m_flags    = flags;
    m_cur_flag = 0;
}

void SharedReusableData::addCurFlag(unsigned int flag) {
    m_cur_flag |= flag;
}

void SharedReusableData::resetCurFlag() {
    m_cur_flag = 0;
}

bool SharedReusableData::isFree()
{
    return (m_cur_flag == m_flags);
}

bool SharedReusableData::isBusy()
{
    return !isFree();
}

void SharedReusableData::lock()
{
    m_mutex->lock();
}

void SharedReusableData::unlock()
{
    m_mutex->unlock();
}
