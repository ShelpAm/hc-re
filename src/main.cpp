#include <CLI/CLI.hpp>
#include <httplib.h>
#include <libhc/config.h>
#include <libhc/server.h>
#include <libhc/xdg-basedir.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <sqlpp23/postgresql/postgresql.h>
#include <sqlpp23/sqlpp23.h>

int main(int argc, char **argv)
{
    using config::datahome;
    using config::verbose;

    CLI::App app("homework-collection-remastered", "hc");
    app.add_flag("-v,--verbose", verbose());
    CLI11_PARSE(app, argc, argv);

    spdlog::set_level(config::verbose() ? spdlog::level::debug
                                        : spdlog::level::info);
    spdlog::debug("datahome={}", datahome().string());
    spdlog::debug("verbose={}", verbose());

    // Create a connection configuration.
    auto config = sqlpp::postgresql::connection_config{};
    config.host = "localhost";
    config.dbname = "hc";
    config.user = "postgres";

    Server server(config);
    server.start("localhost", 8080);

    // Blocks until something is triggered.
    while (true) { // TODO: something here.
    }
}
