#include <gtest/gtest.h>
#include <httplib.h>
#include <libhc/server.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <thread>

class ServerTest : public testing::Test {
  protected:
    ServerTest() : c_("localhost", 10010), s_(make_config())
    {
        s_.start("localhost", 10010);
    }

    // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    httplib::Client c_;

  private:
    static sqlpp::postgresql::connection_config make_config()
    {
        auto config = sqlpp::postgresql::connection_config{};
        // Under Windows this breaks. We should write specialized code for
        // different platform. And we may need to create test db instead of
        // using production db.
        config.host = "localhost";
        config.dbname = "hc";
        config.user = "postgres";
        return config;
    }

    Server s_;
};

TEST_F(ServerTest, ConnectivityTest)
{
    auto r = c_.Get("/hi");
    EXPECT_TRUE(r);
    EXPECT_EQ(r->body, "Hello World!");
}

TEST_F(ServerTest, WrongPath)
{
    auto r = c_.Get("/api/assignments/");
    EXPECT_TRUE(r);
    EXPECT_EQ(r->status, httplib::StatusCode::NotFound_404);
}

TEST_F(ServerTest, Valid)
{
    auto r = c_.Get("/api/assignments");
    EXPECT_TRUE(r);
    EXPECT_EQ(r->status, httplib::StatusCode::OK_200);
}

TEST_F(ServerTest, AddAssignment)
{
    auto const *const body = R"({
        "name": "Test Assignment",
        "start_time": "2025-11-26T00:00:00Z",
        "end_time": "2025-11-26T00:00:00Z",
        "submissions": {}
    })";
    auto r = c_.Post("/api/assignments/add", body, "application/json");
    EXPECT_TRUE(r);
    // EXPECT_EQ(r->status, httplib::StatusCode::OK_200);
}

TEST_F(ServerTest, AddStudent)
{
    auto const *const body = R"({
        "student_id": "202326202022",
        "name": "刘家福"
    })";
    auto r = c_.Post("/api/students/add", body, "application/json");
    EXPECT_TRUE(r);
    // EXPECT_EQ(r->status, httplib::StatusCode::OK_200);
}

TEST_F(ServerTest, SubmitToAssignment)
{
    auto const *const empty_body = R"()";
    auto r = c_.Post("/api/assignments/submit", empty_body, "application/json");
    EXPECT_TRUE(r);
    EXPECT_EQ(r->status, httplib::StatusCode::InternalServerError_500);

    auto const *const normal_body = R"({
        "student_id": "202326202022",
        "student_name": "刘家福",
        "assignment_name": "Test Assignment",
        "file": {
            "filename": "ljf sb",
            "content": "U0IgTEpG"
        }
    })";
    r = c_.Post("/api/assignments/submit", normal_body, "application/json");
    EXPECT_TRUE(r);
    EXPECT_EQ(r->status, httplib::StatusCode::OK_200);
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    spdlog::set_level(spdlog::level::debug);
    return RUN_ALL_TESTS();
}
