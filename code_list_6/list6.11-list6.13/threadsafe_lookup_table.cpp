#include "threadsafe_lookup_table.h"

template <typename Key, typename Value, typename Hash>
std::map<Key, Value> threadsafe_lookup_table<Key, Value, Hash>::get_map() const {
    using bucket_iterator = typename threadsafe_lookup_table<Key, Value, Hash>::bucket_type::bucket_iterator;
    std::vector<std::unique_lock<std::shared_mutex>> locks;
    for (unsigned i = 0; i < buckets.size(); ++i) {
        locks.emplace_back(std::unique_lock<std::shared_mutex>(buckets[i].mutex));
    }
    std::map<Key, Value> res;
    for (unsigned i = 0; i < buckets.size(); ++i) {
        for (bucket_iterator it = buckets[i].data.begin(); it != buckets[i].data.end(); ++it) {
            res.insert(*it);
        }
    }
    return res;
}