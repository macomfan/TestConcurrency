#pragma once

#include <map>

#include <iostream>
#include <mutex>
#include <list>
#include <utility>//std::pair
#include <vector>
#include <string>
#include <algorithm>    // std::find_if
#include <memory>
#include <functional>
#include <atomic>
#include <unordered_map>


namespace KV {

    void DelayInLookUp() {
        //        std::string str = std::to_string(1);
        //        str.append(" ");
    }

    class KV_NoLock {
    public:

        void add(int key, int value) {
            kv.emplace(key, value);
        }

        bool lookup(int key, int& value) {
            auto it = kv.find(key);
            if (it != kv.end()) {
                value = it->second;
                return true;
            }
            return false;
        }
    private:
        std::unordered_map<int, int> kv;
    };

    template <typename LOCK>
    class KV_LockAll {
        std::unordered_map<int, int> kv;
        LOCK m;
    public:

        void add(int key, int value) {
            std::lock_guard<LOCK> lk(m);
            kv.emplace(key, value);
        }

        void remove(int key) {
            std::lock_guard<LOCK> lk(m);
            kv.erase(key);
        }

        bool lookup(int key, int& value) {
            std::lock_guard<LOCK> lk(m);
            DelayInLookUp();
            auto it = kv.find(key);
            if (it != kv.end()) {
                value = it->second;
                return true;
            }
            return false;
        }
    };

    template <typename LOCK>
    class KV_RWLock {
        std::unordered_map<int, int> kv;
        LOCK m;
    public:

        void add(int key, int value) {
            m.wlock();
            kv.emplace(key, value);
            m.wunlock();
        }

        void remove(int key) {
            m.wlock();
            kv.erase(key);
            m.wunlock();
        }

        bool lookup(int key, int& value) {
            m.rlock();
            auto it = kv.find(key);
            DelayInLookUp();
            if (it != kv.end()) {
                value = it->second;
                m.runlock();
                return true;
            }
            m.runlock();
            return false;
        }
    };

    template <typename LOCK>
    class KV_CopyOnWrite {
        using Map = std::unordered_map<int, int>;
        using MapPtr = std::shared_ptr<Map>;

        MapPtr kv;
        LOCK m;
    public:

        KV_CopyOnWrite() {
            kv = std::make_shared<Map>();
        }

        void add(int key, int value) {
            std::lock_guard<LOCK> lk(m);
            if (!kv.unique()) {
                MapPtr newData = std::make_shared<Map>();
                kv.swap(newData);
            }
            kv->emplace(key, value);
        }

        void remove(int key) {
            std::lock_guard<LOCK> lk(m);
            if (!kv.unique()) {
                MapPtr newData = std::make_shared<Map>();
                kv.swap(newData);
            }
            kv->erase(key);
        }

        bool lookup(int key, int& value) {
            MapPtr data = getData();
            auto it = kv->find(key);
            DelayInLookUp();
            if (it != kv->end()) {
                value = it->second;
                return true;
            }
            return false;
        }
    private:

        MapPtr getData() {
            std::lock_guard<LOCK> lk(m);
            return kv;
        }
    };
}