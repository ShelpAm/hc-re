#include <chrono>
#include <filesystem>
#include <hc/time-defs.h>
#include <print>

namespace fs = std::filesystem;

int main()
{
    // auto dur =
    // parse_time<std::chrono::time_point<std::chrono::system_clock>>(
    //     "%Y-%m-%d %H:%M:%S", "2025-11-27 14:32:02");
    // std::println("{}",
    // std::chrono::time_point<std::chrono::system_clock>{dur});

    // auto now = TimePoint{UtcClock::now().time_since_epoch()};
    //
    // std::println("{}", now);

    // fs::path p("path/to/some/file.etx");
    // std::println("p.string(): {}", p.string());
    // std::println("p.stem().string(): {}", p.stem().string());
    // std::println("p.filename().string(): {}", p.filename().string());

    fs::path abs{"/123"};
    fs::path absabs{"/123" / abs};
    std::println("abs: {}", abs.string());
    std::println("absabs: {}", absabs.string());
}
