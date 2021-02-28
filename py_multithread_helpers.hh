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

#pragma once

#include <pybind11/embed.h>
#include <thread>

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
        pybind11::initialize_interpreter();

        // initialize_interpreter does basically following, but also initializes some pybind22 states
        // const auto init_signal_handlers = true;
        // Py_InitializeEx(init_signal_handlers); // Start python

#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION < 9
        if (!PyEval_ThreadsInitialized()) {
            // Needed for multithreading, missing from basic pybind11
            // 3.7 call automatically with Py_Initialize()
            // Will be removed in 3.11
            PyEval_InitThreads(); 
        }
#endif

        // Releas GIL so threading can start
        ts_ = PyThreadState_Get();
        PyEval_ReleaseThread(ts_);
    }

    ~PythonEnvironment() {
        pybind11::finalize_interpreter();
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