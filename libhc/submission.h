#pragma once
#include "time-defs.h"
#include <filesystem>

namespace fs = std::filesystem;

struct Submission {
    std::string student_name;
    TimePoint submission_time;
    fs::path filepath;
    std::string original_filename;
};
