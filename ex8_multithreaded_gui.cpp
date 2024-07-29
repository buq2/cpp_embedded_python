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
#ifdef _MSC_VER
#define _STL_CRT_SECURE_INVALID_PARAMETER(expr) _CRT_SECURE_INVALID_PARAMETER(expr)
#endif
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
    auto thread_state = PythonEnvironment::GetInstance().CreateThreadState();

    for (int i = 0; i < 30; ++i) {
        try {
            auto lock = thread_state->GetLock(); // Lock GIL

            // We could move this import outside of the loop, but it
            // we would need to ensure that when its freed, we also lock GIL, etc
            // so it would needlessly complicate this example.
            py::module_ gui = py::module_::import("ex8_threaded_gui");

            gui.attr("update_gui_info")(thread_idx);
        } catch(const std::exception &e) {
            std::cout << "Python code raised exception: " << std::endl;
            std::cout << e.what() << std::endl;
            break;
        }

        // Add small delay which simulates "other work".
        // Without any delay, GIL is help by C++ threads all the time
        // and Python GUI thread does not get a change to acquire it.
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(300ms);
    }

    std::cout << "Thread exiting: " << thread_idx << std::endl;
}

int main() {
    // Init Python
    PythonEnvironment& env = PythonEnvironment::GetInstance();

    {
        // Setup paths
        auto ts = env.CreateThreadState();
        auto lock = ts->GetLock();
        py::exec(R"(
            # Add current working directory and subdir to module search path
            # If build is under cwd, we catch the example modules.
            import sys,os;
            sys.path.append(os.getcwd())
            sys.path.append(os.path.join(os.getcwd(), '..'))
            sys.path.append(os.path.join(os.getcwd(), '..', '..'))

            import numpy as np # For some reason numpy needs to be imported from the main thread
        )");
    }

    std::vector<std::thread> threads;
    for (int i = 0; i < 20; ++i) {
        threads.emplace_back([=](){Process(i);});
    }
    for (auto &t : threads) {
        t.join();
    }
}
