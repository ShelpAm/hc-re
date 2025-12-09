#include <cassert>
#include <httplib.h>
#include <print>
#include <thread>
#include <vector>

using namespace httplib;
using namespace std::chrono;
using namespace std::chrono_literals;

int main()
{
    constexpr auto num_threads = 12UZ;
    constexpr auto total_tasks = 1'000;

    std::atomic_int_least64_t remaining_tasks{total_tasks};
    auto task = [&]() {
        Client client("localhost", 8080);
        client.set_keep_alive(true);
        client.set_max_timeout(200ms);
        while (remaining_tasks-- > 0) {
            auto const res = client.Get("/api/assignments");
            assert(res && res->status == StatusCode::OK_200);
            std::println("{}", remaining_tasks.load());
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
        auto const end = steady_clock::now();
        std::println("{} tasks in {} ms", total_tasks,
                     duration_cast<milliseconds>(end - start).count());
    }
}
