#include <gtest/gtest.h>
#include <libhc/optional.h>

TEST(OptionalTest, Basic)
{
    auto t = nlohmann::json().get<std::optional<int>>();
    EXPECT_TRUE(!t.has_value());

    t = nlohmann::json(2).get<std::optional<int>>();
    EXPECT_TRUE(t.has_value());

    auto j = nlohmann::json("2");
    EXPECT_THROW(j.get<std::optional<int>>(), nlohmann::json::type_error);

    j = 2;
    EXPECT_TRUE(!j.is_null());
    EXPECT_EQ(j.dump(), "2");

    j = std::optional{2};
    EXPECT_TRUE(!j.is_null());
    EXPECT_EQ(j.dump(), "2");
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
