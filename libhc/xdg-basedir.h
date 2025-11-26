#pragma once
#include <filesystem>
#include <string>
#include <string_view>

namespace xdg {

std::string env(char const *variable)
{
    char *p = std::getenv(variable);
    return p != nullptr ? p : std::string{};
}

namespace fs = std::filesystem;

fs::path config_home()
{
    auto dir = env("XDG_CONFIG_HOME");
    return dir.empty() ? fs::path{env("HOME")} / ".config" : fs::path{dir};
}

fs::path data_home()
{
    auto dir = env("XDG_DATA_HOME");
    return dir.empty() ? fs::path{env("HOME")} / ".config" / "share"
                       : fs::path{dir};
}

} // namespace xdg
