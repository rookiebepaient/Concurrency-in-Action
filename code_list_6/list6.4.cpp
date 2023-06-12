/** \brief 单线程队列的简单实现
 *      
*/

#include <memory>

template <typename T>
class queue {
private:
    struct node {
        T data;
        std::unique_ptr<node> next;
        node(T data_) : data(std::move(data_)) {}
    };
    // 1.头指针
    std::unique_ptr<node> head;
    // 2.尾指针
    node *tail;

public:
    queue() : tail(nullptr) {}
    queue(const queue &other) = delete;
    queue& operator=(const queue &other) = delete;

    std::shared_ptr<T> try_pop() {
        if (head == nullptr) {
            return std::shared_ptr<T>();
        }
        const std::shared_ptr<T> res(
            std::make_shared<T>(std::move(head->data))
        );
        const std::unique_ptr<node> old_head = std::move(head);
        // 3. 与 4. 相关联 try_pop操作读取old_head->next。如果队列只有一个元素，则两个next指针的目标节点是重合的
        head = std::move(old_head->next);
        if (head == nullptr) {
            tail = nullptr;
        }
        return res;
    }

    void push(T new_val) {
        std::unique_ptr<node> p(std::make_shared<T>(std::move(new_val)));
        const node *new_tail = p.get();
        if (tail) {
            // 4. push操作更新tail->next。 可能不能辨别是否是同一个节点，就会在执行push()和try_pop()的过程中，无意中试图锁定同一互斥
            tail->next = std::move(p);
        } else {
            // 5. push操作可以同时改动头尾指针，所以需要两个互斥量来保护头尾指针
            head == std::move(p);
        }
        // 6. push操作可以同时改动头尾指针
        tail = new_tail;
    }

};