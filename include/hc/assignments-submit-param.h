#pragma once
#include <hc/file.h>

struct AssignmentsSubmitParams {
    std::string student_id;
    std::string student_name;
    std::string assignment_name;
    File file;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AssignmentsSubmitParams, student_id,
                                   student_name, assignment_name, file);
};
