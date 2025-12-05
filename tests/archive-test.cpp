#include <fstream>
#include <gtest/gtest.h>
#include <hc/archive.h>
#include <spdlog/spdlog.h>
#include <string>
#include <text_encoding>

namespace fs = std::filesystem;
using namespace std::string_literals;

TEST(ArchiveTest, Basic)
{
    auto const wd = fs::temp_directory_path() / "hc" / "test basic";
    fs::create_directories(wd);
    fs::current_path(wd);
    auto const simple_file = u8"simple file"s;
    auto const simple_dir = u8"simple dir"s;
    auto const non_ascii_file = u8"中文文件"s;
    auto const non_ascii_dir = u8"中文目录"s;
    std::ofstream ofs(fs::path{simple_file});
    std::ofstream ofs2(fs::path{non_ascii_file});
    fs::create_directories(simple_dir);
    fs::create_directories(non_ascii_dir);

    hc::archive::ArchiveWriter w("basic.tar.zst",
                                 ARCHIVE_FORMAT_TAR_PAX_RESTRICTED,
                                 {ARCHIVE_FILTER_ZSTD});
    w.imbue(std::locale("en_US.UTF-8"));
    EXPECT_NO_THROW(w.add_path(simple_file, simple_file));
    EXPECT_NO_THROW(w.add_path(simple_dir, simple_dir));
    EXPECT_NO_THROW(w.add_path(non_ascii_file, non_ascii_file));
    EXPECT_NO_THROW(w.add_path(non_ascii_dir, non_ascii_dir));

    fs::remove_all(wd);
}

int main(int argc, char **argv)
{
    spdlog::set_level(spdlog::level::debug);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
