add_executable(server-test)

target_sources(server-test
    PRIVATE
        server-test.cpp
)

target_link_libraries(server-test
    PRIVATE
        libhc
        gtest::gtest
)
