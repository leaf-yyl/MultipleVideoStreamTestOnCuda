
#include "QDebug"
#include "QThread"
#include "shdmempool.h"

static QString           SharedMemoryPool::m_key      = QString("ShdMemoryPool");
static SharedMemoryPool* SharedMemoryPool::m_shd_pool = new SharedMemoryPool();

SharedMemoryPool::SharedMemoryPool()
{
    m_capacity  = 0;

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
    m_owner    = owner;
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

int SharedMemoryPool::getFreeSharedMemId(int size)
{
    int id = -1;
    char *data = nullptr;
    while (id < 0) {
        m_shd_free->lock();
        data = (char*)m_shd_free->data();
        for (int i = 0; i < m_capacity; i++)
        {
            if(Status_Free == data[i]) {
                data[i]     = Status_Inuse;
                id = i;
                break;
            }
        }
        m_shd_free->unlock();

        /* Here just wait 10ms, maybe process detection is needed */
        if (id < 0) {
            QThread::msleep(10);
        }
    }

    QSharedMemory *shd = ml_shd_mem.at(id);
    if (shd->size() < size) {
        shd->detach();

        if(!shd->create(size)) {
            /* This shd memory encounters an error, so we keep its status inuse instead of recover its status */
            qDebug("Failed to create shared memory-->size = %d, err = %d",
                   size, shd->error());
            return -1;
        }
    }

    return id;
}

void SharedMemoryPool::writeData(int id, void *data, int size)
{
    QSharedMemory *shd = ml_shd_mem.at(id);

    if (!m_owner) {
        shd->attach();
    }

    Q_ASSERT(shd->size() >= size);
    memcpy(shd->data(), data, size);

    if (!m_owner) {
        shd->detach();
    }
}

void SharedMemoryPool::returnReadySharedMem(int id)
{
    if (!m_owner) {
        m_shd_ready->attach();
    }

    m_shd_ready->lock();
    ((char*)m_shd_ready->data())[id] = Status_Ready;
    m_shd_ready->unlock();

    if (!m_owner) {
        m_shd_ready->detach();
    }
}


int SharedMemoryPool::getReadySharedMem()
{
    int id = -1;
    char *data = nullptr;
    m_shd_ready->attach();
    while (id < 0) {
        m_shd_ready->lock();
        data = (char*)m_shd_ready->data();
        for (int i = 0; i < m_capacity; i++)
        {
            if(Status_Ready == data[i]) {
                data[i]     = Status_Inuse;
                id = i;
                break;
            }
        }
        m_shd_ready->unlock();

        /* Here just wait 10ms, maybe process detection is needed */
        if (id < 0) {
            QThread::msleep(10);
        }
    }
    m_shd_ready->detach();

    return id;
}


void SharedMemoryPool::readData(int id, void *data, int size)
{
    QSharedMemory *shd = ml_shd_mem.at(id);

    if (!m_owner) {
        shd->attach();
    }

    Q_ASSERT(shd->size() >= size);
    memcpy(data, shd->data(), size);

    if (!m_owner) {
        shd->detach();
    }
}

void SharedMemoryPool::returnFreeSharedMem(int id)
{
    m_shd_free->attach();
    m_shd_free->lock();
    ((char*)m_shd_free->data())[id] = Status_Free;
    m_shd_free->unlock();
    m_shd_free->detach();
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
