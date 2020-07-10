#pragma once

#include <string>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <atomic>
#include <regex>
#include "concurrentqueue.h"

const std::string NULL_VALUE;

namespace Q {

    class Queue_Blocking {
        std::mutex m;
        std::condition_variable c;
        std::queue<std::string> q;

        bool close_ = false; //This is only for test.
    public:

        Queue_Blocking() {

        }

        void close() {
            close_ = true;
            c.notify_all();
        }

        void push(const std::string& msg) {
            std::lock_guard<std::mutex> lk(m);
            q.push(msg);
            c.notify_one();
        }

        std::string pop() {
            std::unique_lock<std::mutex> lk(m);
            c.wait(lk, [&] {
                return !q.empty() || close_;
            });
            if (close_) {
                return NULL_VALUE;
            }
            auto res = q.front();
            q.pop();
            return res;
        }
    };

    template <typename LOCK = std::mutex>
    class Queue_NonBlocking {
        LOCK m;
        std::queue<std::string> q;
    public:

        Queue_NonBlocking() {

        }

        void close() {
        }

        void push(const std::string& msg) {
            std::lock_guard<LOCK> lk(m);
            q.push(msg);
        }

        std::string pop() {
            std::lock_guard<LOCK> lk(m);
            if (!q.empty()) {
                auto res = q.front();
                q.pop();
                return res;
            }
            return NULL_VALUE;
        }
    };

    typedef struct node_t {
        std::string value;
        node_t *next;
    } NODE;

    
    template <typename LOCK = std::mutex>
    struct queue_t {
        NODE *head;
        NODE *tail;
        LOCK q_h_lock;
        LOCK q_t_lock;
    };
    //typedef struct queue_t Q;

    template <typename LOCK = std::mutex>
    class Queue_NonBlocking_TwoLock {
    public:

        void close() {
        }

        Queue_NonBlocking_TwoLock() {
            NODE* none = new NODE();
            none->next = nullptr;
            q.head = q.tail = none;
        }

        void push(const std::string& value) {
            NODE* node = new NODE;
            node->value = value;
            node->next = nullptr;
            std::lock_guard<LOCK> lk(q.q_t_lock);
            q.tail->next = node;
            q.tail = node;
        }

        std::string pop() {
            std::lock_guard<LOCK> lk(q.q_h_lock);
            NODE* node = q.head;
            NODE* newHead = node->next;
            if (nullptr == newHead) {
                return NULL_VALUE;
            }
            std::string res = newHead->value;
            q.head = newHead;
            delete node;
            return res;
        }
    private:
        queue_t<LOCK> q;
    };

    class Queue_Blocking_TwoLock {
    public:

        Queue_Blocking_TwoLock() {
            NODE* none = new NODE();
            none->next = nullptr;
            q.head = q.tail = none;
            e.lock();

        }

        void close() {
            close_ = true;
            e.unlock();
        }

        void push(const std::string& value) {
            NODE* node = new NODE;
            node->value = value;
            node->next = nullptr;
            std::lock_guard<std::mutex> lk(q.q_t_lock);
            bool needUnlock = false;

            q.tail->next = node;
            q.tail = node;
            //size++;
            e.unlock();
        }

        std::string pop() {
            std::lock_guard<std::mutex> lk(q.q_h_lock);
            e.lock();
            if (close_) {
                e.unlock();
                return NULL_VALUE;
            }
            NODE* node = q.head;
            NODE* newHead = node->next;
            std::string value = newHead->value;
            //size--;
            q.head = newHead;
            delete node;
            if (nullptr != q.head->next) {
                e.unlock();
            }
            return value;
        }

    private:
        queue_t<std::mutex> q;
        std::mutex e;
        bool close_ = false; //This is only for test.
    };

    class Queue_ConcurrentQueue {
        moodycamel::ConcurrentQueue<std::string> q;
    public:

        Queue_ConcurrentQueue() {
        }

        void close() {

        }

        void push(const std::string& value) {
            q.enqueue(value);
        }

        std::string pop() {
            std::string res;
            if (q.try_dequeue(res)) {
                return res;
            }
            return NULL_VALUE;
        }

    };
}