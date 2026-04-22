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
        using config::datahome;

        auto config = Config{};
        std::string str;
        rfl::yaml::read<Config>(str);

        CLI::App app("homework-collection-remastered", "hc");
        app.add_flag("-V,--version", config.show_version,
                     "Print hc version and exit");
        app.add_flag("-v,--verbose", config.verbose, "Use debug mode");
        app.add_flag("--ask", config.ask,
                     "Ask username and password for database connection");
        app.add_option("-p,--port", config.port, "Port of the web server");
        CLI11_PARSE(app, argc, argv);

        spdlog::set_level(config.verbose.value() ? spdlog::level::debug
                                                 : spdlog::level::info);
        spdlog::debug("datahome={}", datahome().string());
        spdlog::debug("verbose={}", config.verbose.value());
        spdlog::debug("show_version={}", config.show_version.value());
        spdlog::debug("port={}", config.port.value());

        if (config.show_version.value()) {
            std::println("hc version {}", HCRE_VERSION);
            return 0;
        }

        // Create a connection configuration.
        if (config.ask.value()) {
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
