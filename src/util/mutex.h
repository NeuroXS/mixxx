#pragma once

// Thread annotation aware variants of locks, read-write locks and scoped
// lockers. This allows us to use Clang thread safety analysis in Mixxx.
// See: http://clang.llvm.org/docs/ThreadSafetyAnalysis.html

#include <QMutex>
#include <QReadWriteLock>

#include "util/compatibility/qmutex.h"
#include "util/thread_annotations.h"

class CAPABILITY("mutex") MMutex {
  public:
    MMutex() = default;

    inline void lock() ACQUIRE() { m_mutex.lock(); }
    inline void unlock() RELEASE() { m_mutex.unlock(); }
    inline bool tryLock() TRY_ACQUIRE(true) {
        return m_mutex.tryLock();
    }

  private:
    QMutex m_mutex;
    friend class MMutexLocker;
};

class CAPABILITY("mutex") MReadWriteLock {
  public:
    MReadWriteLock(QReadWriteLock::RecursionMode mode = QReadWriteLock::NonRecursive)
            : m_lock(mode) {
    }

    void lockForRead() ACQUIRE_SHARED() { m_lock.lockForRead(); }
    bool tryLockForRead() TRY_ACQUIRE_SHARED(true) {
        return m_lock.tryLockForRead();
    }

    void lockForWrite() ACQUIRE() { m_lock.lockForWrite(); }
    bool tryLockForWrite() TRY_ACQUIRE(true) {
        return m_lock.tryLockForWrite();
    }

    inline void unlock() RELEASE() { m_lock.unlock(); }

  private:
    QReadWriteLock m_lock;
    friend class MWriteLocker;
    friend class MReadLocker;
};

class SCOPED_CAPABILITY MMutexLocker {
  public:
    MMutexLocker(MMutex* mu) ACQUIRE(mu) : m_locker(&mu->m_mutex) {}
    ~MMutexLocker() RELEASE() {}

    inline void unlock() RELEASE() { m_locker.unlock(); }

  private:
    QT_MUTEX_LOCKER m_locker;
};

class SCOPED_CAPABILITY MMutexLockerDebug {
  public:
    MMutexLockerDebug(MMutex* pMu, const QString& info = {})
            : m_pMutex(pMu) {
        if (!m_pMutex->tryLock()) {
            qDebug() << "Mutex wait" << info;
            m_pMutex->lock();
        }
        qDebug() << "Mutex locked" << info;
    }
    ~MMutexLockerDebug() {
        m_pMutex->unlock();
        qDebug() << "Mutex unlocked";
    }

  private:
    MMutex* m_pMutex;
};

class SCOPED_CAPABILITY MWriteLocker {
  public:
    MWriteLocker(MReadWriteLock* mu) ACQUIRE(mu) : m_locker(&mu->m_lock) {}
    ~MWriteLocker() RELEASE() {}

    inline void unlock() RELEASE() { m_locker.unlock(); }

  private:
    QWriteLocker m_locker;
};

class SCOPED_CAPABILITY MReadLocker {
  public:
    MReadLocker(MReadWriteLock* mu) ACQUIRE_SHARED(mu)
            : m_locker(&mu->m_lock) {}
    ~MReadLocker() RELEASE() {}

    inline void unlock() RELEASE() { m_locker.unlock(); }

  private:
    QReadLocker m_locker;
};
