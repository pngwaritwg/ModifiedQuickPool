set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/tests)

set(testbin "")

macro(add_testfile source_name)
    add_executable(${source_name}_test ${source_name}.cpp)
    target_link_libraries(${source_name}_test PUBLIC Boost::unit_test_framework Threads::Threads Quickpool)
    # add_test(NAME ${source_name}_test COMMAND ${source_name}_test)
    list(APPEND testbin ${source_name}_test)
endmacro()

add_testfile(quickpool_sharing)
add_testfile(quickpool_offline)
add_testfile(quickpool_online)
add_testfile(quickpool_endpoint)
add_testfile(quickpool_intersect)

add_custom_target(tests)
add_dependencies(tests ${testbin})


