project(oocddaTests LANGUAGES CXX)

# Dependencies
find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

# Tests
add_executable(oocdda_test src/enums_test.cpp src/file_utils_test.cpp
                           src/monster_type_test.cpp)

target_compile_features(oocdda_test PRIVATE cxx_std_20)
target_link_libraries(oocdda_test PRIVATE oocdda_lib GTest::gtest_main)

gtest_discover_tests(oocdda_test)
