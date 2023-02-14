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

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
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
     * \param workers Number of workers (work will be processed synchronously if 0).
     */
    PoolWorker(size_t workers);

    // PoolWorker can't be copied, assigned or moved.
    PoolWorker() = delete;
    PoolWorker(const PoolWorker&) = delete;
    PoolWorker(PoolWorker&&) = delete;
    PoolWorker& operator=(const PoolWorker&) = delete;
    void operator=(PoolWorker&&) = delete;

    /**
     * \brief Destroy the pool of workers.
     *
     * Once the destructor is called, PoolWorker will not schedule further work
     * and will wait until all scheduled work is completed before returning.
     */
    ~PoolWorker();

    /**
     * \brief Schedule work (keep a std::future for the result).
     * \param work Callable of the work to do.
     * \param args Arguments to pass to the callable.
     * \return A future of the return value of the callable.
     */
    template<
        typename Function,
        typename... Args,
        typename ReturnType = decltype(std::declval<Function&&>()(std::declval<Args&&>()...))
    >
    auto schedule(Function&& fn, Args&&... args) -> std::future<ReturnType> {
        using PackagedTask = std::packaged_task<ReturnType()>;

        // Package the work into a storable form.
        auto packagedTask = std::make_shared<PackagedTask>(std::bind(std::forward<Function&&>(fn), std::forward<Args&&>(args)...));

        this->scheduleWork(std::move([packagedTask]() { (*packagedTask)(); }));
        return packagedTask->get_future();
    }

    /**
     * \brief Offload work (do not keep a std::future for the result).
     * \param work Callable of the work to do.
     * \param args Arguments to pass to the callable.
     */
    template<
        typename Function,
        typename... Args
    >
    auto offload(Function&& fn, Args&&... args) -> void {
        // Package the work into a storable form.
        WorkUnit packagedTask = std::bind(std::forward<Function&&>(fn), std::forward<Args&&>(args)...);

        this->scheduleWork(std::move(packagedTask));
    }

private:
    /// \brief Unit of work for pool worker.
    using WorkUnit = std::function<void()>;
    void scheduleWork(WorkUnit&& work);

    std::atomic_bool m_terminated;
    std::vector<std::thread> m_workers;

    std::mutex m_mutex;
    std::queue<WorkUnit> m_jobs;
    std::condition_variable m_cv;
} ;

}
