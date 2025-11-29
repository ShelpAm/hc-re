if(NOT HCRE_TEST_DB)
    message(WARNING "HCRE_TEST_DB is empty, which may leads to problem")
endif()

add_executable(server-test)

target_sources(server-test
    PRIVATE
        server-test.cpp
)

target_compile_definitions(server-test
    PRIVATE HCRE_TEST_DB=\"${HCRE_TEST_DB}\"
)

target_link_libraries(server-test
    PRIVATE
        libhc
        gtest::gtest
        spdlog::spdlog
        libpqxx::pqxx # For mocking PG database
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
