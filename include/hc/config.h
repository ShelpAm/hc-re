#pragma once
#include <hc/xdg-basedir.h>
#include <ranges>
#include <rfl/yaml.hpp>
#include <spdlog/spdlog.h>

namespace config {

inline std::filesystem::path const &datahome()
{
    static auto const datahome = xdg::data_home() / "hc";
    return datahome;
}

inline std::filesystem::path const &cachehome()
{
    static auto const cachehome = xdg::cache_home() / "hc";
    return cachehome;
}

inline std::filesystem::path const &config_home()
{
    static auto const config_home = xdg::config_home() / "hc";
    return config_home;
}

} // namespace config

struct Config {
    static Config load_from_search_paths()
    {
        std::vector<std::filesystem::path> paths_to_try{
            config::config_home() / "config.yaml",
            config::config_home() / "config.yml",
        };
        auto it = std::ranges::find_if(paths_to_try, [](auto const &p) {
            spdlog::debug("Trying to find config file in {}", p.string());
            return std::filesystem::exists(p);
        });

        if (it == paths_to_try.end()) {
            spdlog::debug("Config file not found at {}, using default config",
                          paths_to_try |
                              std::views::transform(
                                  [](auto const &p) { return p.string(); }));
            return Config{};
        }

        spdlog::debug("Using config file at {}", it->string());
        std::ifstream ifs(*it);
        auto result = rfl::yaml::read<Config>(ifs);
        if (!result)
            throw std::runtime_error("Failed to parse config file: " +
                                     result.error().what());
        return result.value();
    }

    rfl::DefaultVal<std::uint16_t> port{8080};

    struct DatabaseConfig {
        rfl::DefaultVal<std::string> host{"localhost"};
        rfl::DefaultVal<std::string> name{"hc"};
        rfl::DefaultVal<std::string> user{"postgres"};
        rfl::DefaultVal<std::string> password{};
    };
    rfl::DefaultVal<DatabaseConfig> db;
};
