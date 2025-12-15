#pragma once
#include <cassert>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace xdg {

inline std::optional<std::string> env(char const *variable)
{
    char *p = std::getenv(variable);
    return p != nullptr ? std::optional{p} : std::nullopt;
}

namespace fs = std::filesystem;

inline fs::path home()
{
    auto homedir = env("HOME");
    assert(homedir.has_value());
    return homedir.value();
}

inline fs::path config_home()
{
    auto dir = env("XDG_CONFIG_HOME");
    return dir.has_value() ? fs::path{dir.value()} : home() / ".config";
}

inline fs::path data_home()
{
    auto dir = env("XDG_DATA_HOME");
    return dir.has_value() ? fs::path{dir.value()}
                           : home() / ".local" / "share";
}

} // namespace xdg
