add_executable(hc-cli)

set_target_properties(hc-cli
    PROPERTIES
        OUTPUT_NAME hc
)

target_sources(hc-cli
    PRIVATE
        main.cpp
)

target_link_libraries(hc-cli
    PRIVATE
        hc::hc
        CLI11::CLI11
        spdlog::spdlog
        sqlpp23::postgresql
)

target_link_options(hc-cli
    PRIVATE
        -static-libstdc++
        # -static-libgcc
)
