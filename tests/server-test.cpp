#include <gtest/gtest.h>
#include <httplib.h>
#include <libhc/server.h>
#include <nlohmann/json.hpp>
#include <thread>

class ServerTest : public testing::Test {
  protected:
    ServerTest() : c_("localhost", 8080), s_(make_config())
    {
        s_.start("localhost", 8080);
    }

    // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    httplib::Client c_;

  private:
    static sqlpp::postgresql::connection_config make_config()
    {
        auto config = sqlpp::postgresql::connection_config{};
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
        "name": "",
        "start_time_": "2025-11-26"
    })";
    auto r = c_.Post("/api/assignments/add", body, "application/json");
    EXPECT_TRUE(r);
    EXPECT_EQ(r->status, httplib::StatusCode::OK_200);
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
