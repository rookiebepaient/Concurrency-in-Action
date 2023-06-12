/** \brief  清单6.2
 *          
 * 
 * 
*/

#include <queue>
#include <condition_variable>
#include <mutex>

template <typename T>
class threadsafe_queue {
private:
    mutable std::mutex mut;
    std::queue<T> data_queue;
    std::condition_variable data_cond;

public:
    threadsafe_queue() {}

    void push(T new_val) {  // 为什么没用引用传递参数
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(std::move(new_val));    // 为什么没有emplace
        // 1. 如果有多线程正在等待，那么此处只会被唤醒一个
        data_cond.notify_one();
    }

    // 2.
    void wait_and_pop(T &val) {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this] { return !data_queue.empty(); });
        val = std::move(data_queue.front());
        data_queue.pop();
    }

    // 3.
    std::shared_ptr<T> wait_and_pop() {
        std::unique_lock<std::mutex> lk(mut);
        // 4. 
        data_cond.wait(lk, [this] { return !data_queue.empty(); });
        // 在此处构建新的shared_ptr 可能产生异常。
        /** \note
         *  可能的解决办法
         *  1. data_cond.notify_one(); 改为 data_cond.notify_all(); 但是开销增大：大部分线程发现队列依然是空的，只能重新休眠
         *  2. 处理异常情况，在此函数中再次调用 data_cond.notify_one()
         *  3. 将共享指针的初始化移动到push函数中，让容器存储共享指针，从std::queue内部复制共享指针的示例操作不会抛出异常，所以wait_pop函数是异常安全的
        */
        std::shared_ptr<T> res(
            std::make_shared<T>(std::move(data_queue.front())));
        data_queue.pop();
        return res;
    }

    bool try_pop(T &val) {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty()) {
            return false;
        }
        val = std::move(data_queue.front());
        data_queue.pop();
        return true;
    }

    std::shared_ptr<T> try_pop() {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty()) {
            // 5.
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> res(
            std::make_shared<T>(std::move(data_queue.front())));
        data_queue.pop();
        return res;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }
};