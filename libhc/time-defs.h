#pragma once
#include <chrono>
#include <format>
#include <nlohmann/json.hpp>

using UtcClock = std::chrono::utc_clock;
using SystemClock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<SystemClock>;

class ParseTimeException {};

// To understand how to use `fmt`, see section 'Format string' in page
// https://www.en.cppreference.com/w/cpp/chrono/parse.html
template <typename Result>
static Result parse_time(std::string const &fmt, std::string const &str)
{
    auto is = std::istringstream{str};
    is.imbue(std::locale("en_US.utf-8"));
    Result res{};
    is >> std::chrono::parse(fmt, res);
    if (is.fail()) {
        throw std::runtime_error{"Failed to parse time " +
                                 std::format("format: {}, str: {}", fmt, str)};
    }
    return res;
};

// Time in Json should follow ISO 8601.
namespace nlohmann {
template <> struct adl_serializer<TimePoint> {
    static constexpr std::string_view from_format{"%Y-%m-%dT%H:%M:%SZ"};
    static constexpr std::string_view to_format{"{:%Y-%m-%dT%H:%M:%SZ}"};

    static void to_json(json &j, TimePoint const &tp)
    {
        j = std::format(to_format, tp);
    }

    static void from_json(json const &j, TimePoint &tp)
    {
        tp = parse_time<TimePoint>(std::string{from_format},
                                   j.get<std::string>());
    }
};
} // namespace nlohmann
