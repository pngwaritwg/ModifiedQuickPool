set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/benchmarks)

set(benchbin "")

macro(add_benchmark source_name)
    add_executable(${source_name} ${source_name}.cpp utils.cpp)
    target_link_libraries(${source_name} Boost::system Boost::program_options nlohmann_json::nlohmann_json Threads::Threads Quickpool)
    list(APPEND benchbin ${source_name})
endmacro()

add_benchmark(quickpool_ED)
add_benchmark(quickpool_intersection)

add_custom_target(benchmarks)
add_dependencies(benchmarks ${benchbin})
