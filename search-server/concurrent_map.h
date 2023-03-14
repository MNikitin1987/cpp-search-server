#pragma once

#include <mutex>
#include <map>
#include <vector>
#include <future>

using namespace std;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    struct Pack {
        mutex mutex_;
        map<Key, Value> map_;
    };

    struct Access {
        lock_guard<mutex> guard;
        Value& ref_to_value;

        Access(Pack& pack, Key key)
            :   guard(pack.mutex_)
            ,   ref_to_value(pack.map_[key]) {
        }
    };

    explicit ConcurrentMap(size_t num)
        :   packs_(num) {
    }

    Access operator[](const Key& key) {
        const size_t map_num = static_cast<uint64_t>(key) % packs_.size();

        return Access(packs_[map_num], key);
    }

    map<Key, Value> BuildOrdinaryMap() {
        map<Key, Value> result;

        for (auto& [mutex, map] : packs_) {
            lock_guard guard(mutex);
            result.insert(map.begin(), map.end());
        }

        return result;
    }

    void erase(const Key& key) {
        const size_t map_num = static_cast<uint64_t>(key) % packs_.size();
        {
            lock_guard<mutex> guard(packs_[map_num].mutex_);
            packs_[map_num].map_.erase(key);
        }
    }

private:
    vector<Pack> packs_;
};