#include <fstream>
#include <gtest/gtest.h>
#include <hc/server.h>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>
#include <sstream>
#include <thread>

#ifndef HCRE_TEST_DB
#error [dev] HCHCRE_TEST_DB not defined, should be defined in CMakeLists.txt
#endif

namespace {

void successfully_add_assignment_testassignmentinfinite(httplib::Client &client)
{
    auto const *const body = R"({
            "name": "Test Assignment Infinite",
            "start_time": "2025-11-26T00:00:00Z",
            "end_time": "2099-11-26T00:00:00Z",
            "submissions": {}
        })";
    auto r = client.Post("/api/assignments/add", body, "application/json");
    EXPECT_TRUE(r);
    EXPECT_EQ(r->status, httplib::StatusCode::OK_200);
}

void successfully_add_student_ljf(httplib::Client &client)
{
    auto const *const body = R"({
            "student_id": "202326202022",
            "name": "刘家福"
        })";
    auto r = client.Post("/api/students/add", body, "application/json");
    EXPECT_TRUE(r);
    EXPECT_EQ(r->status, httplib::StatusCode::OK_200);
}

void ljf_successfully_submit_to_testassignmentinfinite(httplib::Client &client)
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
    EXPECT_TRUE(r);
    EXPECT_EQ(r->status, httplib::StatusCode::OK_200);
}

} // namespace

class TestDB {
  public:
    TestDB() : connection_(HCRE_TEST_DB)
    {
        pqxx::work tx(connection_);
        tx.exec(file_content("scripts/table.sql"));
        tx.commit();
    }
    TestDB(TestDB const &) = delete;
    TestDB(TestDB &&) = delete;
    TestDB &operator=(TestDB const &) = delete;
    TestDB &operator=(TestDB &&) = delete;
    ~TestDB()
    {
        pqxx::work tx(connection_);
        tx.exec(file_content("scripts/clean.sql"));
        tx.commit();
    }

    [[nodiscard]] pqxx::connection &connection()
    {
        return connection_;
    }

  private:
    static std::string file_content(std::string const &filename)
    {
        std::ifstream ifs(filename);
        if (!ifs.is_open()) {
            throw std::runtime_error{
                std::format("cannot access '{}': No such file", filename)};
        }
        std::stringstream ss;
        ss << ifs.rdbuf();
        return ss.str();
    }

    pqxx::connection connection_;
};

class ServerTest : public testing::Test {
  protected:
    ServerTest() : c_("localhost", 10010), s_(make_config())
    {
        s_.start("localhost", 10010);
    }

    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    httplib::Client c_;
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

  private:
    sqlpp::postgresql::connection_config make_config()
    {
        auto config = sqlpp::postgresql::connection_config{};
        // Under Windows this breaks. We should write specialized code for
        // different platform. And we may need to create test db instead of
        // using production db.
        config.host = testdb_.connection().hostname();
        config.dbname = testdb_.connection().dbname();
        config.user = testdb_.connection().username();
        return config;
    }

    // Should not change the order because initialization of s_ depends on
    // testdb_.
    TestDB testdb_;
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
    EXPECT_EQ(r->status, httplib::StatusCode::OK_200);
    r = c_.Post("/api/assignments/add", body, "application/json");
    EXPECT_TRUE(r);
    EXPECT_EQ(r->status, httplib::StatusCode::BadRequest_400); // Already exists
}

TEST_F(ServerTest, AddStudent)
{
    auto const *const body = R"({
        "student_id": "202326202022",
        "name": "刘家福"
    })";
    auto r = c_.Post("/api/students/add", body, "application/json");
    EXPECT_TRUE(r);
    EXPECT_EQ(r->status, httplib::StatusCode::OK_200);
}

TEST_F(ServerTest, SubmitToAssignment)
{
    successfully_add_assignment_testassignmentinfinite(c_);

    {
        auto const *const empty_body = R"()";
        auto r =
            c_.Post("/api/assignments/submit", empty_body, "application/json");
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
        // Time out of bound
        EXPECT_EQ(r->status, httplib::StatusCode::BadRequest_400);
    }

    {
        auto const *const empty_body = R"()";
        auto r =
            c_.Post("/api/assignments/submit", empty_body, "application/json");
        EXPECT_TRUE(r);
        EXPECT_EQ(r->status, httplib::StatusCode::InternalServerError_500);

        auto const *const normal_body = R"({
            "student_id": "202326202022",
            "student_name": "刘家福",
            "assignment_name": "Test Assignment Infinite",
            "file": {
                "filename": "ljf sb",
                "content": "U0IgTEpG"
            }
        })";
        r = c_.Post("/api/assignments/submit", normal_body, "application/json");
        EXPECT_TRUE(r);
        // No such student
        EXPECT_EQ(r->status, httplib::StatusCode::BadRequest_400);
    }

    successfully_add_student_ljf(c_);

    {
        auto const *const empty_body = R"()";
        auto r =
            c_.Post("/api/assignments/submit", empty_body, "application/json");
        EXPECT_TRUE(r);
        EXPECT_EQ(r->status, httplib::StatusCode::InternalServerError_500);
    }

    ljf_successfully_submit_to_testassignmentinfinite(c_);
}

TEST_F(ServerTest, Export)
{
    successfully_add_assignment_testassignmentinfinite(c_);
    successfully_add_student_ljf(c_);
    ljf_successfully_submit_to_testassignmentinfinite(c_);

    {
        auto const *const empty_body = R"()";
        auto r =
            c_.Post("/api/assignments/export", empty_body, "application/json");
        EXPECT_TRUE(r);
        EXPECT_EQ(r->status, httplib::StatusCode::BadRequest_400);
    }

    {
        auto const *const body = R"({
            "assignment_name": "Test Assignment Infinite"
        })";
        auto r = c_.Post("/api/assignments/export", body, "application/json");
        EXPECT_TRUE(r);
        EXPECT_EQ(r->status, httplib::StatusCode::OK_200);
    }
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    spdlog::set_level(spdlog::level::debug);
    return RUN_ALL_TESTS();
}
