#include <nlohmann/json.hpp>
#include <string>

struct AdminLoginParams {
    std::string username;
    std::string password;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AdminLoginParams, username, password);
};

struct AdminLoginResult {
    std::string token;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AdminLoginResult, token);
};

struct AdminVerifyTokenParams {
    std::string token;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AdminVerifyTokenParams, token);
};

struct AdminVerifyTokenResult {
    bool ok;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AdminVerifyTokenResult, ok);
};
