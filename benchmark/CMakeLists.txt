add_executable(api-assignments-benchmark)

target_sources(api-assignments-benchmark
    PRIVATE
        api-assignments-benchmark.cpp
)

target_link_libraries(api-assignments-benchmark
    PRIVATE
        httplib::httplib
)
