/*  =========================================================================
    fty_common_messagebus_pool_worker - class description

    Copyright (C) 2014 - 2019 Eaton

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
    /// \brief Unit of work for pool worker.
    using Work = std::function<void()>;

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
     * \brief Schedule work.
     * \param work Callable of the work to do.
     */
    void operator()(Work&& work);

private:
    std::atomic_bool m_terminated;
    std::vector<std::thread> m_workers;

    std::mutex m_mutex;
    std::queue<Work> m_jobs;
    std::condition_variable m_cv;
} ;

}

void fty_common_messagebus_pool_worker_test (bool verbose);

#endif
