#include <algorithm>
#include <thread>
#include <vector>
#include <numeric>

template<typename Iterator, typename T>
struct accumulate_block {
    void operator()(Iterator first, Iterator last, T &result) {
        result = std::accumulate(first, last, result);
    }
};

template<typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init) {
    const unsigned long length = std::distance(first, last);
    if (!length) {\
        // 1. 加入传入的区间为空，就直接返回初始值
        return init;
    }
    // 每个线程处理元素的最低限定量
    const unsigned long min_per_thread = 25;
    // 2. 求出计算所需线程的最大数量，预防不合理的发起过多线程
    const unsigned long max_thread = (length + min_per_thread - 1) / min_per_thread;
    const unsigned long hardware_threads = std::thread::hardware_concurrency();
    // 3. 对比算出的最小线程数量和硬件线程数量，较小者即为实际需要运行的线程数量
    const unsigned long num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_thread);
    // 4. 将目标区间的长度除以线程数量，得出各线程需分担的元素数量
    const unsigned long block_size = length / num_threads;
    // 5. 存放中间结果
    std::vector<T> results(num_threads);
    // 装载线程 需要发起的线程数量比前面所求的num_threads少一个，因为主线程本身就算一个
    std::vector<std::thread> threads(num_threads - 1);
    Iterator block_start = first;
    for (unsigned long i = 0; i < (num_threads - 1); ++i) {
        Iterator block_end = block_start;
        // 6.迭代器block_end前移至当前小块的结尾
        std::advance(block_end, block_size);
        // 7.新线程就此启动，计算该小块的累加结果
        threads[i] = std::thread(accumulate_block<Iterator, T>(), 
                    block_start, block_end, std::ref(results[i]));
        // 8.
        block_start = block_end;
    }

    accumulate_block<Iterator, T>() (
        // 9.发起全部线程后，主线程随之处理最后一个小块
        block_start, last, results[num_threads - 1]
    );

    for(auto &entry : threads) {
        // 10.最后小块的累加一旦完成，我们就在一个循环里等待前面所有生成的线程
        entry.join();
    }
    // 11.调用std::accumulate()函数累加中间结果
    return std::accumulate(results.begin(), results.end(), init);
}