#include <list>
#include <mutex>
#include <algorithm>

// 1.独立的全局变量
std::list<int> some_list;
// 2.由对应的std::mutex实例保护（另一个全局变量
std::mutex some_mutex;

void add_to_list(int new_val) {
    // 3.意义是使这两个函数对链表的访问互斥：假定add_to_list()正在改动链表，
    // 若操作进行到一半，则list_contains()绝对看不见该状态
    std::lock_guard<std::mutex> guard(some_mutex);
    some_list.push_back(new_val);
}

bool list_contians(int val_to_find) {
    // 4.
    std::lock_guard<std::mutex> guard(some_mutex);
    return std::find(some_list.begin(), some_list.end(), val_to_find)
            != some_list.end();
}