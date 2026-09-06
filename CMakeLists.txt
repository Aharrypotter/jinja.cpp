cmake_minimum_required(VERSION 3.10)
project(jinja_cpp VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Header-only Library
add_library(jinja INTERFACE)
target_include_directories(jinja INTERFACE . third_party)

option(UJSON_USE_RAPIDJSON "Use RapidJSON backend" OFF)
if(UJSON_USE_RAPIDJSON)
    add_definitions(-DUJSON_USE_RAPIDJSON)
    include_directories(third_party/rapidjson/include)
endif()

# Test
enable_testing()
add_executable(test_main tests/test_main.cpp)
target_link_libraries(test_main jinja)

add_executable(test_transformers_compat tests/test_transformers_compat.cpp)
target_link_libraries(test_transformers_compat jinja)
target_compile_features(test_transformers_compat PRIVATE cxx_std_20)
target_compile_definitions(test_transformers_compat PRIVATE UJSON_USE_ORDERED_JSON=1 JINJA_PRESERVE_JSON_OBJECT_ORDER=1)
add_test(NAME jinja_transformers_compat COMMAND test_transformers_compat)
