
#include "QDebug"
#include "QThread"
#include "sharedmempool.h"

static QString           SharedMemoryPool::m_key      = QString("ShdMemoryPool");
static SharedMemoryPool* SharedMemoryPool::m_shd_pool = new SharedMemoryPool();

SharedMemoryPool::SharedMemoryPool()
{
    m_capacity  = 0;
    m_cur_index = -1;

    m_shd_free  = new QSharedMemory(m_key.append(SharedMemoryPool::m_key_free));
    m_shd_ready = new QSharedMemory(m_key.append(SharedMemoryPool::m_key_ready));
}

SharedMemoryPool::~SharedMemoryPool()
{
    delete m_shd_free;
    delete m_shd_ready;

    for (int i = 0; i < m_capacity; i++)
    {
        QSharedMemory *shd = ml_shd_mem.takeFirst();
        delete shd;
    }
}

bool SharedMemoryPool::init(int capacity, bool owner)
{
    m_capacity = capacity;
    for (int i = 0; i < capacity; i++)
    {
        QSharedMemory *shd = new QSharedMemory(m_key.append(QString::number(i)));
        ml_shd_mem.append(shd);
    }

    /* create free/ready shared memory for memory reuse */
    if (owner) {
        detachAll();

        if (!m_shd_free->create(capacity)) {
            qDebug("Failed to create shared memory-->%d!", m_shd_free->error());
            return false;
        }
        m_shd_free->lock();
        char *data = (char*)m_shd_free->data();
        for (int i = 0; i < capacity; i++) {
            data[i] = Status_Free;
        }
        m_shd_free->unlock();

        /* ready shared mem manager */
        if (!m_shd_ready->create(capacity)) {
            qDebug("Failed to create shared memory-->%d!", m_shd_ready->error());
            return false;
        }
        m_shd_ready->lock();
        data = (char*)m_shd_ready->data();
        for (int i = 0; i < capacity; i++) {
            data[i] = -1;
        }
        m_shd_ready->unlock();
    }

    return true;
}

QSharedMemory* SharedMemoryPool::getFreeSharedMem(int size)
{
    char *data = nullptr;
    while (m_cur_index < 0) {
        m_shd_free->lock();
        data = (char*)m_shd_free->data();
        for (int i = 0; i < m_capacity; i++)
        {
            if(Status_Free == data[i]) {
                data[i]     = Status_Inuse;
                m_cur_index = i;
                break;
            }
        }
        m_shd_free->unlock();

        /* Here just wait 10ms, maybe process detection is needed */
        if (m_cur_index < 0) {
            QThread::msleep(10);
        }
    }

    QSharedMemory *shd = ml_shd_mem.at(m_cur_index);
    if (shd->size() < size) {
        shd->detach();

        if(!shd->create(size)) {
            qDebug("Failed to create shared memory-->size = %d, err = %d",
                   size, shd->error());
            return nullptr;
        }
    }

    return shd;
}

void SharedMemoryPool::setData(QSharedMemory *shd, void *data, int size)
{
    memcpy(shd->data(), data, qMin(shd->size(), size));
}

void SharedMemoryPool::returnReadySharedMem(QSharedMemory *shd)
{
    Q_ASSERT(shd == ml_shd_mem.at(m_cur_index));

    m_shd_ready->lock();
    ((char*)m_shd_ready->data())[m_cur_index] = Status_Ready;
    m_cur_index = -1;
    m_shd_ready->unlock();
}

QSharedMemory* SharedMemoryPool::getReadySharedMem()
{
    char *data = nullptr;
    while (m_cur_index < 0) {
        m_shd_ready->lock();
        data = (char*)m_shd_ready->data();
        for (int i = 0; i < m_capacity; i++)
        {
            if(Status_Ready == data[i]) {
                data[i]     = Status_Inuse;
                m_cur_index = i;
                break;
            }
        }
        m_shd_ready->unlock();

        /* Here just wait 10ms, maybe process detection is needed */
        if (m_cur_index < 0) {
            QThread::msleep(10);
        }
    }

    return ml_shd_mem.at(m_cur_index);
}

void SharedMemoryPool::readData(QSharedMemory *shd, void *data, int size)
{
    memcpy(data, shd->data(), qMin(shd->size(), size));
}

void SharedMemoryPool::returnFreeSharedMem(QSharedMemory *shd)
{
    Q_ASSERT(shd == ml_shd_mem.at(m_cur_index));

    m_shd_free->lock();
    ((char*)m_shd_free->data())[m_cur_index] = Status_Free;
    m_cur_index = -1;
    m_shd_free->unlock();
}

void SharedMemoryPool::detachAll()
{
    /* Here may need do something else */
    detachSharedMemory(m_shd_free);
    detachSharedMemory(m_shd_ready);
    for (int i = 0; i < m_capacity; i++)
    {
        detachSharedMemory(ml_shd_mem.at(i));
    }
}

void SharedMemoryPool::detachSharedMemory(QSharedMemory *shd)
{
    if (shd->isAttached()) {
        shd->detach();
    }

    if (shd->attach()) {
        shd->detach();
    }
}
