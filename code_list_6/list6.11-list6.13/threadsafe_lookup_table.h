/** \brief  设计采用精细粒度锁操作的map数据结构    
 *          清单6.11 线程安全的查找表
 * 为什么选择使用散列表来实现
 *  如果用红黑树，每次查找或者改动都要从根节点开始，就必须对根节点进行加锁操作。比起对整个数据结构单独使用一个锁差不多
 *  使用有序数组，因为不知道查找目标的具体位置，所以只能对整个有序数组加锁，影响并发可能
 *  使用散列表，为每一个桶单独上锁
 *      这个单元中说的桶(bucket), 就是存放每个键的基本单位。可以单独的为每个桶使用独立的锁。
 *      如果使用共享锁，支持多个读线程和一个写线程，就会令并发操作的机会增加N倍，N是桶的数目
*/

#ifndef __THREADSAFE_LOOKUP_TABLE__
#define __THREADSAFE_LOOKUP_TABLE__

#include <utility>      // pair
#include <list>
#include <shared_mutex>
#include <algorithm>    // find_if
#include <memory>
#include <vector>
#include <map>

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class threadsafe_lookup_table {
private:
    class bucket_type {
    private:
        // using bucket_value = std::pair<Key, Value>;
        // using bucket_data = std::list<bucket_value>;
        // using bucket_iterator = typename bucket_data::iterator;
        typedef std::pair<Key, Value> bucket_value;
        typedef std::list<bucket_value> bucket_data;
        typedef typename bucket_data::iterator bucket_iterator;
        bucket_data data;
        // 1. 每个桶由独立的共享锁实例来保护
        mutable std::shared_mutex mutex;
        // 2. 判断桶内是否有给定数据
        bucket_iterator find_entry_for(const Key &key) const {
            return std::find_if(data.begin(), data.end(),
                [&] (const bucket_value &item)
                { return item.first == key; }
            );
        }
    
    public:
        Value value_for(const  Key &key, const Value &default_value) const {
            // 3. 共享方式加锁，只读
            std::shared_lock<std::shared_mutex> lock(mutex);
            const bucket_iterator found_entry = find_entry_for(key);
            return (found_entry == data.end()) ? default_value : found_entry->second;
        }

        void add_or_update_mapping(const Key &key, const Value &value) {
            // 4. 独享方式加锁，
            std::unique_lock<std::shared_mutex> lock(mutex);
            const bucket_iterator found_entry = find_entry_for(key);
            if (found_entry == data.end()) {
                data.emplace_back(bucket_value(key, value));
            }  else {
                found_entry->second = value;
            }
        }

        void remove_mapping(const Key &key) {
            // 5. 独享方式加锁，
            std::unique_lock<std::shared_mutex> lock(mutex);
            const bucket_iterator found_entry = find_entry_for(key);
            if (found_entry != data.end()) {
                data.erase(found_entry);
            }
        }
    };
    // 6. 用数组来存放桶，可以通过构造函数来设定桶的数目，桶的数量最好的质数
    std::vector<std::unique_ptr<bucket_type>> buckets;
    Hash hasher;
    // 7. 桶的数目固定，对此函数的调用不需要加锁保护
    bucket_type &get_bucket(const Key &key) const {
        const std::size_t bucket_index = hasher(key) % buckets.size();
        return *(buckets[bucket_index]);
    }

public:
    using key_type = Key;
    using mapped_type = Value;
    using hash_type = Hash;

    threadsafe_lookup_table(unsigned num_buckets = 19, const Hash &hasher_ = Hash()) 
        : buckets(num_buckets), hasher(hasher_) {
            for (unsigned i = 0; i < num_buckets; ++i) {
                buckets[i].reset(std::make_unique<bucket_type>());
            }
    }
    threadsafe_lookup_table(const threadsafe_lookup_table &) = delete;
    threadsafe_lookup_table &operator=(const threadsafe_lookup_table &) = delete;

    Value value_for(const Key &key, const Value &default_val = Value()) const {
        // 8.
        return get_bucket(key).value_for(key, default_val);
    }

    void add_or_update_mapping(const Key &key, const Value &val) {
        // 9.
        get_bucket(key).add_or_update_mapping(key, val);
    }

    void remove_mapping(const Key &key) {
        // 10.
        get_bucket(key).remove_mapping(key);
    }

    std::map<Key, Value> get_map() const;
};



#endif // __THREADSAFE_LOOKUP_TABLE__