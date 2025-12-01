cmake_minimum_required(VERSION 3.30)

project(hc-re VERSION 0.1.0)

option(HCRE_BUILD_TESTS "Whether to build tests" OFF)
set(HCRE_TEST_DB "" CACHE STRING
    "Postgresql test databse connection string, used for tests")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Resolves dependencies
## Header-only
find_package(nlohmann_json REQUIRED)
find_package(CLI11 REQUIRED)

include(FetchContent)
FetchContent_Declare(sqlpp23
    GIT_REPOSITORY  https://github.com/rbock/sqlpp23
    GIT_TAG         origin/main
)
set(BUILD_POSTGRESQL_CONNECTOR ON)
FetchContent_MakeAvailable(sqlpp23)

find_package(Boost REQUIRED)
find_package(httplib REQUIRED)
find_package(cppcodec REQUIRED)

## Compiled lib
find_package(spdlog REQUIRED)
find_package(GTest REQUIRED)
find_package(libpqxx REQUIRED)
# End

add_library(libhc)

set_target_properties(libhc
    PROPERTIES
        OUTPUT_NAME hc
)

target_sources(libhc
    PRIVATE
        libhc/server.cpp
)

target_compile_definitions(libhc
    PRIVATE
        SPDLOG_USE_STD_FORMAT
)

target_include_directories(libhc
    PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(libhc
    PUBLIC
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        CLI11::CLI11
        sqlpp23::postgresql
        Boost::headers
        httplib::httplib
        cppcodec::cppcodec
)


add_executable(hc)

target_sources(hc
    PRIVATE
        src/main.cpp
)

target_link_libraries(hc
    PRIVATE
        libhc
)

target_link_options(hc
    PRIVATE
        -static-libstdc++
        # -static-libgcc
)

# target_compile_options(hc
#     PRIVATE
#         -fsanitize=thread
# )
#
# target_link_options(hc
#     PRIVATE
#         -fsanitize=thread
# )

configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/version.h.in
    ${CMAKE_BINARY_DIR}/version/version.h
    @ONLY
)

target_include_directories(hc
    PRIVATE
    ${CMAKE_BINARY_DIR}/version
)


if(HCRE_BUILD_TESTS)
    add_subdirectory(tests)
endif()
# include(cmake/msg.cmake)


install(TARGETS hc
        EXPORT hc-targets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)

# Set the package name and version
set(CPACK_PACKAGE_NAME "hc-re")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "ShelpAm <shelpam@outlook.com>")

# CPACK_GENERATOR set by root CMakeLists.txt
include(CPack)
