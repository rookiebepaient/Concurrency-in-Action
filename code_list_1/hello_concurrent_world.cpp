#include <iostream>
#include <thread>

void hello() {
    std::cout << "hello concurrent world\n";
}

void f(int i, const std::string &s) {
    std::cout << "i = " << i << ", s = " << s << std::endl;
}

// 应用程序的起始线程（initial thread）而言，该函数是main()
int main() {
    // 每个线程都需要一个起始函数（initial function），新线程从这个函数开始执行
    std::thread t(hello);
    // 该调用会令主线程等待子线程，前者负责执行main()函数，后者则与std::thread对象关联，即本例中的变量t
    t.join();

    f(1, "hello");

    std::cout << "hardware_concurrency = " << std::thread::hardware_concurrency() << std::endl;

    return 0;
}