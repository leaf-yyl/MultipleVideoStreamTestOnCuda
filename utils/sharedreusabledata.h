
#ifndef SHAREDREUSABLEDATA_H
#define SHAREDREUSABLEDATA_H

#include <QMutex>
#include <QObject>

class SharedReusableData : public QObject
{
    Q_OBJECT
public:
    explicit SharedReusableData(QObject *parent = 0);
    ~SharedReusableData();

    void initFlags(unsigned int flags);
    void addCurFlag(unsigned int flag);
    void resetCurFlag();

    bool isFree();
    bool isBusy();

    void lock();
    void unlock();

signals:
    void signal_ready(void *);

public slots:

protected:
    QMutex *m_mutex;

private:
    int m_flags;
    int m_cur_flag;
};

#endif // SHAREDREUSABLEDATA_H
