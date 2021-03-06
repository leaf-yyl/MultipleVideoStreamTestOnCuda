
#ifndef SHAREDMEMPOOL_H
#define SHAREDMEMPOOL_H

#include <QString>
#include "QSharedMemory"

class SharedMemoryPool
{
public:
    static SharedMemoryPool* getInstance() {
            return m_shd_pool;
        }
    ~SharedMemoryPool();

    bool init(int capacity, bool owner = true);

    int getFreeSharedMemId(int size);
    void writeData(int id, void *data, int size);
    void returnReadySharedMem(int id);

    int getReadySharedMem();
    void readData(int id, void *data, int size);
    void returnFreeSharedMem(int id);

private:
    /* Singleton class */
    SharedMemoryPool();
    SharedMemoryPool(const SharedMemoryPool&) {}
    SharedMemoryPool& operator=(const SharedMemoryPool&) {}

    static QString m_key;
    static SharedMemoryPool *m_shd_pool;
    static void newInstance() {
        m_shd_pool = new SharedMemoryPool();
    }

    enum {
        Status_Free   = 0x01,
        Status_Ready  = 0x02,
        Status_Inuse  = 0x04
    };

    const char *m_key_free  = "free";
    const char *m_key_ready = "ready";
    QSharedMemory *m_shd_free;
    QSharedMemory *m_shd_ready;

    int  m_capacity;
    bool m_owner;
    QList<QSharedMemory *> ml_shd_mem;

    void detachAll();
    void detachSharedMemory(QSharedMemory *shd);

};

#endif // SHAREDMEMPOOL_H
