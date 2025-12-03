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
    PRIVATE
        LibArchive::LibArchive
)

target_include_directories(hc
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
)

add_library(hc::hc ALIAS hc)

