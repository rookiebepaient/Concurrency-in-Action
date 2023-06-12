/** \brief 清单6.3 存储std::shared_ptr<T>实例的线程安全队列
 *      与清单6.2相似，缺点都是有唯一的互斥锁保护整个数据结构，因为支持的并发程度受限。
 *      多个线程的阻塞可能在不同成员函数中发生，但事实上每次只允许一个线程操作队列数据，
 *      这是因为对于 std::queue 我们对其加锁的策略是，要么不保护，要么全部保护。可以
 *      提供更细粒度的锁来提高并发度
*/

#include <queue>
#include <condition_variable>
#include <mutex>
#include <memory>

template <typename T>
class threadsafe_queue {
private:
    mutable std::mutex mut;
    // 队列数据所存储的类型从值变成了共享指针
    std::queue<std::shared_ptr<T>> data_queue;
    std::condition_variable data_cond;

public:
    threadsafe_queue() {}

    void push(T new_val) {
        // 5. 数据通过std::shared_ptr<T>间接存储，与清单6.2内存操作必须在持锁状态下不同，内存分配的成本很高，这一做法
        // 缩短了互斥锁的持锁时长，分配内存时，允许其他线程在队列容器上执行操作，有利增强性能
        std::shared_ptr<T> data(
            std::make_shared<T>(std::move(new_val))
        );
        // 只需要在对共享变量进行操作的时候再加锁，所以当通过共享指针来分配内存的时候，不需要提前上锁
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(std::move(data));
        data_cond.notify_one();
    }

    void wait_and_pop(T &val) {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this] { return !data_queue.empty(); });
        // 1.根据指针提取出值
        val = std::move(*data_queue.front());
        data_queue.pop();
    }

    std::shared_ptr<T> wait_and_pop() {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this] { return !data_queue.empty(); });
        // 3.先从底层容器取出结果，然后返回给调用者
        std::shared_ptr<T> res(
            std::make_shared<T>(std::move(*data_queue.front()))
        );
        data_queue.pop();
        return res;
    }

    bool try_pop(T &val) {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty()) {
            return false;
        }
        // 2.根据指针提取出值
        val = std::move(*data_queue.front());
        data_queue.pop();
        return true;
    }

    std::shared_ptr<T> try_pop() {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty()) {
            return std::shared_ptr<T>();
        }
        // 4.先从底层容器取出结果，然后返回给调用者
        std::shared_ptr<T> res = data_queue.front();
        data_queue.pop();
        return res;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }
};