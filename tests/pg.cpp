#include <chrono>
#include <libhc/time-defs.h>
#include <print>

int main()
{
    auto dur = parse_time<std::chrono::time_point<std::chrono::system_clock>>(
        "%Y-%m-%d %H:%M:%S", "2025-11-27 14:32:02");
    // std::println("{}",
    // std::chrono::time_point<std::chrono::system_clock>{dur});
    std::println("{}", dur);
}
