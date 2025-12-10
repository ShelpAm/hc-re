add_library(hc STATIC)

target_sources(hc
    PRIVATE
        server.cpp
        archive.cpp
)

target_link_libraries(hc
    PUBLIC
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        sqlpp23::postgresql
        Boost::headers
        httplib::httplib
        cppcodec::cppcodec
        LibArchive::LibArchive
)

target_include_directories(hc
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
)

add_library(hc::core ALIAS hc)


add_library(mock STATIC)
target_sources(mock
    PRIVATE
        mock/mock-client.cpp
)
target_link_libraries(mock
    PUBLIC
        httplib::httplib
    PRIVATE
        gtest::gtest
        nlohmann_json::nlohmann_json
)
target_include_directories(mock
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
)
add_library(hc::mock ALIAS mock)


add_library(all_lib INTERFACE)
target_link_libraries(all_lib
    INTERFACE
        hc::core
        hc::mock
)
add_library(hc::hc ALIAS all_lib)
