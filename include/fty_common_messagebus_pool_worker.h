/*  =========================================================================
    fty_common_messagebus_pool_worker - class description

    Copyright (C) 2014 - 2020 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
*/

#ifndef FTY_COMMON_MESSAGEBUS_POOL_WORKER_H_INCLUDED
#define FTY_COMMON_MESSAGEBUS_POOL_WORKER_H_INCLUDED

#include "fty_common_messagebus_library.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <tuple>
#include <type_traits>
#include <vector>
#include <queue>

namespace messagebus {

/**
 * \brief Pool of worker threads.
 */
class PoolWorker {
public:
    /**
     * \brief Create a pool of worker threads.
     * \param workers Number of workers (job will be processed synchronously if 0).
     */
    PoolWorker(size_t workers = std::thread::hardware_concurrency() + 1);

    // PoolWorker can't be copied, assigned or moved.
    PoolWorker() = delete;
    PoolWorker(const PoolWorker&) = delete;
    PoolWorker(PoolWorker&&) = delete;
    PoolWorker& operator=(const PoolWorker&) = delete;
    void operator=(PoolWorker&&) = delete;

    /**
     * \brief Destroy the pool of workers.
     *
     * Once the destructor is called, PoolWorker will wait until all scheduled jobs
     * are completed before returning.
     */
    ~PoolWorker();

    /**
     * \brief Offload job (do not keep a std::future for the result).
     * \param job Callable of the job to do.
     * \param args Arguments to pass to the callable.
     */
    template<
        typename Function,
        typename... Args
    >
    auto offload(Function&& fn, Args&&... args) -> void {
        // Package the job into a storable form.
        std::function<void()> packagedJob = std::bind(std::forward<Function&&>(fn), std::forward<Args&&>(args)...);

        // Add a non-rescheduling job.
        addJob([packagedJob]() -> bool { packagedJob(); return false; });
    }

    /**
     * \brief Queue job (keep a std::future for the result).
     * \param fn Callable of the job to do.
     * \param args Arguments to pass to the callable.
     * \return A future of the return value of the callable.
     */
    template<
        typename Function,
        typename... Args,
        typename ReturnType = decltype(std::declval<Function&&>()(std::declval<Args&&>()...))
    >
    auto queue(Function&& fn, Args&&... args) -> std::future<ReturnType> {
        // Package the job into a storable form.
        auto packagedJob = std::make_shared<std::packaged_task<ReturnType()>>(std::bind(std::forward<Function&&>(fn), std::forward<Args&&>(args)...));

        // Add a non-rescheduling job.
        addJob([packagedJob]() -> bool { (*packagedJob)(); return false; });
        return packagedJob->get_future();
    }

    /**
     * \brief Schedule job (queue when the std::future is ready).
     * \warning Jobs cannot be scheduled with a PoolWorker of 0 threads!
     * \param fn Callable of the job to do.
     * \param arg Future argument to pass to the callable.
     */
    template<
        typename Function,
        typename Arg
    >
    auto schedule(Function&& fn, std::shared_future<Arg> arg) -> void {
        /**
         * Add a self-rescheduling job that yields if the future isn't ready.
         * Once the future is ready, execute the job and don't reschedule.
         */
        addJob([fn = std::forward<decltype(fn)>(fn), arg = std::move(arg)]() -> bool {
            if (arg.wait_for(std::chrono::milliseconds(50)) == std::future_status::ready) {
                fn(arg.get());
                return false;
            }
            return true;
        });
    }

    /**
     * \brief Schedule job (queue when the std::future is ready).
     * \warning Jobs cannot be scheduled with a PoolWorker of 0 threads!
     * \param fn Callable of the job to do.
     * \param args Future arguments to apply to the callable.
     */
    template<
        typename Function,
        typename Args
    >
    auto scheduleWithApply(Function&& fn, std::shared_future<Args> args) -> void {
        /**
         * Add a self-rescheduling job that yields if the future isn't ready.
         * Once the future is ready, execute the job and don't reschedule.
         */
        addJob([fn = std::forward<decltype(fn)>(fn), args = std::move(args)]() -> bool {
            if (args.wait_for(std::chrono::milliseconds(50)) == std::future_status::ready) {
                std::apply(fn, args.get());
                return false;
            }
            return true;
        });
    }

private:
    /// \brief Unit of scheduled job for pool worker.
    using Job = std::function<bool()>;

    /**
     * \brief Add a Job to the queue of jobs to process.
     * \param Job Job to queue.
     */
    void addJob(Job&& Job);

    std::atomic_bool m_terminated;
    std::vector<std::thread> m_workers;

    std::mutex m_mutex;
    std::queue<Job> m_jobs;
    std::condition_variable m_cv;
} ;

}

void fty_common_messagebus_pool_worker_test (bool verbose);

#endif
