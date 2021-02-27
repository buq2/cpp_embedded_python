#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <iostream>
#include <vector>
#include <thread>

namespace py = pybind11;
using namespace py::literals;

void Process(int thread_idx) {
    std::cout << "Thread started: " << thread_idx << std::endl;
    std::vector<int> data;
    for (int i = 0; i < 10000; ++i) {
        data.push_back(i);
    }

    for (int i = 0; i < 50; ++i) {
        std::cout << "Thread " << thread_idx << " processing " << i << std::endl;
        py::scoped_interpreter guard{};
        try {
            py::exec(R"(
                # Add current working directory and subdir to module search path
                # If build is under cwd, we catch the example modules.
                import sys,os;
                sys.path.append(os.getcwd())
                sys.path.append(os.path.join(os.getcwd(), '..'))
                sys.path.append(os.path.join(os.getcwd(), '..', '..'))
            )");

            py::module_ calc = py::module_::import("ex6_threaded");
            py::object result = calc.attr("sum")(data, 2);
            int n = result.cast<int>();

            std::cout << "Python returned val: " << n << std::endl;
        } catch(const std::exception &e) {
            std::cout << "Python code raised exception: " << std::endl;
            std::cout << e.what() << std::endl;
            break;
        }
    }
}

int main() {
    std::cout << "This example is brokenand demonstrates why things are hard when multiple threads are used with pybind11." << std::endl;
    std::vector<std::thread> threads;
    for (int i = 0; i < 20; ++i) {
        threads.emplace_back([=](){Process(i);});
    }
    for (auto &t : threads) {
        t.join();
    }
}