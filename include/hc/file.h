#pragma once
#include <nlohmann/json.hpp>

struct File {
    std::string filename;
    std::string content;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(File, filename, content);
};
