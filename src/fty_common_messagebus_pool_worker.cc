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
            m_cv.wait(lk, [this]() -> bool { return m_terminated.load() || !m_jobs.empty(); });

            while (!m_jobs.empty()) {
                auto job = std::move(m_jobs.front());
                m_jobs.pop();
                lk.unlock();

                auto shouldReschedule = job();

                lk.lock();
                if (shouldReschedule) {
                    m_jobs.emplace(std::move(job));
                    m_cv.notify_one();
                }
            }

            if (m_terminated.load()) {
                break;
            }
        }
    } ;

    m_workers.reserve(workers);
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

void PoolWorker::addJob(Job&& work) {
    std::unique_lock<std::mutex> lk(m_mutex);

    if (m_workers.empty()) {
        // No workers, run work unit synchronously.
        work();
    }
    else {
        // Got workers, schedule.
        m_jobs.emplace(work);
        m_cv.notify_one();
    }
}

}

//  --------------------------------------------------------------------------
//  Self test of this class

// If your selftest reads SCMed fixture data, please keep it in
// src/selftest-ro; if your test creates filesystem objects, please
// do so under src/selftest-rw.
// The following pattern is suggested for C selftest code:
//    char *filename = NULL;
//    filename = zsys_sprintf ("%s/%s", SELFTEST_DIR_RO, "mytemplate.file");
//    assert (filename);
//    ... use the "filename" for I/O ...
//    zstr_free (&filename);
// This way the same "filename" variable can be reused for many subtests.
#define SELFTEST_DIR_RO "src/selftest-ro"
#define SELFTEST_DIR_RW "src/selftest-rw"

#include <cassert>
#include <iostream>
#include <set>
#include <numeric>

static uint64_t collatz(uint64_t i) {
    uint64_t n;
    for (n = 0; i > 1; n++) {
        if (i%2) {
            i = 3*i+1;
        }
        else {
            i = i/2;
        }
    }
    return n;
}

void fty_common_messagebus_pool_worker_test(bool verbose)
{
    std::cerr << " * fty_common_messagebus_pool_worker: " << std::endl;
    using namespace messagebus;
    constexpr size_t NB_WORKERS = 16;
    constexpr size_t NB_JOBS = 8*1024;

    // Job offloading test.
    {
        for (size_t nWorkers = 0; nWorkers < NB_WORKERS; nWorkers = nWorkers*2 + 1) {
            std::cerr << "  - Array initialization with PoolWorker(" << nWorkers << "): ";

            std::vector<std::atomic_uint_fast32_t> results(NB_JOBS);
            {
                PoolWorker pool(nWorkers);
                for (size_t i = 0; i < NB_JOBS; i++) {
                    pool.offload([&results](size_t i) { results[i].store(i); }, i);
                }
            }

            for (size_t i = 0; i < NB_JOBS; i++) {
                assert(results[i].load() == i);
            }

            std::cerr << "OK" << std::endl;
        }
    }

    // Job queueing test.
    {
        std::array<uint64_t, NB_JOBS> collatzExpectedResults;
        for (size_t i = 0; i < NB_JOBS; i++) {
            collatzExpectedResults[i] = collatz(i);
        }

        for (size_t nWorkers = 0; nWorkers < NB_WORKERS; nWorkers = nWorkers*2 + 1) {
            std::cerr << "  - Collatz sequence with PoolWorker(" << nWorkers << "): ";

            PoolWorker pool(nWorkers);
            std::array<std::future<uint64_t>, NB_JOBS> futuresArray;
            for (uint64_t i = 0; i < NB_JOBS; i++) {
                futuresArray[i] = pool.queue(collatz, i);
            }

            for (size_t i = 0; i < NB_JOBS; i++) {
                assert(futuresArray[i].get() == collatzExpectedResults[i]);
            }

            std::cerr << "OK" << std::endl;
        }
    }

    // Job scheduling test.
    {
        for (size_t nWorkers = 1; nWorkers < NB_WORKERS; nWorkers = nWorkers*2 + 1) {
            std::cerr << "  - Integer enumeration with PoolWorker(" << nWorkers << "): ";

            std::array<std::promise<uint64_t>, NB_JOBS> promisesArray;
            std::array<std::shared_future<uint64_t>, NB_JOBS> futuresArray;
            {
                PoolWorker pool(nWorkers);
                for (uint64_t i = 0; i < NB_JOBS; i++) {
                    futuresArray[i] = std::shared_future(promisesArray[i].get_future());

                    pool.schedule([&promisesArray](uint64_t value) {
                        uint64_t next_value_1 = value * 2;
                        uint64_t next_value_2 = value * 2 + 1;

                        if (next_value_1 <= NB_JOBS) {
                            promisesArray[next_value_1 - 1].set_value(next_value_1);
                        }
                        if (next_value_2 <= NB_JOBS) {
                            promisesArray[next_value_2 - 1].set_value(next_value_2);
                        }
                    }, futuresArray[i]);
                }
                promisesArray[0].set_value(1);
            }

            for (size_t i = 0; i < NB_JOBS; i++) {
                auto a = futuresArray[i].get();
                assert(a == i + 1);
            }

            std::cerr << "OK" << std::endl;
        }
    }

    // Job scheduling test with apply.
    {
        std::cerr << "  - Schedule with apply: ";
        std::atomic_int result;

        {
            PoolWorker pool(1);
            auto promise = std::promise<std::tuple<int, int>>();
            auto future = std::shared_future(promise.get_future());

            pool.scheduleWithApply([&result](int a, int b) {
                result = a + b;
            }, future);

            promise.set_value({2, 3});
        }

        assert(result.load() == 5);

        std::cerr << "OK" << std::endl;
    }
}
