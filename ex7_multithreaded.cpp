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

namespace py = pybind11;
using namespace py::literals;

// Helper classes for Python >= 3.3

// PyThreadSafe is interpreter and thread specific object.
// It should not be used form other threads.
class PythonThreadState {
    class Lock;
 public:
    PythonThreadState(PyInterpreterState* interpreter) 
        :
        state_(PyThreadState_New(interpreter)),
        thread_id_(std::this_thread::get_id())
    {}

    /// Lock GIL
    /// Returned Lock object must be kept alive during any pybind11 / Python method calls
    Lock GetLock() {
        CheckThread();
        return Lock(state_);
    }

    ~PythonThreadState() {
        CheckThread();
        // Need to aqcuire GIL
        PyEval_RestoreThread(state_);

        // Clear and destroy
        PyThreadState_Clear(state_); //GIL needs to be held
        PyThreadState_DeleteCurrent(); // No need for GIL
        state_ = nullptr;
    }
 private:
    /// Ensure that current thread is the same one which created this object
    void CheckThread() {
        if (thread_id_ != std::this_thread::get_id()) {
            throw std::runtime_error("Tried to lock from thread which does not own PythonThreadState");
        }
    }

    class Lock {
     public:
        Lock(PyThreadState *ts) 
            :
            ts_(ts) 
        {
            // Lock GIL, set current thread state to ts
            PyEval_RestoreThread(ts_); 
        }

        ~Lock() {
            // Release GIL
            PyEval_ReleaseThread(ts_);
        }
     private:
        PyThreadState* ts_;
    };
 private:
    PyThreadState* state_;
    std::thread::id thread_id_;
};

/// This class initializes the Python environment.
class PythonEnvironment {
 public:
    static PythonEnvironment& GetInstance()
    {
        static PythonEnvironment instance; 
        return instance;
    }

    PyInterpreterState* GetInterpreter() {
        // Global state of Python interpreter.
        // We could have sub interpreters, which kinda have their own environment,
        // but let's use just one so module loading is faster.
        return PyInterpreterState_Main();
    }

    PythonThreadState CreateThreadState() 
    {
        return PythonThreadState(GetInterpreter());
    }
 private:
    PythonEnvironment() :
        ts_(nullptr)
    {
        // If we are using pybind11, we should initialize using its own initializer
        // This locks GIL
        py::initialize_interpreter();

        // initialize_interpreter does basically following, but also initializes some pybind22 states
        // const auto init_signal_handlers = true;
        // Py_InitializeEx(init_signal_handlers); // Start python

        if (!PyEval_ThreadsInitialized()) {
            // Needed for multithreading, missing from basic pybind11
            // 3.7 call automatically with Py_Initialize()
            // Will be removed in 3.11
            PyEval_InitThreads(); 
        }

        // Releas GIL so threading can start
        ts_ = PyThreadState_Get();
        PyEval_ReleaseThread(ts_);
    }

    ~PythonEnvironment() {
        py::finalize_interpreter();
        // Or without pybind11
        // Py_Finalize();
    }

    PythonEnvironment(const PythonEnvironment &) = delete;
    PythonEnvironment &operator=(const PythonEnvironment &) = delete;
    PythonEnvironment &operator=(PythonEnvironment &&) = delete;
 private:
    // Do we need to use this?
    PyThreadState* ts_;
};

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

    for (int i = 0; i < 50; ++i) {
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