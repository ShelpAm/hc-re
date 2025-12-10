#include <cassert>
#include <hc/mock/mock-client.h>
#include <httplib.h>
#include <print>
#include <thread>
#include <vector>

using namespace httplib;
using namespace std::chrono;
using namespace std::chrono_literals;

int main()
{
    constexpr auto total_threads = 6UZ;
    constexpr auto total_tasks = 10000Z;

    // for (auto num_threads = 1UZ; num_threads != total_threads + 1;
    //      ++num_threads) {
    constexpr auto num_threads = total_threads;
    std::atomic_int_least64_t remaining_tasks{total_tasks};
    auto task = [&]() {
        Client client("localhost", 8080);
        client.set_keep_alive(false);
        client.set_max_timeout(10s);
        while (true) {
            auto old = remaining_tasks.fetch_sub(1, std::memory_order_relaxed);
            if (old <= 0)
                break;

            hc::mock::successfully_hi(client);
            auto const res = client.Get("/api/assignments");
            assert(res && res->status == StatusCode::OK_200);
            // if ((old - 1) % 100 == 0)
            //     std::println("executing task: {}", old - 1);
        }
    };

    {
        std::vector<std::jthread> threads;
        threads.reserve(num_threads);

        auto const start = steady_clock::now();
        for (auto i = 0UZ; i != num_threads; ++i) {
            threads.push_back(std::jthread(task));
        }
        for (auto &t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }
        auto s = duration<double>(steady_clock::now() - start);
        // std::println("{} tasks in {} ms", total_tasks, ms.count());
        std::println("Threads: {:3}, Rate: {:4} tasks/s", num_threads,
                     static_cast<double>(total_tasks) / s.count());
    }
    // }
}
