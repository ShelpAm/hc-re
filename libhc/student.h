#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <utility>

struct Student {
    std::string student_id;
    std::string name;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Student, student_id, name);
};
