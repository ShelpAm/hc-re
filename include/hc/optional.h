#pragma once
#include <nlohmann/json.hpp>
#include <optional>

namespace nlohmann {
template <typename T> struct adl_serializer<std::optional<T>> {
    static void to_json(json &j, std::optional<T> const &opt)
    {
        if (opt.has_value()) {
            j = opt.value();
        }
        else {
            j = nullptr;
        }
    }

    static void from_json(json const &j, std::optional<T> &opt)
    {
        if (j.is_null()) {
            opt = std::nullopt;
        }
        else {
            opt = j.get<T>();
            // j.get_to(opt); // This won't throw, instead it triggers
            // segmentation fault.
        }
    }
};
} // namespace nlohmann
