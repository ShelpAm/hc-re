#include <hc/mock/mock-client.h>

#include <gtest/gtest.h>

using namespace httplib;

namespace hc::mock {

void successfully_hi(Client &client)
{
    auto r = client.Get("/hi");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, StatusCode::OK_200);
}
void hi_unreachable(Client &client)
{
    auto r = client.Get("/hi");
    EXPECT_TRUE(!r);
}

void successfully_add_assignment_testassignmentinfinite(Client &client)
{
    auto const *const body = R"({
            "name": "Test Assignment Infinite",
            "start_time": "2025-11-26T00:00:00Z",
            "end_time": "2099-11-26T00:00:00Z",
            "submissions": {}
        })";
    auto r = client.Post("/api/assignments/add", body, "application/json");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, StatusCode::OK_200);
}

void successfully_add_student_ljf(Client &client)
{
    auto const *const body = R"({
            "student_id": "202326202022",
            "name": "刘家福"
        })";
    auto r = client.Post("/api/students/add", body, "application/json");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, StatusCode::OK_200);
}

void ljf_successfully_submit_to_testassignmentinfinite(Client &client)
{
    auto const *const normal_body = R"({
            "student_id": "202326202022",
            "student_name": "刘家福",
            "assignment_name": "Test Assignment Infinite",
            "file": {
                "filename": "ljf sb",
                "content": "U0IgTEpG"
            }
        })";
    auto const r =
        client.Post("/api/assignments/submit", normal_body, "application/json");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, StatusCode::OK_200);
}

} // namespace hc::mock
