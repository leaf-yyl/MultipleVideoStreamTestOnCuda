#ifndef SHAREDLIST_H
#define SHAREDLIST_H

#include <QList>
#include <QMutex>
#include <QWaitCondition>

template <class T>
class SharedList
{
public:
    SharedList(){ m_capacity = DEFAULT_CAPACTITY; }
    SharedList(int capacity){ m_capacity = capacity; }

    int size() {
        m_mutex.lock();
        int tmp = ml_data.size();
        m_mutex.unlock();

        return tmp;
    }

    T pop() {
        m_mutex.lock();
        if (ml_data.isEmpty()) {
            m_cond.wait(&m_mutex);
        }

        if (ml_data.size() >= m_capacity) {
            m_cond.wakeOne();
        }
        T t = ml_data.takeFirst();
        m_mutex.unlock();

        return t;
    }

    void push(T t) {
        m_mutex.lock();

        if (ml_data.size() >= m_capacity) {
            m_cond.wait(&m_mutex);
        }

        if (ml_data.isEmpty()) {
            m_cond.wakeOne();
        }
        ml_data.append(t);
        m_mutex.unlock();
    }

    /* for temporary use */
    void popSpecifiedElement(T t) {
        m_mutex.lock();
        if (ml_data.isEmpty()) {
            m_cond.wait(&m_mutex);
        }

        for (int i = 0; i < ml_data.size(); i++)
        {
            if (t == ml_data.at(i))
            {
                ml_data.takeAt(i);
                break;
            }
        }
        m_mutex.unlock();

        return ;
    }

protected:
    QMutex         m_mutex;
    QWaitCondition m_cond;
    QList<T>       ml_data;

    int m_capacity;
    static const int DEFAULT_CAPACTITY = 128;
};

#endif // SHAREDLIST_H
