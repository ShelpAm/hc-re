#pragma once
#include <libhc/file.h>

struct SubmitRequest {
    std::string student_id;
    std::string assignment_name;
    File file;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SubmitRequest, student_id, assignment_name,
                                   file);
};
