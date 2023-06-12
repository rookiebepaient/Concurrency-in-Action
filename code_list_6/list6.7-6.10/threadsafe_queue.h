/** \brief 清单6.7 采用锁操作并支持等待功能的线程安全队列
 *      清单6.8 压入数据
 *      清单6.9 wait_and_pop()
 *      清单6.10 try_pop() 和 empty()
 *  头文件
*/

#ifndef __THREADSAFEQUEUE__
#define __THREADSAFEQUEUE__

#include <memory>
#include <mutex>
#include <condition_variable>

template <typename T>
class threadsafe_queue {
private:
    struct node {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };
    std::mutex head_mutex;
    std::unique_ptr<node> head;
    std::mutex tail_mutex;
    node *tail;
    std::condition_variable data_cond;

    node *get_tail() {
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        return tail;
    }

    // 1. 移除都节点
    std::unique_ptr<node> pop_head() {
        std::unique_ptr<node> old_head = std::move(head);
        head = std::move(old_head->next);
        return old_head;
    }

    // 2. 等待数据加入空队列
    std::unique_lock<std::mutex> wait_for_data() {
        std::unique_lock<std::mutex> head_lock(head_mutex);
        data_cond.wait(head_lock, [&] { return head.get() != get_tail(); });
        // 3.在条件变量处等待，以lambda函数作为断言，并向调用者返回锁的实例
        return std::move(head_lock);
    }

    std::unique_ptr<node> wait_pop_head() {
        // 4. 头节点弹出的全过程都有同一个锁
        std::unique_lock<std::mutex> head_lock(wait_for_data());
        return pop_head(); 
    }

    std::unique_ptr<node> wait_pop_head(T &val) {
        // 5. 头节点弹出的全过程都有同一个锁
        std::unique_lock<std::mutex> head_lock(wait_for_data());
        val = std::move(*head->data);
        return pop_head();
    }

    std::shared_ptr<T> try_pop_head() {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if (head.get() == get_tail()) {
            return std::unique_ptr<node>();
        }
        return pop_head();
    }

    bool try_pop_head(T &val) {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if (head.get() == get_tail()) {
            return std::unique_ptr<node>();
        }
        val = std::move(*head->data);
        return pop_head();
    }

public:
    threadsafe_queue() : head(std::make_unique<node>()), tail(head.get()) {}
    threadsafe_queue(const threadsafe_queue &) = delete;
    threadsafe_queue &operator=(const threadsafe_queue &) = delete;

    std::shared_ptr<T> try_pop() {
        std::unique_ptr<node> old_head = try_pop_head();
        return old_head ? old_head->data : std::shared_ptr<T>();
    }

    bool try_pop(T &val) {
        std::unique_ptr<node> old_head = try_pop_head(val);
        return old_head != nullptr;
    }

    std::shared_ptr<T> wait_and_pop() {
        const std::unique_ptr<node> old_head = wait_pop_head();
        return old_head->data;
    }

    void wait_and_pop(T &val) {
        const std::unique_ptr<node> old_head = wait_pop_head(val); 
    }

    void push(T new_val);

    bool empty() {
        std::unique_lock<std::mutex> head_lock(head_lock);
        return (head.get() == get_tail());
    }
};

#endif // __THREADSAFEQUEUE__