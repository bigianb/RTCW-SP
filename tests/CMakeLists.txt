
add_executable(example_test exampleTest.cpp)
target_link_libraries(example_test PRIVATE Catch2::Catch2WithMain)


catch_discover_tests(example_test)
