#pragma once
#include <nlohmann/json.hpp>
#include <string>

struct Teacher {
    std::string teacher_id; // 教师号
    std::string name;       // 教师名字
    std::string password;   // 明文存储（建议后续改为哈希）

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Teacher, teacher_id, name, password);
};