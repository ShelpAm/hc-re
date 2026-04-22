#include <CLI/CLI.hpp>
#include <hc/config.h>
#include <hc/server.h>
#include <hc/version.h>
#include <hc/xdg-basedir.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <print>
#include <spdlog/spdlog.h>
#include <sqlpp23/postgresql/postgresql.h>
#include <sqlpp23/sqlpp23.h>

int main(int argc, char **argv)
{
    try {
        CLI::App app("homework-collection-remastered", "hc");
        auto show_version = false;
        auto verbose = false;
        auto ask_for_db_info = false;
        std::uint32_t port{-1U};
        app.add_flag("-V,--version", show_version, "Print hc version and exit");
        app.add_flag("-v,--verbose", verbose, "Use debug mode");
        app.add_flag("--ask", ask_for_db_info,
                     "Ask username and password for database connection");
        app.add_option("-p,--port", port, "Port of the web server");
        CLI11_PARSE(app, argc, argv);

        spdlog::set_level(verbose ? spdlog::level::debug : spdlog::level::info);
        spdlog::debug("verbose={}", verbose);
        spdlog::debug("show_version={}", show_version);

        auto config = Config::load_from_search_paths();
        if (port != -1U) // Uses port from CLI.
            config.port = port;

        spdlog::debug("Config:---\n{}\n---", rfl::yaml::write(config));

        if (show_version) {
            std::println("hc version {}", HCRE_VERSION);
            return 0;
        }

        // Create a connection configuration.
        if (ask_for_db_info) {
            std::print("Input your db username: ");
            std::getline(std::cin, config.db.value().user.value());
            std::print("Input your db password: ");
            std::getline(std::cin, config.db.value().password.value());
        }

        auto dbconfig = sqlpp::postgresql::connection_config{};
        dbconfig.host = config.db.value().host.value();
        dbconfig.dbname = config.db.value().name.value();
        dbconfig.user = config.db.value().user.value();
        dbconfig.password = config.db.value().password.value();

        Server server(dbconfig);
        server.start("127.0.0.1", config.port.value());

        using namespace std::chrono_literals;
        // Blocks until something is triggered (such as shutdown command).
        while (server.is_running()) {
            std::this_thread::sleep_for(10ms); // Gives out CPU
        }
    }
    catch (std::exception const &e) {
        spdlog::critical("Fatal error: {}", e.what());
    }
}
