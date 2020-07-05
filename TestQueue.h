#pragma once

#include <thread>
#include <algorithm>
#include <vector>
#include <atomic>
#include <functional>
#include "Performance.h"
#include "Queue.h"
#include "Lock.h"

typedef struct {
    int msgNum;
    int producerNum;
    int consumerNum;
    std::function<void(const std::string& value) > fn;
} CONTEXT;

template <typename QTYPE>
void TestQBlock(const std::string& testcase, CONTEXT context) {
    int msgNum = context.msgNum;
    int producerNum = context.producerNum;
    int consumerNum = context.consumerNum;
    std::cout << "Start " << testcase << " Msg: " << msgNum * producerNum << " Producer: " << producerNum << " Consumer: " << consumerNum << std::endl;
    std::atomic<uint64_t> msgSize(msgNum * producerNum);
    Time t;
    QTYPE queue;
    std::vector<std::thread> pool;
    auto producer = [&](int msgNum, int threadNum) {
        for (int num = 0; num < threadNum; num++) {
            pool.push_back(std::thread([&](int msgNum, int current) {
                for (int i = 0; i < msgNum; i++) {
                    std::string value(std::to_string(current));
                    value.append("_");
                    value.append(std::to_string(i));
                    queue.push(value);

                }
            }, msgNum, num));
        }

    };
    auto consumer = [&](int threadNum) {
        for (int i = 0; i < threadNum; i++) {
            pool.push_back(std::thread([&]() {
                while (msgSize != 0) {
                    std::string value = queue.pop();
                    if (!value.empty()) {
                        context.fn(value);
                        msgSize--;
                    }
                    if (0 == msgSize) {
                        queue.close();
                    }
#ifdef TRACE
                    std::cout << value << std::endl;
#endif
                }
            }));
        }

    };

    t.start();
    consumer(consumerNum);
    producer(msgNum, producerNum);
    for (int i = 0; i < pool.size(); i++) {
        pool[i].join();
    }
    t.end(testcase);
}

template <typename FUNC>
void StartTestQueue(int msgNum, int producerNum, int consumerNum, FUNC fn) {
    CONTEXT context;
    context.msgNum = msgNum;
    context.consumerNum = consumerNum;
    context.producerNum = producerNum;
    context.fn = fn;
    std::this_thread::sleep_for(std::chrono::seconds(1));


    TestQBlock<Q::Queue_Blocking>("Test blocking Q", context);
    TestQBlock<Q::Queue_NonBlocking_TwoLock<>>("Test non-blocking 2 lock Q", context);
    TestQBlock<Q::Queue_Blocking_TwoLock>("Test blocking 2 lock Q", context);
    TestQBlock<Q::Queue_ConcurrentQueue>("Test ConcurrentQueue Q", context);
    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << "=============================================" << std::endl;
    TestQBlock<Q::Queue_Blocking>("Test blocking Q", context);
    TestQBlock<Q::Queue_Blocking>("Test blocking Q", context);
    TestQBlock<Q::Queue_Blocking>("Test blocking Q", context);
    std::cout << "=============================================" << std::endl;
    TestQBlock<Q::Queue_NonBlocking<>>("Test non-blocking Q with mutex", context);
    TestQBlock<Q::Queue_NonBlocking<>>("Test non-blocking Q with mutex", context);
    TestQBlock<Q::Queue_NonBlocking<>>("Test non-blocking Q with mutex", context);
    std::cout << "=============================================" << std::endl;
    TestQBlock<Q::Queue_Blocking_TwoLock>("Test blocking 2 lock Q", context);
    TestQBlock<Q::Queue_Blocking_TwoLock>("Test blocking 2 lock Q", context);
    TestQBlock<Q::Queue_Blocking_TwoLock>("Test blocking 2 lock Q", context);
    std::cout << "=============================================" << std::endl;
    TestQBlock<Q::Queue_NonBlocking_TwoLock<>>("Test non-blocking 2 lock Q", context);
    TestQBlock<Q::Queue_NonBlocking_TwoLock<>>("Test non-blocking 2 lock Q", context);
    TestQBlock<Q::Queue_NonBlocking_TwoLock<>>("Test non-blocking 2 lock Q", context);
    std::cout << "=============================================" << std::endl;
    TestQBlock<Q::Queue_ConcurrentQueue>("Test ConcurrentQueue Q", context);
    TestQBlock<Q::Queue_ConcurrentQueue>("Test ConcurrentQueue Q", context);
    TestQBlock<Q::Queue_ConcurrentQueue>("Test ConcurrentQueue Q", context);
    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << "=============================================" << std::endl;
    //    TestQBlock<Q::Queue_NonBlocking<L::SpinLock_atomic>>("Test non-blocking Q with Spin atomic", context);
    //    TestQBlock<Q::Queue_NonBlocking<L::SpinLock_atomic>>("Test non-blocking Q with Spin atomic", context);
    //    TestQBlock<Q::Queue_NonBlocking<L::SpinLock_atomic>>("Test non-blocking Q with Spin atomic", context);
    std::cout << "=============================================" << std::endl;
    TestQBlock<Q::Queue_NonBlocking_TwoLock < L::SpinLock_atomic >> ("Test non-blocking 2 lock Q with Spin atomic", context);
    TestQBlock<Q::Queue_NonBlocking_TwoLock < L::SpinLock_atomic >> ("Test non-blocking 2 lock Q with Spin atomic", context);
    TestQBlock<Q::Queue_NonBlocking_TwoLock < L::SpinLock_atomic >> ("Test non-blocking 2 lock Q with Spin atomic", context);
    std::cout << "=============================================" << std::endl;
    TestQBlock<Q::Queue_NonBlocking_TwoLock < L::SpinLock_CAS >> ("Test non-blocking 2 lock Q with Spin CAS", context);
    TestQBlock<Q::Queue_NonBlocking_TwoLock < L::SpinLock_CAS >> ("Test non-blocking 2 lock Q with Spin CAS", context);
    TestQBlock<Q::Queue_NonBlocking_TwoLock < L::SpinLock_CAS >> ("Test non-blocking 2 lock Q with Spin CAS", context);
    std::this_thread::sleep_for(std::chrono::seconds(1));
}