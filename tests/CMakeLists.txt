
add_executable(tests server/world_test.cpp)

target_link_libraries(tests PRIVATE idlib server Catch2::Catch2WithMain)


catch_discover_tests(tests)
