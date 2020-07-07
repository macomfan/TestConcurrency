#pragma once

#include <chrono>
#include <iostream>
#include <functional>
#include <thread>
#include <mutex>
#include "Performance.h"
#include "KV.h"
#include "Lock.h"

typedef struct {
    uint32_t itemNum;
    uint32_t readerNum;
    uint32_t writeNum;
} KV_CONTEXT;

template <typename KV>
void TestKV(const char* title, KV_CONTEXT context) {
    Time t;
    KV kv;
    std::cout << "=== Start test " << title << " ===" << std::endl;
    t.start();
    for (int i = 0; i < context.itemNum; i++) {
        kv.add(i, i);
    }
    t.end("Initialization");
    for (int i = 0; i < context.itemNum; i++) {
        int value = 0;
        kv.lookup(-1, value);
    }
    for (int i = 0; i < context.itemNum; i++) {
        int value = 0;
        kv.lookup(i, value);
    }
    t.end("Single thread read");
    std::vector<std::thread> pool;
    auto reader = [&](int itemNum, int threadNum) {
        for (int i = 0; i < threadNum; i++) {
            pool.push_back(std::thread([&](int itemNum) {
                for (int i = 0; i < itemNum; i++) {
                    int value = 0;
                    kv.lookup(-1, value);
                }
                for (int i = 0; i < itemNum; i++) {
                    int value = 0;
                    kv.lookup(i, value);
                }
            }, itemNum));
        }
    };
    auto writer = [&](int itemNum, int threadNum) {
        for (int i = 0; i < threadNum; i++) {
            pool.push_back(std::thread([&](int itemNum) {
                for (int i = 0; i < context.itemNum; i++) {
                    kv.remove(i);
                    kv.add(i, i);
                }
            }, itemNum));
        }
    };

    reader(context.itemNum, context.readerNum);
    writer(context.itemNum, context.writeNum);
    for (int i = 0; i < pool.size(); i++) {
        pool[i].join();
    }
    t.end("End");
}

void StartTestKV() {
    KV_CONTEXT context;
    context.itemNum = 9999999;
    context.readerNum = 2;
    context.writeNum = 1;
    TestKV<KV::KV_LockAll < std::mutex >> ("LockAll - mutex", context);
    TestKV<KV::KV_LockAll < std::mutex >> ("LockAll - mutex", context);
    TestKV<KV::KV_LockAll < std::mutex >> ("LockAll - mutex", context);
    TestKV<KV::KV_LockAll < L::SpinLock_exchange >> ("LockAll - exchange", context);
    TestKV<KV::KV_LockAll < L::SpinLock_exchange >> ("LockAll - exchange", context);
    TestKV<KV::KV_LockAll < L::SpinLock_exchange >> ("LockAll - exchange", context);
    TestKV<KV::KV_LockAll < L::SpinLock_atomic >> ("LockAll - atomic", context);
    TestKV<KV::KV_LockAll < L::SpinLock_atomic >> ("LockAll - atomic", context);
    TestKV<KV::KV_LockAll < L::SpinLock_atomic >> ("LockAll - atomic", context);
    TestKV<KV::KV_CopyOnWrite < L::SpinLock_CAS >> ("CopyOnWrite", context);
    TestKV<KV::KV_CopyOnWrite < L::SpinLock_CAS >> ("CopyOnWrite", context);
    TestKV<KV::KV_CopyOnWrite < L::SpinLock_CAS >> ("CopyOnWrite", context);
    TestKV<KV::KV_RWLock < L::AtomicRWLock >> ("RW - Atomic", context);
    TestKV<KV::KV_RWLock < L::AtomicRWLock >> ("RW - Atomic", context);
    TestKV<KV::KV_RWLock < L::AtomicRWLock >> ("RW - Atomic", context);
    TestKV<KV::KV_RWLock < L::LinuxRWLock >> ("RW - Linux", context);
    TestKV<KV::KV_RWLock < L::LinuxRWLock >> ("RW - Linux", context);
    TestKV<KV::KV_RWLock < L::LinuxRWLock >> ("RW - Linux", context);
}