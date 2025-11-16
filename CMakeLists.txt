cmake_minimum_required(VERSION 3.14)
project(laba5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0
  TLS_VERIFY false
)
FetchContent_MakeAvailable(googletest)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(demo main.cpp)

add_executable(queue_pmr_test tests/queue_pmr_test.cpp)
target_link_libraries(queue_pmr_test gtest_main)

if(MINGW)
    target_link_options(queue_pmr_test PRIVATE -lpthread)
endif()

enable_testing()
add_test(NAME QueuePMRTest COMMAND queue_pmr_test)