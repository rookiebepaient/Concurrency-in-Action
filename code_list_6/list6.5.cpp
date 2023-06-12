/** \brief 清单6.6 带有精细粒度锁的线程安全队列
 *      
*/

#include <memory>
#include <mutex>

template <typename T>
class threadsafe_queue {
private:
    struct node{
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };
    std::mutex head_mutex;
    std::unique_ptr<node> head;
    std::mutex tail_mutex;
    node *tail;
    
    node *get_tail() {
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        return tail;
    }

    std::unique_ptr<node> pop_head() {
        // 此处必须先对头节点上锁，再对尾节点进行上锁
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if (head.get() == get_tail()) {
            return nullptr;
        }
        std::unique_ptr<node> old_head = std::move(head);
        head = std::move(old_head->next);
        return old_head;
    }

public:
    threadsafe_queue() : head(std::make_unique<node>()), tail(head.get()) {}
    threadsafe_queue(const threadsafe_queue &other) = delete;
    threadsafe_queue &operator=(const threadsafe_queue &other) = delete;

    std::shared_ptr<T> try_pop() {
        std::unique_ptr<node> old_head = pop_head();
        return old_head ? old_head->data : std::shared_ptr<T>();
    }

    void push(T new_val) {
        std::shared_ptr<T> new_data(
            std::make_shared<T> (std::move(new_val))
        );
        std::unique_ptr<node> p(std::make_unique<node>());
        const node *new_tail = p.get();
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        tail->data = new_data;
        tail->next = std::move(p);
        tail = new_tail;
    }
};

/** \note
 *  这个程序的不变量
 *      tail->next = nullptr
 *      tail->data = nullptr
 *      head == tail 说明队列为空
 *      对于每个节点x，只要 x != tail, 则x -> data 指向一个T类型的实例，则x->next 指向后续节点
 *      x->next == tail 说明x是最后一个节点。从head指针指向的节点出发，跟随next指针向下访问最终回到tail
*/