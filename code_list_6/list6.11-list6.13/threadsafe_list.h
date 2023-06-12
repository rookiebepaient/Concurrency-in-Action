/** \brief  清单6.13 支持迭代功能的线程安全的链表
 *  链表所需要的操作
 *      向链表中加入数据
 *      根据一定的条件从链表中移除数据项
 *      根据一定的条件在链表中查找数据
 *      更新符合条件的数据
 *      向另一容器复制链表中的全部数据
 * 
 *  需要用户给出的断言函数和功能函数代码编写得当，遵守多线程行为准则
 *  迭代操作总是单向的，总是从头结点开始，并且都是先锁住 next 节点的互斥，才解锁 current 的互斥
*/

#include <mutex>
#include <memory>

template <typename T>
class threadsafe_list {
    // 1. 单向链表
    struct node {
        std::mutex m;
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
        // 2. 头节点按照默认构造方式生成，next指针是nullptr
        node() : next() {}
        // 3. 新节点的创建
        node(const T &val) : data(std::make_shared<T>(val)) {}
    };

    node head;
public:
    threadsafe_list() {}
    ~threadsafe_list() {
        remove_if([] (const node &) { return true; });
    }
    threadsafe_list(const threadsafe_list &) = delete;
    threadsafe_list &operator=(const threadsafe_list &) = delete;

    void push_front(const T &val) {
        // 4. 先在堆上分配内存
        std::unique_ptr<node> new_node(std::make_unique<node>(val));
        std::lock_guard<std::mutex> lk(head.m);
        // 5. next指针此时为nullptr,然后锁住头节点的互斥，以便正确的读取next指针，
        new_node->next = std::move(head.next);
        // 6. 将头节点的next指针改为指向新节点
        head.next = std::move(new_node);
    }

    // 7. 将每项数据都直接传入用户给出的函数，在必要时可通过该函数更新数据，
    //      或将数据复制到另一个容器中，或执行任何其他操作
    template <typename Function>
    void for_each(Function f) {
        node *current = &head;
        // 8. 在最开始，锁住头节点的互斥
        std::unique_lock<std::mutex> lk(head.m);
        // 9. next指针的get()调用，安全的获取指向 next 结点的指针，只要不取得的指针不是null
        while (const node *next = current->next.get()) {
            // 10. 锁住指向的 next 节点
            std::unique_lock<std::mutex> next_lk(next->m);
            // 11. 只要锁住了next节点，即可释放current节点上的锁
            lk.unlock();
            // 12. 并按传入的参数调用指定的函数
            f(*next->data);
            current = next;
            // 13. 函数运行完成，更新current指针，使其指向刚刚处理的 next 节点,并将 next_lk 所持有的锁转移给lk
            lk = std::move(next_lk);
        }
    }

    // 14. 与for_each不同，找到匹配的数据则立即返回该数据的智能指针
    template <typename Predicate>
    std::shared_ptr<T> find_first_if(Predicate p) {
        node *current = &head;
        std::unique_lock<std::mutex> lk(head.m);
        while (const node *next = current->next.get()) {
            std::unique_lock<std::mutex> next_lk(next->m);
            lk.unlock();
            // 15. 如果找到了匹配的数据，用户给出的Predi断言函数就必须返回true，否则返回false
            if (p(*next->data)) {
                // 16. 找到匹配的数据，停止查找并将其返回
                return next->data;
            }
            current = next;
            lk = std::move(next_lk);
        }
        return std::shared_ptr<T>();
    }

    // 17. 该函数需要更新链表，其操作不能通过for_each()完成
    template <typename Predicate>
    void remove_if(Predicate p) {
        node *current = &head;
        std::unique_lock<std::mutex> lk(head.m);
        while (const node *next = current->next.get()) {
            std::unique_lock<std::mutex> next_lk(next->m);
            // 18. 如果给出的Predicate返回true
            if (p(*next->data)) {
                std::unique_ptr<node> old_next = std::move(current->next);
                // 19. 通过改变 current->next 从而移除该节点
                current->next = std::move(next->next);
                next_lk.unlock();
                // 20. 在改动完成后随即解锁 next 的互斥，令old_next暂时指向目标节点，
                //      一旦if分支结束，智能指针自动销毁，也连带删除目标节点 
                // 此处的销毁节点行为可能会发生条件竞争
                // 目标节点的销毁是在解锁以后发生(否则，销毁节点在前，但它所含的互斥已被锁住，销毁已锁的互斥是未定义行为)
                // 但这其实是安全的行为，因为current指针指向的节点上有锁，所以在删除过程中，没有线程可以越过 current 节点，从被删除节点上获取锁
            } else {    
                // 21. 如果Predicate函数返回false，移动到下一个节点进行考察
                lk.unlock();
                current = next;
                lk = std::move(next_lk);
            }
        }
    }
};