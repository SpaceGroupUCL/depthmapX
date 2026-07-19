// genlib - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2026 depthmapX contributors

#pragma once

#include "genlib/comm.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <limits>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace depthmapX {

inline size_t configuredAnalysisThreadCount(size_t taskCount, size_t estimatedBytesPerWorker = 0) {
    size_t requested = 0;
    if (const char *value = std::getenv("DEPTHMAPX_THREADS")) {
        try {
            requested = static_cast<size_t>(std::stoul(value));
        } catch (...) {
            requested = 0;
        }
    }

    if (requested == 0 && taskCount < 256) {
        return 1;
    }

    size_t hardwareThreads = static_cast<size_t>(std::thread::hardware_concurrency());
    if (hardwareThreads == 0) {
        hardwareThreads = 2;
    }

    size_t workers = requested > 0 ? std::min(requested, hardwareThreads)
                                   : std::min<size_t>(hardwareThreads, 8);
    workers = std::max<size_t>(1, std::min(workers, taskCount));

    // Keep per-analysis scratch allocations under a conservative 512 MiB budget by default.
    if (estimatedBytesPerWorker > 0) {
        constexpr size_t MEMORY_BUDGET = size_t(512) * 1024 * 1024;
        const size_t memoryLimitedWorkers = std::max<size_t>(1, MEMORY_BUDGET / estimatedBytesPerWorker);
        workers = std::min(workers, memoryLimitedWorkers);
    }

    return std::max<size_t>(1, workers);
}

template <typename Function>
void parallelFor(size_t taskCount, size_t workerCount, Communicator *comm, Function &&function) {
    if (taskCount == 0) {
        return;
    }

    workerCount = std::max<size_t>(1, std::min(workerCount, taskCount));
    if (workerCount == 1) {
        for (size_t task = 0; task < taskCount; ++task) {
            if (comm && comm->IsCancelled()) {
                throw Communicator::CancelledException();
            }
            function(task, size_t(0));
            if (comm && (((task + 1) & 0x3f) == 0 || task + 1 == taskCount)) {
                comm->CommPostMessage(Communicator::CURRENT_RECORD, static_cast<int>(task + 1));
            }
        }
        return;
    }

    std::atomic<size_t> completed{0};
    std::atomic_bool stop{false};
    std::exception_ptr workerException;
    std::mutex exceptionMutex;

    std::vector<std::thread> workers;
    workers.reserve(workerCount);
    for (size_t worker = 0; worker < workerCount; ++worker) {
        workers.emplace_back([&, worker]() {
            // Static cyclic scheduling keeps reductions repeatable while still spreading
            // neighbouring roots across the available workers.
            for (size_t task = worker; task < taskCount && !stop.load(std::memory_order_relaxed);
                 task += workerCount) {
                if (comm && comm->IsCancelled()) {
                    stop.store(true, std::memory_order_relaxed);
                    break;
                }

                try {
                    function(task, worker);
                    completed.fetch_add(1, std::memory_order_release);
                } catch (...) {
                    {
                        std::lock_guard<std::mutex> lock(exceptionMutex);
                        if (!workerException) {
                            workerException = std::current_exception();
                        }
                    }
                    stop.store(true, std::memory_order_relaxed);
                    break;
                }
            }
        });
    }

    size_t lastReported = std::numeric_limits<size_t>::max();
    while (!stop.load(std::memory_order_relaxed) && completed.load(std::memory_order_acquire) < taskCount) {
        if (comm && comm->IsCancelled()) {
            stop.store(true, std::memory_order_relaxed);
            break;
        }
        const size_t current = completed.load(std::memory_order_acquire);
        if (comm && current != lastReported) {
            comm->CommPostMessage(Communicator::CURRENT_RECORD, static_cast<int>(current));
            lastReported = current;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    for (std::thread &worker : workers) {
        worker.join();
    }

    if (workerException) {
        std::rethrow_exception(workerException);
    }
    if (comm && comm->IsCancelled()) {
        throw Communicator::CancelledException();
    }
    if (comm) {
        comm->CommPostMessage(Communicator::CURRENT_RECORD, static_cast<int>(completed.load()));
    }
}

} // namespace depthmapX
