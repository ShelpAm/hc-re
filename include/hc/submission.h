#pragma once
#include "time-defs.h"
#include <filesystem>

// Before uploading to database, See time-related issues mentioned in
// assignment.h .

struct Submission {
    std::string assignment_name;
    std::string student_id;
    TimePoint submission_time;
    std::filesystem::path filepath;
    std::string original_filename;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Submission, assignment_name, student_id,
                                   submission_time, filepath,
                                   original_filename);
};
