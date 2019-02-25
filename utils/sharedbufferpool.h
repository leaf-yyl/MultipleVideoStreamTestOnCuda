#ifndef SHAREDBUFFERPOOL_H
#define SHAREDBUFFERPOOL_H

#include <QDebug>
#include <QList>
#include <QMutex>
#include <QSemaphore>
#include <QWaitCondition>

template <class T>
class SharedBufferPool
{
public:
    SharedBufferPool() {
        m_capacity = 11;
        m_mutex      = new QMutex();
        m_cond_free  = new QWaitCondition();
        m_cond_ready = new QWaitCondition();
    }

    ~SharedBufferPool() {
        delete m_mutex;
        delete m_cond_free;
        delete m_cond_ready;
    }

    int getFreeBufferCount() {
        m_mutex->lock();
        int tmp = ml_free_buffer.size();
        m_mutex->unlock();

        return tmp;
    }

    int getReadyBufferCount() {
        m_mutex->lock();
        int tmp = ml_ready_buffer.size();
        m_mutex->unlock();

        return tmp;
    }

    int getTotalBufferCount() {
        m_mutex->lock();
        int tmp = ml_free_buffer.size() + ml_ready_buffer.size();
        m_mutex->unlock();

        return tmp;
    }

    T getFreeBuffer() {
        m_mutex->lock();

        while (ml_free_buffer.isEmpty()) {
            m_cond_free->wait(m_mutex);
        }

        T t = ml_free_buffer.takeFirst();
        m_mutex->unlock();
        return t;
    }

    void returnFreeBuffer(T t) {
        m_mutex->lock();

        ml_free_buffer.append(t);
        m_mutex->unlock();
        m_cond_free->wakeOne();
    }

    T getReadyBuffer() {
        m_mutex->lock();
        while (ml_ready_buffer.isEmpty()) {
            m_cond_ready->wait(m_mutex);
        }

        T t = ml_ready_buffer.takeFirst();
        m_mutex->unlock();

        return t;
    }

    void returnReadyBuffer(T t) {
        m_mutex->lock();

        if (ml_ready_buffer.isEmpty()) {
            m_cond_ready->wakeOne();
        }
        ml_ready_buffer.append(t);
        m_mutex->unlock();
    }

protected:
    QMutex         *m_mutex;
    QSemaphore     *m_semaphore;
    QWaitCondition *m_cond_free;
    QWaitCondition *m_cond_ready;
    QList<T>       ml_free_buffer;
    QList<T>       ml_ready_buffer;

    int m_capacity;
};


#endif // SHAREDBUFFERPOOL_H
