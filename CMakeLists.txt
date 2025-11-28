cmake_minimum_required(VERSION 3.30)

project(hc-re)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_CXX_STANDARD 26)

# Resolves dependencies
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
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
find_package(GTest REQUIRED)
find_package(cppcodec REQUIRED)
# End

add_library(libhc)

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


add_subdirectory(tests)
# include(cmake/msg.cmake)
