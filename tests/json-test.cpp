#include <gtest/gtest.h>
#include <hc/student.h>
#include <nlohmann/json.hpp>

TEST(JsonTest, Student)
{
    auto const j = nlohmann::json::parse(
        R"({"name":"刘志远","student_id":"202326202001"})");
    auto const s = j.get<Student>();
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
