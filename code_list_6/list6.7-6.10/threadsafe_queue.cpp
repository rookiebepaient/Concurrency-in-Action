#include "threadsafe_queue.h"

template <typename T>
void threadsafe_queue<T>::push(T new_val) {
    std::shared_ptr<T> new_data(
        std::make_shared<T>(std::move(new_val))
    );
    std::unique_ptr<node> p(std::make_unique<node>());
    {
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        tail->data = new_data;
        const node *new_tail = p.get();
        tail->next = std::move(p);
        tail = new_tail;
    }
    data_cond.notify_one();
}