#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <iostream>
#include <vector>

namespace py = pybind11;
using namespace py::literals;

int main() {
    std::vector<int> data1{1,2,3,4,5};
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

        py::module_ sys = py::module_::import("sys");
        py::print("Paths from which modules are searched:", sys.attr("path"));

        py::module_ calc = py::module_::import("ex5_sum");
        py::object result = calc.attr("sum")(data1, 2);
        int n = result.cast<int>();

        std::cout << "Python returned val: " << n << std::endl;
    } catch(const std::exception &e) {
        std::cout << "Python code raised exception: " << std::endl;
        std::cout << e.what() << std::endl;
    }
}