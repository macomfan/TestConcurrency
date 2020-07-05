#pragma once

#include <atomic>
#include <thread>
#include <memory>


namespace L {

    class SpinLock_atomic {
        std::atomic_flag flag;
    public:

        SpinLock_atomic() : flag(ATOMIC_FLAG_INIT) {

        }

        void lock() {
            while (flag.test_and_set(std::memory_order_acquire));
        }

        void unlock() {
            flag.clear(std::memory_order_release);
        }
    };

    class SpinLock_CAS {
        std::atomic<bool> flag = ATOMIC_VAR_INIT(false);
    public:
        SpinLock_CAS() = default;

        void lock() {
            bool expected = false;
            while (!flag.compare_exchange_strong(expected, true, std::memory_order_acquire))
                expected = false;
        }

        void unlock() {
            flag.store(false, std::memory_order_release);
        }
    };

    class SpinLock_exchange {
        std::atomic<bool> flag = ATOMIC_VAR_INIT(false);
    public:
        SpinLock_exchange() = default;

        void lock() {
            while (flag.exchange(true, std::memory_order_acquire));
        }

        void unlock() {
            flag.store(false, std::memory_order_release);
        }
    };

    class LinuxRWLock {
        pthread_rwlock_t flock = PTHREAD_RWLOCK_INITIALIZER;
    public:

        ~LinuxRWLock() {
            pthread_rwlock_destroy(&flock);
        }

        void rlock() {
            pthread_rwlock_rdlock(&flock);
        }

        void runlock() {
            pthread_rwlock_unlock(&flock);
        }

        void wlock() {
            pthread_rwlock_wrlock(&flock);
        }

        void wunlock() {
            pthread_rwlock_unlock(&flock);
        }
    };

    class AtomicRWLock {
#define WRITE_LOCK_STATUS -1
#define FREE_STATUS 0
    private:
        static const std::thread::id NULL_THEAD;
        const bool WRITE_FIRST;
        std::thread::id m_write_thread_id;
        std::atomic_int m_lockCount;
        std::atomic_uint m_writeWaitCount;
    public:

        AtomicRWLock(bool writeFirst = false) :
        WRITE_FIRST(writeFirst),
        m_write_thread_id(),
        m_lockCount(0),
        m_writeWaitCount(0) {

        }

        int rlock() {
            if (std::this_thread::get_id() != this->m_write_thread_id) {
                int count;
                if (WRITE_FIRST)
                    do {
                        while ((count = m_lockCount) == WRITE_LOCK_STATUS || m_writeWaitCount > 0);
                    } while (!m_lockCount.compare_exchange_weak(count, count + 1));
                else
                    do {
                        while ((count = m_lockCount) == WRITE_LOCK_STATUS);
                    } while (!m_lockCount.compare_exchange_weak(count, count + 1));
            }
            return m_lockCount;
        }

        int runlock() {
            if (std::this_thread::get_id() != this->m_write_thread_id)
                --m_lockCount;
            return m_lockCount;
        }

        int wlock() {
            if (std::this_thread::get_id() != this->m_write_thread_id) {
                ++m_writeWaitCount;
                for (int zero = FREE_STATUS; !this->m_lockCount.compare_exchange_weak(zero, WRITE_LOCK_STATUS); zero = FREE_STATUS);
                --m_writeWaitCount;
                m_write_thread_id = std::this_thread::get_id();
            }
            return m_lockCount;
        }

        int wunlock() {
            if (std::this_thread::get_id() != this->m_write_thread_id) {
                throw std::runtime_error("writeLock/Unlock mismatch");
            }
            assert(WRITE_LOCK_STATUS == m_lockCount);
            m_write_thread_id = NULL_THEAD;
            m_lockCount.store(FREE_STATUS);
            return m_lockCount;
        }

    };

    const std::thread::id AtomicRWLock::NULL_THEAD;
}
