/* Copyright (c) 2021 Matti Jukola <buq2@buq2.com>, All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <iostream>
#include <vector>
#include <thread>
#include "py_multithread_helpers.hh"

namespace py = pybind11;
using namespace py::literals;



void Process(int thread_idx) {
    std::cout << "Thread started: " << thread_idx << std::endl;

    // Each thread must use its own thread state object
    PythonThreadState thread_state = PythonEnvironment::GetInstance().CreateThreadState();

    std::vector<int> data1;
    std::vector<int> data2;
    for (int i = 0; i < 10000; ++i) {
        data1.push_back(i);
        data2.push_back(i*2);
    }

    for (int i = 0; i < 10; ++i) {
        std::cout << "Thread " << thread_idx << " processing " << i << std::endl;
        try {
            auto lock = thread_state.GetLock(); // Lock GIL

            // We could move this import outside of the loop, but it
            // we would need to ensure that when its freed, we also lock GIL, etc
            // so it would needlessly complicate this example.
            py::module_ calc = py::module_::import("ex7_threaded2");

            py::object result = calc.attr("sum")(data1, data2);
            auto n = result.cast<std::vector<int>>();

            std::cout << "Python returned vector with n elements: " << n.size() << std::endl;
        } catch(const std::exception &e) {
            std::cout << "Python code raised exception: " << std::endl;
            std::cout << e.what() << std::endl;
            break;
        }
    }
}

int main() {
    // Init Python
    PythonEnvironment& env = PythonEnvironment::GetInstance();

    {
        // Setup paths
        auto ts = env.CreateThreadState();
        auto lock = ts.GetLock();
        py::exec(R"(
            # Add current working directory and subdir to module search path
            # If build is under cwd, we catch the example modules.
            import sys,os;
            sys.path.append(os.getcwd())
            sys.path.append(os.path.join(os.getcwd(), '..'))
            sys.path.append(os.path.join(os.getcwd(), '..', '..'))
        )");
    }

    std::vector<std::thread> threads;
    for (int i = 0; i < 20; ++i) {
        threads.emplace_back([&](){Process(i);});
    }
    for (auto &t : threads) {
        t.join();
    }
}