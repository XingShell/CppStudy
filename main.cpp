#include <iostream>
// size: 72704

#include "util/systemUtil.h"
#include "10_1/原子操作CAS与锁的实现.h"

//#include "10_2/无锁消息队列.h"

// hook mutex
//#include "10_3/死锁.h"
//#include "10_3/graph.h"

#include "10_4/内存泄露检测.h"

#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;


int main() {
    // "/tmp/tmp.wHgMUUjupg/cmake-build-debug"
    std::cout << "Current working directory: " << fs::current_path() << endl;
    // test_10_1();
    // test_10_2();
    // test_10_3();
    // test_graph();
    test_10_4();
    return 0;
}
