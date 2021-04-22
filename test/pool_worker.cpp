#include "fty_common_messagebus_pool_worker.h"
#include <catch2/catch.hpp>

#include <iostream>
#include <set>
#include <numeric>

uint64_t collatz(uint64_t i) {
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

uint64_t summation(std::vector<uint64_t> data) {
    return std::accumulate(data.begin(), data.end(), 0);
}


TEST_CASE("Pool worker")
{
    bool verbose = true;

    std::cerr << " * fty_common_messagebus_pool_worker: " << std::endl;
    using namespace messagebus;
    constexpr size_t NB_WORKERS = 16;
    constexpr size_t NB_JOBS    = 8 * 1024;

    {
        for (size_t nWorkers = 0; nWorkers < NB_WORKERS; nWorkers = nWorkers * 2 + 1) {
            std::cerr << "  - Array initialization with PoolWorker(" << nWorkers << "): ";

            std::vector<std::atomic_uint_fast32_t> results(NB_JOBS);
            {
                PoolWorker pool(nWorkers);
                for (size_t i = 0; i < NB_JOBS; i++) {
                    pool.offload(
                        [&results](size_t i) {
                            results[i].store(i);
                        },
                        i);
                }
            }

            for (size_t i = 0; i < NB_JOBS; i++) {
                REQUIRE(results[i].load() == i);
            }

            std::cerr << "OK" << std::endl;
        }
    }

    {
        std::array<uint64_t, NB_JOBS> collatzExpectedResults;
        for (size_t i = 0; i < NB_JOBS; i++) {
            collatzExpectedResults[i] = collatz(i);
        }

        for (size_t nWorkers = 0; nWorkers < NB_WORKERS; nWorkers = nWorkers * 2 + 1) {
            std::cerr << "  - Collatz sequence with PoolWorker(" << nWorkers << "): ";

            PoolWorker                                 pool(nWorkers);
            std::array<std::future<uint64_t>, NB_JOBS> futuresArray;
            for (uint64_t i = 0; i < NB_JOBS; i++) {
                futuresArray[i] = pool.schedule(collatz, i);
            }

            for (size_t i = 0; i < NB_JOBS; i++) {
                REQUIRE(futuresArray[i].get() == collatzExpectedResults[i]);
            }

            std::cerr << "OK" << std::endl;
        }
    }

    {
        std::array<uint64_t, NB_JOBS> sumExpectedResults;
        for (size_t i = 0; i < NB_JOBS; i++) {
            sumExpectedResults[i] = i * (i + 1) / 2;
        }

        for (size_t nWorkers = 0; nWorkers < NB_WORKERS; nWorkers = nWorkers * 2 + 1) {
            std::cerr << "  - Summation with PoolWorker(" << nWorkers << "): ";

            PoolWorker                                 pool(nWorkers);
            std::array<std::future<uint64_t>, NB_JOBS> futuresArray;
            for (uint64_t i = 0; i < NB_JOBS; i++) {
                std::vector<uint64_t> terms(i);
                for (uint64_t j = 0; j < i; j++) {
                    terms[j] = j + 1;
                }

                futuresArray[i] = pool.schedule(summation, std::move(terms));
            }

            for (size_t i = 0; i < NB_JOBS; i++) {
                auto a = futuresArray[i].get();
                REQUIRE(a == sumExpectedResults[i]);
            }

            std::cerr << "OK" << std::endl;
        }
    }
}
