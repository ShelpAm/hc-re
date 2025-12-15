#pragma once
#include <hc/xdg-basedir.h>

namespace config {

inline std::filesystem::path const &datahome()
{
    static auto const datahome = xdg::data_home() / "hc";
    return datahome;
}

inline bool &verbose()
{
    static auto verbose = false;
    return verbose;
}

} // namespace config
