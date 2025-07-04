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

// Compatibility macros for different Python versions
#if PY_VERSION_HEX < 0x03020000
    // Python < 3.2: No finalization check available
    #define Py_IsFinalizing() (0)
#elif PY_VERSION_HEX < 0x03070000
    // Python 3.2-3.6: Use _Py_Finalizing global variable
    #ifdef __cplusplus
    extern "C" {
    #endif
    extern PyThreadState* _Py_Finalizing;
    #ifdef __cplusplus
    }
    #endif
    #define Py_IsFinalizing() (_Py_Finalizing != NULL)
#elif PY_VERSION_HEX < 0x030D0000
    // Python 3.7-3.12: _Py_IsFinalizing() is available
    // Note: It's not officially public API, but it's the only option
    #ifndef Py_IsFinalizing
        #define Py_IsFinalizing() _Py_IsFinalizing()
    #endif
#endif
// Python 3.13+: Py_IsFinalizing() is part of public API

// Helper classes for Python >= 3.3

// PyThreadSafe is interpreter and thread specific object.
// It should not be used from other threads.
class PythonThreadState {
    class Lock;
 public:
    PythonThreadState(PyInterpreterState* interpreter)
        :
        state_(nullptr),
        thread_id_(std::this_thread::get_id()),
        was_new_(true)
    {
        // Check if Python is still running
        if (!Py_IsFinalizing()) {
            state_ = PyThreadState_New(interpreter);
        }
    }

    PythonThreadState(PyThreadState* state)
        :
        state_(state),
        thread_id_(std::this_thread::get_id()),
        was_new_(false)
    {}

    /// Lock GIL
    /// Returned Lock object must be kept alive during any pybind11 / Python method calls
    std::unique_ptr<Lock> GetLock() {
        CheckThread();
        return std::unique_ptr<Lock>(new Lock(state_, was_new_));
    }

    ~PythonThreadState() {
        CheckThread();

        if (!was_new_ || !state_) {
            // No need to do anything
            return;
        }

        // Check if Python is finalizing
        if (Py_IsFinalizing()) {
            // Don't attempt cleanup during finalization
            return;
        }

        // For new thread states, we need to properly clean up
        // We need to make this thread state current to clean it up
        PyThreadState* old_state = PyThreadState_Swap(state_);

        // Clear and destroy
        PyThreadState_Clear(state_);
        PyThreadState_DeleteCurrent();

        // Note: We can't restore the old state after DeleteCurrent
        // The thread now has no associated thread state
        state_ = nullptr;
    }
 private:
    /// Ensure that current thread is the same one which created this object
    void CheckThread() {
        if (thread_id_ != std::this_thread::get_id()) {
            throw std::runtime_error("Tried to lock from thread which does not own PythonThreadState");
        }
        // Note: PyGILState_Check() is not reliable in all contexts
        if (PyGILState_Check()) {
            throw std::runtime_error("Tried to lock GIL twice on same thread");
        }
    }

    class Lock {
     public:
        Lock(PyThreadState *ts, bool was_new)
            :
            ts_(ts),
            was_new_(was_new)
        {
            if (!ts_ || Py_IsFinalizing()) {
                // No thread state or Python is finalizing, don't acquire
                ts_ = nullptr;
                return;
            }

            // Get GIL
            if (!was_new) {
                PyEval_RestoreThread(ts_);
            } else {
                PyEval_AcquireThread(ts_);
            }
        }

        ~Lock() {
            if (!ts_ || Py_IsFinalizing()) {
                return;
            }

            // Release GIL
            PyEval_ReleaseThread(ts_);
        }
     private:
        Lock(const Lock &) = delete;
        Lock &operator=(const Lock &) = delete;
        Lock &operator=(Lock &&) = delete;
        PyThreadState* ts_{nullptr};
        bool was_new_;
    };
 private:
    PythonThreadState(const PythonThreadState &) = delete;
    PythonThreadState &operator=(const PythonThreadState &) = delete;
    PythonThreadState &operator=(PythonThreadState &&) = delete;
    PyThreadState* state_{nullptr};
    std::thread::id thread_id_;
    bool was_new_{false};
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
        return ts_ ? ts_->interp : nullptr;
    }

    std::unique_ptr<PythonThreadState> CreateThreadState()
    {
        if (Py_IsFinalizing() || !ts_) {
            // Python is finalizing or not initialized
            return nullptr;
        }

        if (thread_id_ == std::this_thread::get_id()) {
            // Creating from same thread
            return std::unique_ptr<PythonThreadState>(new PythonThreadState(ts_));
        }

        // Need to create a new one
        auto out = std::unique_ptr<PythonThreadState>(new PythonThreadState(GetInterpreter()));
        return out;
    }
 private:
    PythonEnvironment() :
        ts_(nullptr),
        thread_id_(std::this_thread::get_id()),
        initialized_(false)
    {
        // Check if Python is already initialized (might be in some embedded scenarios)
        if (Py_IsInitialized()) {
            ts_ = PyThreadState_Get();
            initialized_ = false;  // We didn't initialize it
            return;
        }

        // If we are using pybind11, we should initialize using its own initializer
        // This locks GIL
        pybind11::initialize_interpreter();
        initialized_ = true;

        // initialize_interpreter does basically following, but also initializes some pybind11 states
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

        // Release GIL so threading can start
        // Basically same as PyEval_SaveThread
        ts_ = PyThreadState_Get();
        if (ts_) {
            PyEval_ReleaseThread(ts_);
        }
    }

    ~PythonEnvironment() {
        // Only finalize if we initialized
        if (initialized_ && ts_ && !Py_IsFinalizing()) {
            PyEval_RestoreThread(ts_);
            pybind11::finalize_interpreter();
            // Or without pybind11
            // Py_Finalize();
        }
    }

 private:
    PythonEnvironment(const PythonEnvironment &) = delete;
    PythonEnvironment &operator=(const PythonEnvironment &) = delete;
    PythonEnvironment &operator=(PythonEnvironment &&) = delete;
    PyThreadState* ts_;
    std::thread::id thread_id_;
    bool initialized_;  // Track if we initialized Python
};
