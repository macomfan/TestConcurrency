#pragma once

#include <atomic>
#include <thread>
#include <memory>
#include <condition_variable>

// Two pages discuss the spin lock and RWLock
// https://github.com/cyfdecyf/spinlock/
// http://locklessinc.com/articles/locks/

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

    class CVRWLock {
    public:

        void rlock() {
            if (write_cnt != 0) {
                std::unique_lock<std::mutex> ulk(counter_mutex);
                cond_r.wait(ulk, [&]() {
                    return write_cnt == 0; });
            }
            ++read_cnt;
        }

        void runlock() {
            std::unique_lock<std::mutex> ulk(counter_mutex);
            if (--read_cnt == 0 && write_cnt > 0) {
                cond_w.notify_one();
            }
        }

        void wlock() {
            std::unique_lock<std::mutex> ulk(counter_mutex);
            ++write_cnt;
            cond_w.wait(ulk, [&]() {
                return read_cnt == 0 && !inwriteflag;
            });
            inwriteflag = true;
        }

        void wunlock() {
            inwriteflag = false;
            if (--write_cnt == 0) {
                cond_r.notify_all();
            } else {
                cond_w.notify_one();
            }
        }

    private:
        std::atomic<uint32_t> read_cnt{0};
        std::atomic<uint32_t>write_cnt{0};
        std::atomic<bool> inwriteflag{false};
        std::mutex counter_mutex;
        std::condition_variable cond_w;
        std::condition_variable cond_r;
    };

    class NginxRWLock2 {
        std::atomic<uint64_t> lock{0};
        uint64_t WLOCK = (uint64_t) - 1;
        uint64_t ZERO = (uint64_t) 0;
    public:

        void rlock() {
            uint64_t reader;
            for (;;) {
                reader = lock;
                if (reader != WLOCK && lock.compare_exchange_weak(reader, reader + 1)) {
                    return;
                }
            }
        }

        void runlock() {
            uint64_t reader = lock;
            for (;;) {
                if (lock > ZERO && lock.compare_exchange_weak(reader, reader - 1)) {
                    return;
                }
                reader = lock;
            }
        }

        void wlock() {
            for (;;) {
                if (lock == ZERO && lock.compare_exchange_weak(ZERO, WLOCK)) {
                    return;
                }
                ZERO = 0;
            }
        }

        void wunlock() {
            if (lock == WLOCK) {
                lock.compare_exchange_weak(WLOCK, ZERO);
                return;
            }
        }
    };

#define NGX_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)
#define NGX_RWLOCK_SPIN   2048
#define NGX_RWLOCK_WLOCK  ((ngx_atomic_uint_t) -1)
#define ngx_cpu_pause()         __asm__ ("pause")
#define ngx_sched_yield()   sched_yield()
#define ngx_ncpu 4

    class NginxRWLock {
    public:
        typedef int64_t ngx_atomic_int_t;
        typedef uint64_t ngx_atomic_uint_t;
        typedef volatile ngx_atomic_uint_t ngx_atomic_t;
        typedef intptr_t ngx_int_t;
        typedef uintptr_t ngx_uint_t;
        typedef intptr_t ngx_flag_t;

        static ngx_atomic_uint_t
        ngx_atomic_cmp_set(ngx_atomic_t *lock, ngx_atomic_uint_t old,
                ngx_atomic_uint_t set) {
            u_char res;

            __asm__ volatile (

                    "    cmpxchgq  %3, %1;   "
                    "    sete      %0;       "

                    : "=a" (res) : "m" (*lock), "a" (old), "r" (set) : "cc", "memory");

            return res;
        }

        void rlock() {
            ngx_uint_t i, n;
            ngx_atomic_uint_t readers;

            for (;;) {
                readers = lock;

                if (readers != NGX_RWLOCK_WLOCK
                        && ngx_atomic_cmp_set(&lock, readers, readers + 1)) {
                    std::cout << "lock " << lock << std::endl;
                    return;
                }

                //                if (ngx_ncpu > 1) {
                //
                //                    for (n = 1; n < NGX_RWLOCK_SPIN; n <<= 1) {
                //
                //                        for (i = 0; i < n; i++) {
                //                            ngx_cpu_pause();
                //                        }
                //
                //                        readers = lock;
                //
                //                        if (readers != NGX_RWLOCK_WLOCK
                //                                && ngx_atomic_cmp_set(&lock, readers, readers + 1)) {
                //                            std::cout << "lock " << lock << std::endl;
                //                            return;
                //                        }
                //                    }
                //                }

                //ngx_sched_yield();
            }
        }

        void runlock() {
            ngx_atomic_uint_t readers;

            readers = lock;

            if (readers == NGX_RWLOCK_WLOCK) {
                (void) ngx_atomic_cmp_set(&lock, NGX_RWLOCK_WLOCK, 0);
                return;
            }

            for (;;) {
                if (lock == 0) {
                    std::cout << "error" << std::endl;
                }
                if (ngx_atomic_cmp_set(&lock, readers, readers - 1)) {
                    std::cout << "unlock " << lock << std::endl;
                    return;
                }

                readers = lock;
            }
        }

        void wlock() {
            ngx_uint_t i, n;

            for (;;) {

                if (lock == 0 && ngx_atomic_cmp_set(&lock, 0, NGX_RWLOCK_WLOCK)) {
                    return;
                }

                //                if (ngx_ncpu > 1) {
                //
                //                    for (n = 1; n < NGX_RWLOCK_SPIN; n <<= 1) {
                //
                //                        for (i = 0; i < n; i++) {
                //                            ngx_cpu_pause();
                //                        }
                //
                //                        if (lock == 0
                //                                && ngx_atomic_cmp_set(&lock, 0, NGX_RWLOCK_WLOCK)) {
                //                            return;
                //                        }
                //                    }
                //                }

                //ngx_sched_yield();
            }
        }

        void wunlock() {
            runlock();
        }

    private:
        ngx_atomic_t lock = 0;
    };

#define cmpxchg(P, O, N) __sync_val_compare_and_swap((P), (O), (N))

#define barrier() asm volatile("": : :"memory")
#define cpu_relax() asm volatile("pause\n": : :"memory")

    static inline void *xchg_64(void *ptr, void *x) {
        __asm__ __volatile__("xchgq %0,%1"
                : "=r" ((unsigned long long) x)
                : "m" (*(volatile long long *) ptr), "0" ((unsigned long long) x)
                : "memory");

        return x;
    }

    typedef struct mcs_lock_t mcs_lock_t;

    struct mcs_lock_t {
        mcs_lock_t *next;
        int spin;
    };
    typedef struct mcs_lock_t *mcs_lock;

    static inline void lock_mcs(mcs_lock *m, mcs_lock_t *me) {
        mcs_lock_t *tail;

        me->next = NULL;
        me->spin = 0;

        tail = (mcs_lock_t*) xchg_64(m, me);

        /* No one there? */
        if (!tail) return;

        /* Someone there, need to link in */
        tail->next = me;

        /* Make sure we do the above setting of next. */
        barrier();

        /* Spin on my spin variable */
        while (!me->spin) cpu_relax();

        return;
    }

    static inline void unlock_mcs(mcs_lock *m, mcs_lock_t *me) {
        /* No successor yet? */
        if (!me->next) {
            /* Try to atomically unlock */
            if (cmpxchg(m, me, NULL) == me) return;

            /* Wait for successor to appear */
            while (!me->next) cpu_relax();
        }

        /* Unlock next one */
        me->next->spin = 1;
    }

    static inline int trylock_mcs(mcs_lock *m, mcs_lock_t *me) {
        mcs_lock_t *tail;

        me->next = NULL;
        me->spin = 0;

        /* Try to lock */
        tail = cmpxchg(m, NULL, &me);

        /* No one was there - can quickly return */
        if (!tail) return 0;

        return 1; // Busy
    }

    
    // copied from https://github.com/cyfdecyf/spinlock/
    class MCSLock {
    public:

        void lock() {
            lock_mcs(&cnt_lock, &local_lock);
        }

        void unlock() {
            unlock_mcs(&cnt_lock, &local_lock);
        }

    private:
        mcs_lock cnt_lock = NULL;
        static thread_local mcs_lock_t local_lock;
    };

    thread_local mcs_lock_t MCSLock::local_lock;

    struct qnode {
        std::atomic<qnode *> next;
        std::atomic<bool> wait;
    };

    // copied from https://stackoverflow.com/questions/61944469/problems-with-mcs-lock-implementation
    class MCSLockAtomic {
        std::atomic<qnode *> tail;
        static thread_local qnode p;
    public:
        
        MCSLockAtomic() {
            tail.store(nullptr);
        }

        void lock() {
            p.next.store(nullptr);
            p.wait.store(true);

            qnode *prev = tail.exchange(&p, std::memory_order_acq_rel);

            if (prev) {
                prev->next.store(&p, std::memory_order_release);

                /* spin */
                while (p.wait.load(std::memory_order_acquire))
                    ;
            }
        }

        void unlock() {
            qnode *succ = p.next.load(std::memory_order_acquire);

            if (!succ) {
                auto expected = &p;
                if (tail.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel))
                    return;

                do {
                    succ = p.next.load(std::memory_order_acquire);
                } while (succ == nullptr);
            }

            succ->wait.store(false, std::memory_order_release);
        }
    };
    thread_local qnode MCSLockAtomic::p;
}
