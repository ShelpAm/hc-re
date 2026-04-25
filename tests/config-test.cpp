#include <filesystem>
#include <queue>

#include <gtest/gtest.h>
#include <hc/config.h>

namespace fs = std::filesystem;

TEST(ConfigTest, Basic)
{
    auto tmpfs = fs::temp_directory_path() / "hc";
    auto cfg_path = tmpfs / "config.yaml";

    fs::create_directories(tmpfs);
    std::ofstream(cfg_path) << R"(# HC-RE default configuration
# All fields are optional and will fall back to their default values

port: 8080  # uint16 - HTTP server port (0-65535)

db:
  host: "localhost"      # string - PostgreSQL server hostname or IP
  name: "hc"             # string - Database name
  user: "postgres"       # string - Database username
  password: ""           # string - Database password
)";

    auto cfg = Config::load_from_file(cfg_path);
    ASSERT_EQ(cfg.port, 8080);
    ASSERT_EQ(cfg.db.host, "localhost");
    ASSERT_EQ(cfg.db.name, "hc");
    ASSERT_EQ(cfg.db.user, "postgres");
    ASSERT_EQ(cfg.db.password, "");
}

// Missing fields should fall back to their default values instead of throwing
// an error
TEST(ConfigTest, MissingFields)
{
    auto tmpfs = fs::temp_directory_path() / "hc";
    auto cfg_path = tmpfs / "config.yaml";

    fs::create_directories(tmpfs);
    std::ofstream(cfg_path) << R"(# HC-RE default configuration
# All fields are optional and will fall back to their default values

db:
  host: "localhost"      # string - PostgreSQL server hostname or IP
  user: "postgres"       # string - Database username
  password: ""           # string - Database password
)";

    auto cfg = Config::load_from_file(cfg_path);
    ASSERT_EQ(cfg.port, 8080);
    ASSERT_EQ(cfg.db.name, "hc");
}

TEST(ConfigTest, ExtraneousFields)
{
    auto tmpfs = fs::temp_directory_path() / "hc";
    auto cfg_path = tmpfs / "config.yaml";

    fs::create_directories(tmpfs);
    std::ofstream(cfg_path) << R"(
whatisthis:
  - An array
andThis:
  A
  struct
    )";

    auto cfg = Config::load_from_file(cfg_path);
    // Original keeps untouched
    ASSERT_EQ(cfg.port, 8080);
    ASSERT_EQ(cfg.db.host, "localhost");
    ASSERT_EQ(cfg.db.name, "hc");
    ASSERT_EQ(cfg.db.user, "postgres");
    ASSERT_EQ(cfg.db.password, "");
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
