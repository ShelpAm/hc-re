add_executable(server-test)

target_sources(server-test
    PRIVATE
        server-test.cpp
)

target_link_libraries(server-test
    PRIVATE
        libhc
        gtest::gtest
        spdlog::spdlog
)

add_executable(playground pg.cpp)

target_link_libraries(playground
    PRIVATE
        libhc
)

add_executable(optional-test)

target_sources(optional-test
    PRIVATE
        optional-test.cpp
)

target_link_libraries(optional-test
    PRIVATE
        libhc
        gtest::gtest
)
