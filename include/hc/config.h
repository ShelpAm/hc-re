#pragma once
#include <hc/xdg-basedir.h>
#include <rfl/yaml.hpp>

struct Config {
    rfl::DefaultVal<bool> show_version{};
    rfl::DefaultVal<bool> verbose{};
    rfl::DefaultVal<bool> ask{}; // ask for db user and password
    rfl::DefaultVal<std::uint16_t> port{8080};

    struct DatabaseConfig {
        rfl::DefaultVal<std::string> host{"localhost"};
        rfl::DefaultVal<std::string> name{"hc"};
        rfl::DefaultVal<std::string> user{"postgres"};
        rfl::DefaultVal<std::string> password{};
    };
    rfl::DefaultVal<DatabaseConfig> db;

    rfl::DefaultVal<std::filesystem::path> data_dir{xdg::data_home() / "hc"};
    rfl::DefaultVal<std::filesystem::path> cache_dir{xdg::cache_home() / "hc"};
};

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

} // namespace config
