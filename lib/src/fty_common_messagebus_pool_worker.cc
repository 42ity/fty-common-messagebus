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

/*
@header
    fty_common_messagebus_pool_worker -
@discuss
@end
*/

#include "fty_common_messagebus_classes.h"

namespace messagebus {

PoolWorker::PoolWorker(size_t workers) : m_terminated(false) {
    auto workerMainloop = [this]() {
        while (true) {
            std::unique_lock<std::mutex> lk(m_mutex);
            m_cv.wait(lk, [this]() -> bool { return m_terminated.load() || m_jobs.size(); });

            while (!m_jobs.empty()) {
                auto work = std::move(m_jobs.front());
                m_jobs.pop();
                lk.unlock();

                work();

                lk.lock();
            }

            if (m_terminated.load()) {
                break;
            }
        }
    } ;

    for (size_t cpt = 0; cpt < workers; cpt++) {
        m_workers.emplace_back(std::thread(workerMainloop));
    }
}

PoolWorker::~PoolWorker() {
    if (!m_workers.empty()) {
        {
            std::unique_lock<std::mutex> lk(m_mutex);
            m_terminated.store(true);
            m_cv.notify_all();
        }

        for (auto& th : m_workers) {
            th.join();
        }
    }
}

void PoolWorker::scheduleWork(WorkUnit&& work) {
    std::unique_lock<std::mutex> lk(m_mutex);
    if (m_terminated.load()) {
        throw std::runtime_error("PoolThread is terminated");
    }

    if (m_workers.empty()) {
        // No workers, run job synchronously.
        work();
    }
    else {
        // Got workers, schedule.
        m_jobs.emplace(work);
        m_cv.notify_one();
    }
}

}
